#include "dv_vrdriver.hpp"
#include "dvwindow.hpp"
#include <QOpenGLExtraFunctions>
#include <QSGTextureProvider>
#include <QQuickItem>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLBuffer>
#include <QDebug>
#include <cmath>
#include <openvr.h>

class DV_VRDriver_OpenVR : public DV_VRDriver {
public:
    DV_VRDriver_OpenVR(DVWindow* w) : DV_VRDriver(w),
        distortionVBO(QOpenGLBuffer::VertexBuffer), distortionIBO(QOpenGLBuffer::IndexBuffer) {
        if (!vr::VR_IsHmdPresent()) {
            errorString = "No HMD detected.";
            return;
        }

        /* These wil be inited on first usage. */
        vrSystem = nullptr;
        renderFBO[0] = renderFBO[1] = nullptr;
        resolveFBO[0] = resolveFBO[1] = nullptr;

        vrSceneShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glsl/openvrscene.vsh");
        vrSceneShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glsl/openvrscene.fsh");

        vrSceneShader.link();

        distortionShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glsl/openvrdistortion.vsh");
        distortionShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glsl/openvrdistortion.fsh");

        /* Make sure the attributes are bound correctly. */
        distortionShader.bindAttributeLocation("position",     0);
        distortionShader.bindAttributeLocation("uvRedIn",      1);
        distortionShader.bindAttributeLocation("uvGreenIn",    2);
        distortionShader.bindAttributeLocation("uvBlueIn",     3);

        distortionShader.link();

        /* Bind so we set the texture sampler uniform values. */
        distortionShader.bind();

        /* The source texture will be bound to TEXTURE0. */
        distortionShader.setUniformValue("texture", 0);

        vr::EVRInitError error = vr::VRInitError_None;
        vrSystem = vr::VR_Init(&error, vr::VRApplication_Scene);

        if (error != vr::VRInitError_None) {
            vrSystem = nullptr;
            errorString = "Error initing VR system.";
            return;
        }

        /* TODO - Actually use this. Also, support tracked controllers. */
        renderModels = (vr::IVRRenderModels*)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &error);

        if (renderModels == nullptr) {
            vrSystem = nullptr;
            errorString = "Error getting render model interface.";
            return;
        }

        if (!vr::VRCompositor()) {
            vrSystem = nullptr;
            errorString = "Error getting compositor.";
            return;
        }

        /* Get the size for the FBOs. */
        vrSystem->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);

        /* Set up an FBO for each eye. */
        renderFBO[0] = new QOpenGLFramebufferObject(renderWidth, renderHeight);
        renderFBO[1] = new QOpenGLFramebufferObject(renderWidth, renderHeight);
        resolveFBO[0] = new QOpenGLFramebufferObject(renderWidth, renderHeight);
        resolveFBO[1] = new QOpenGLFramebufferObject(renderWidth, renderHeight);

        QVector<QVector2D> verts;
        QVector<GLushort> indexes;

        /* Calculate the left eye's distortion verts. */
        calculateEyeDistortion(vr::Eye_Left, verts, indexes, 0);
        /* Indexes for the right eye must be offset by how many verts there were for the left eye. */
        calculateEyeDistortion(vr::Eye_Right, verts, indexes, verts.size() / 4);

        /* Upload the distortion verts to the GPU. */
        distortionVBO.create();
        distortionVBO.bind();
        distortionVBO.allocate(verts.data(), verts.size() * sizeof(QVector2D));
        /* Upload the distortion indexes to the GPU. */
        distortionIBO.create();
        distortionIBO.bind();
        distortionIBO.allocate(indexes.data(), indexes.size() * sizeof(GLushort));

        /* Store the number of indexes for rendering. */
        distortionNumIndexes = indexes.size();

        qDebug("OpenVR inited.");
    }

    ~DV_VRDriver_OpenVR() {
        delete renderFBO[0]; delete renderFBO[1];
        delete resolveFBO[0]; delete resolveFBO[1];

        vr::VR_Shutdown();

        qDebug("OpenVR shutdown.");
    }

    void calculateEyeDistortion(vr::EVREye eye, QVector<QVector2D>& verts, QVector<GLushort>& indexes, int offset) {
        /* How many verts in each direction. */
        constexpr GLushort lensGridSegmentCount = 43;

        constexpr float w = 1.0f/(lensGridSegmentCount-1);
        constexpr float h = 1.0f/(lensGridSegmentCount-1);

        for (int y = 0; y < lensGridSegmentCount; ++y) {
            for (int x = 0; x < lensGridSegmentCount; ++x) {
                /* Calculate the undistorted UV coordinates for the vertex. */
                float u = x * w;
                float v = y * h;

                /* Place a vertex, taking the UV and mapping from [0, 1] to [-1, 1]. */
                verts.push_back(QVector2D(2.0f*u-1.0f, 2.0f*v-1.0f));

                vr::DistortionCoordinates_t distorted;
                /* Get the distortion coordinates from OpenVR. Invert v because of how OpenGL handles textures. */
                if (!vrSystem->ComputeDistortion(eye, u, 1.0f-v, &distorted))
                    qWarning("Error getting distortion coordinates!");

                /* Put the distortion coordinate into the list, inverting v back into OpenGL land. */
                verts.push_back(QVector2D(distorted.rfRed[0], 1.0f - distorted.rfRed[1]));
                verts.push_back(QVector2D(distorted.rfGreen[0], 1.0f - distorted.rfGreen[1]));
                verts.push_back(QVector2D(distorted.rfBlue[0], 1.0f - distorted.rfBlue[1]));

                /* No indexes for the last element in either x or y. */
                if (y == lensGridSegmentCount-1 || x == lensGridSegmentCount-1) continue;

                /* Calculate the index of the current vertex. */
                GLushort current = lensGridSegmentCount * y + x + offset;

                /* Add a quad as a pair of triangles with the current vertex at the corner. */
                indexes.push_back(current);
                indexes.push_back(current + 1);
                indexes.push_back(current + lensGridSegmentCount + 1);

                indexes.push_back(current);
                indexes.push_back(current + lensGridSegmentCount + 1);
                indexes.push_back(current + lensGridSegmentCount);
            }
        }
    }

    void renderEyeScene(vr::EVREye eye, const QMatrix4x4& head, QSGTexture* imgTexture, QRectF imgRect, qreal imgPan, bool isBackground) {
        QOpenGLExtraFunctions* f = window->openglContext()->extraFunctions();

        /* A matrix for each eye, to tell where it is relative to the user's head. */
        const vr::HmdMatrix34_t& eyeMatrix = vrSystem->GetEyeToHeadTransform(eye);

        /* Get a projection matrix for each eye. */
        const vr::HmdMatrix44_t& eyeProj = vrSystem->GetProjectionMatrix(eye,  0.1f, 1000.0f);

        /* Convert them all to QMatrix4x4 and combine them. */
        QMatrix4x4 eyeMat = QMatrix4x4(*eyeProj.m) * QMatrix4x4(QMatrix4x3(*eyeMatrix.m)) * head;

        /* Setup for the eye. */
        renderFBO[eye]->bind();
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (imgTexture != nullptr) {
            f->glDisable(GL_BLEND);

            QMatrix4x4 sphereMat;
            sphereMat.scale(900.0f);
            sphereMat.rotate(imgPan, 0.0f, 1.0f, 0.0f);
            vrSceneShader.setUniformValue("cameraMatrix", eyeMat * sphereMat);
            vrSceneShader.setUniformValue("rect", imgRect.x(), imgRect.y(), imgRect.width(), imgRect.height());

            imgTexture->bind();

            if (isBackground)
                vrSceneShader.setUniformValue("outputFac", float(1.0 - backgroundDim));

            /* Use the sphere provided by the normal surround rendering. */
            window->renderStandardSphere();

            if (!isBackground) {
                f->glEnable(GL_BLEND);
                f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
        }
        vrSceneShader.setUniformValue("cameraMatrix", eyeMat);
        vrSceneShader.setUniformValue("rect", 0.0f, 0.0f, 1.0f, 1.0f);
        vrSceneShader.setUniformValue("outputFac", 1.0f);

        f->glBindTexture(GL_TEXTURE_2D, eye == vr::Eye_Left ? window->getInterfaceLeftEyeTexture() : window->getInterfaceRightEyeTexture());

        /* Draw the screen to eye FBO. */
        f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, screen.data());
        f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, screenUV.data());
        f->glDrawArrays(GL_TRIANGLE_STRIP, 0, screen.size());

        renderFBO[eye]->release();
    }

    bool renderEyeDistortion(vr::EVREye eye, QOpenGLExtraFunctions *f) {
        resolveFBO[eye]->bind();
        f->glClear(GL_COLOR_BUFFER_BIT);

        /* Render right lens (second half of index array) */
        f->glBindTexture(GL_TEXTURE_2D, renderFBO[eye]->texture());
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        f->glDrawElements(GL_TRIANGLES, distortionNumIndexes/2, GL_UNSIGNED_SHORT, (const void*)(distortionNumIndexes * eye));

        resolveFBO[eye]->release();

        vr::Texture_t eyeTexture = { reinterpret_cast<void*>(static_cast<intptr_t>(resolveFBO[eye]->texture())), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
        if (vr::VRCompositor()->Submit(eye, &eyeTexture) != vr::VRCompositorError_None) {
            errorString = "Error submitting texture to OpenVR.";
            return false;
        }
        return true;
    }

    bool render() {
        QOpenGLExtraFunctions* f = window->openglContext()->extraFunctions();

        if (vrSystem == nullptr)
            return false;

        vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

        f->glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
        f->glClear(GL_COLOR_BUFFER_BIT);

        QRectF currentTextureLeft, currentTextureRight;
        QSGTexture* currentTexture = nullptr;
        qreal currentTexturePan = 0;
        /* Whether the value of currentTexture is the background image (true) or an opened image (false). */
        bool isBackground = false;

        if (window->isSurround()) {
            currentTexture = window->getCurrentTexture(currentTextureLeft, currentTextureRight);
            currentTexturePan = window->getSurroundPan().x();

            if (snapSurroundPan)
                /* Snap the pan value to multiples of 22.5 degrees to limit nausea. */
                currentTexturePan -= fmod(currentTexturePan, 22.5);
        }

        /* if the current image isn't a loaded or isn't surround, try to use the set background image. */
        if (currentTexture == nullptr && backgroundImageItem &&
                backgroundImageItem->textureProvider() && backgroundImageItem->textureProvider()->texture()) {
            currentTexture = backgroundImageItem->textureProvider()->texture();
            currentTexturePan = backgroundPan;

            window->getTextureRects(currentTextureLeft, currentTextureRight, currentTexture, backgroundSwap, backgroundSourceMode);

            isBackground = true;
        }

        /* Get the tracked position of the user's head. */
        QMatrix4x4 head;
        if (trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
            head = QMatrix4x4(QMatrix4x3(*trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m)).inverted();

        vrSceneShader.bind();
        f->glViewport(0, 0, renderWidth, renderHeight);

        f->glEnableVertexAttribArray(0);
        f->glEnableVertexAttribArray(1);
        renderEyeScene(vr::Eye_Left, head, currentTexture, currentTextureLeft, currentTexturePan, isBackground);
        renderEyeScene(vr::Eye_Right, head, currentTexture, currentTextureRight, currentTexturePan, isBackground);

        /* Get ready to render distortion. */
        distortionShader.bind();
        distortionVBO.bind();
        distortionIBO.bind();
        f->glClearColor(1.0f, 0.0f, 0.0f, 0.0f);

        /* Set up vertex arrays. */
        f->glEnableVertexAttribArray(2);
        f->glEnableVertexAttribArray(3);
        f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QVector2D) * 4, (void*)(0));
        f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QVector2D) * 4, (void*)(sizeof(QVector2D)));
        f->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(QVector2D) * 4, (void*)(sizeof(QVector2D) * 2));
        f->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(QVector2D) * 4, (void*)(sizeof(QVector2D) * 3));

        /* Render the distortion for both eyes and submit. */
        return renderEyeDistortion(vr::Eye_Left, f) && renderEyeDistortion(vr::Eye_Right, f);
    }

    void frameSwapped() {
        if (vrSystem != nullptr)
            vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
    }

    QOpenGLFramebufferObject* renderFBO[2];
    QOpenGLFramebufferObject* resolveFBO[2];

    QOpenGLBuffer distortionVBO;
    QOpenGLBuffer distortionIBO;
    intptr_t distortionNumIndexes;

    vr::IVRSystem* vrSystem;
    vr::IVRRenderModels* renderModels;
    vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];

    QOpenGLShaderProgram vrSceneShader;
    QOpenGLShaderProgram distortionShader;
};

DV_VRDriver* DV_VRDriver::createOpenVRDriver(DVWindow* window) {
    return new DV_VRDriver_OpenVR(window);
}
