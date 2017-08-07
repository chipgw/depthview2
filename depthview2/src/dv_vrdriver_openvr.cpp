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
#include <QOpenGLTexture>
#include <QDebug>
#include <QMap>
#include <QThread>
#include <cmath>
#include <openvr.h>

struct ModelComponent {
    QOpenGLBuffer VBO;
    QOpenGLBuffer IBO;

    ModelComponent(const vr::RenderModel_t& model, const vr::RenderModel_TextureMap_t& tex, QOpenGLExtraFunctions* f)
        : VBO(QOpenGLBuffer::VertexBuffer), IBO(QOpenGLBuffer::IndexBuffer), texture(QOpenGLTexture::Target2D) {
        /* Upload the verts to the GPU. */
        VBO.create(); VBO.bind();
        VBO.allocate(model.rVertexData, sizeof(vr::RenderModel_Vertex_t) * model.unVertexCount);
        /* Upload the indexes to the GPU. */
        IBO.create(); IBO.bind();
        IBO.allocate(model.rIndexData, sizeof(uint16_t) * model.unTriangleCount * 3);

        vertexCount = model.unTriangleCount * 3;

        /* Prepare to upload the texture. */
        texture.create(); texture.bind();
        f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.unWidth, tex.unHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.rubTextureMapData);

        /* We want mipmaps. */
        f->glGenerateMipmap(GL_TEXTURE_2D);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        GLfloat largest;
        f->glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largest);
        f->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largest);
    }

    QOpenGLTexture texture;
    GLsizei vertexCount;
};

class DV_VRDriver_OpenVR : public DV_VRDriver {
    /* Model name -> (component name -> component model data). */
    QMap<QByteArray, QMap<QByteArray, ModelComponent*>> renderModels;

    /* List of all loaded model components by their component model name. */
    QMap<QByteArray, ModelComponent*> loadedComponents;

    /* Tracked device -> model name. */
    QByteArray modelForDevice[vr::k_unMaxTrackedDeviceCount];

public:
    DV_VRDriver_OpenVR(DVWindow* w) : DV_VRDriver(w), distortionVBO(QOpenGLBuffer::VertexBuffer), distortionIBO(QOpenGLBuffer::IndexBuffer) {
        if (!vr::VR_IsHmdPresent()) {
            errorString = "No HMD detected.";
            return;
        }

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

        vr::EVRInitError error = vr::VRInitError_None;
        vrSystem = vr::VR_Init(&error, vr::VRApplication_Scene);

        if (error != vr::VRInitError_None) {
            vrSystem = nullptr;
            errorString = "Error initing VR system.";
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
        renderFBO[0] = new QOpenGLFramebufferObject(renderWidth, renderHeight, QOpenGLFramebufferObject::Depth);
        renderFBO[1] = new QOpenGLFramebufferObject(renderWidth, renderHeight, QOpenGLFramebufferObject::Depth);
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

        /* Load the models for attached devices. */
        for (uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i) setupDeviceModel(i);

        qDebug("OpenVR inited.");
    }

    ~DV_VRDriver_OpenVR() {
        delete renderFBO[0]; delete renderFBO[1];
        delete resolveFBO[0]; delete resolveFBO[1];

        /* Delete all loaded render models. */
        for (ModelComponent* m : loadedComponents) delete m;

        vr::VR_Shutdown();

        qDebug("OpenVR shutdown.");
    }

    void handleVREvent(const vr::VREvent_t& e) {
        switch (e.eventType) {
        case vr::VREvent_TrackedDeviceActivated:
            /* Load the model for the newly attached device. */
            setupDeviceModel(e.trackedDeviceIndex);
            break;
        }
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
        QMatrix4x4 eyeMat = QMatrix4x4(*eyeProj.m) * QMatrix4x4(QMatrix4x3(*eyeMatrix.m)).inverted() * head;

        /* Setup for the eye. */
        renderFBO[eye]->bind();
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (imgTexture != nullptr) {
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

            if (!isBackground)
                f->glEnable(GL_BLEND);
        }
        vrSceneShader.setUniformValue("cameraMatrix", eyeMat);
        vrSceneShader.setUniformValue("rect", 0.0f, 0.0f, 1.0f, 1.0f);
        vrSceneShader.setUniformValue("outputFac", 1.0f);

        /* Get the UI texture for the current eye from the window. */
        f->glBindTexture(GL_TEXTURE_2D, eye == vr::Eye_Left ? window->getInterfaceLeftEyeTexture() : window->getInterfaceRightEyeTexture());

        /* Draw the screen to eye FBO. */
        vrSceneShader.setAttributeArray(0, screen.data());
        vrSceneShader.setAttributeArray(1, screenUV.data());
        f->glDrawArrays(GL_TRIANGLE_STRIP, 0, screen.size());

        /* Don't use blending for tracked models. */
        f->glDisable(GL_BLEND);

        for (uint32_t trackedDevice = 0; trackedDevice < vr::k_unMaxTrackedDeviceCount; ++trackedDevice) {
            /* Only render valid devices of the controller class. */
            if (!renderModels.contains(modelForDevice[trackedDevice]) || vrSystem->GetTrackedDeviceClass(trackedDevice) != vr::TrackedDeviceClass_Controller)
                continue;

            const vr::TrackedDevicePose_t& pose = trackedDevicePose[trackedDevice];
            /* Don't render when the tracking data isn't valid. */
            if (!pose.bPoseIsValid) continue;

            QMatrix4x4 deviceToEye = eyeMat * QMatrix4x4(QMatrix4x3(*pose.mDeviceToAbsoluteTracking.m));

            const auto& componentsByName = renderModels[modelForDevice[trackedDevice]];

            /* Go through each component to render it. */
            for (auto component = componentsByName.cbegin(), end = componentsByName.cend(); component != end; ++component) {
                /* Get the current state of the controller. */
                vr::VRControllerState_t controllerState; vrSystem->GetControllerState(trackedDevice, &controllerState, sizeof(controllerState));

                /* I'm making assumptions here... */
                const vr::RenderModel_ControllerMode_State_t renderModelState = { false };

                /* This is where we get the render model system to put the component matrix. */
                vr::RenderModel_ComponentState_t componentState;

                /* Get the component state information from OpenVR. */
                vr::VRRenderModels()->GetComponentState(modelForDevice[trackedDevice].data(), component.key().data(), &controllerState, &renderModelState, &componentState);

                vrSceneShader.setUniformValue("cameraMatrix", deviceToEye * QMatrix4x4(QMatrix4x3(*componentState.mTrackingToComponentRenderModel.m)));

                component.value()->VBO.bind();
                component.value()->IBO.bind();

                /* This model also has normals, but there isn't any lighting so they're unnecessary. */
                vrSceneShader.setAttributeBuffer(0, GL_FLOAT, offsetof(vr::RenderModel_Vertex_t, vPosition), 3, sizeof(vr::RenderModel_Vertex_t));
                vrSceneShader.setAttributeBuffer(1, GL_FLOAT, offsetof(vr::RenderModel_Vertex_t, rfTextureCoord), 2, sizeof(vr::RenderModel_Vertex_t));

                component.value()->texture.bind();

                f->glDrawElements(GL_TRIANGLES, component.value()->vertexCount, GL_UNSIGNED_SHORT, 0);

                component.value()->VBO.release();
                component.value()->IBO.release();
            }
        }

        renderFBO[eye]->release();
    }

    bool renderEyeDistortion(vr::EVREye eye, QOpenGLExtraFunctions *f) {
        resolveFBO[eye]->bind();
        f->glClear(GL_COLOR_BUFFER_BIT);

        f->glBindTexture(GL_TEXTURE_2D, renderFBO[eye]->texture());
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        /* First half = left lens, second half = right lens. */
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

        if (vrSystem == nullptr) return false;

        /* Handle events from OpenVR. */
        vr::VREvent_t event;
        while (vrSystem->PollNextEvent(&event, sizeof(event)))
            handleVREvent(event);

        vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

        f->glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
        f->glClear(GL_COLOR_BUFFER_BIT);
        f->glEnable(GL_DEPTH_TEST);
        f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

        /* Set up vertex buffers for distortion rendering. */
        f->glEnableVertexAttribArray(2);
        f->glEnableVertexAttribArray(3);
        distortionShader.setAttributeBuffer(0, GL_FLOAT, 0,                     2, sizeof(QVector2D) * 4);
        distortionShader.setAttributeBuffer(1, GL_FLOAT, sizeof(QVector2D),     2, sizeof(QVector2D) * 4);
        distortionShader.setAttributeBuffer(2, GL_FLOAT, sizeof(QVector2D) * 2, 2, sizeof(QVector2D) * 4);
        distortionShader.setAttributeBuffer(3, GL_FLOAT, sizeof(QVector2D) * 3, 2, sizeof(QVector2D) * 4);

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
    vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];

    QOpenGLShaderProgram vrSceneShader;
    QOpenGLShaderProgram distortionShader;

    /* Get a tracked device property string. */
    QByteArray getTrackedDeviceString(vr::TrackedDeviceIndex_t deviceIndex, vr::TrackedDeviceProperty prop) {
        uint32_t bufferLen = vrSystem->GetStringTrackedDeviceProperty(deviceIndex, prop, nullptr, 0, nullptr);

        if (bufferLen == 0) return "";

        QByteArray buffer(bufferLen, 0);
        vrSystem->GetStringTrackedDeviceProperty(deviceIndex, prop, buffer.data(), buffer.length(), nullptr);

        return buffer;
    }

    void setupDeviceModel(vr::TrackedDeviceIndex_t deviceIndex) {
        if (deviceIndex >= vr::k_unMaxTrackedDeviceCount) return;

        QByteArray modelName = getTrackedDeviceString(deviceIndex, vr::Prop_RenderModelName_String);

        /* OpenVR doesn't have a model to load. */
        if (modelName.isEmpty()) return;

        modelForDevice[deviceIndex] = modelName;

        /* If the model is loaded already we're good to go, otherwise try to load it. */
        if (renderModels.contains(modelName)) return;

        /* Easy access to the current model's components. This will create a value in the renderModels map for the current model. */
        auto& componentMap = renderModels[modelName];

        for (uint32_t i = 0; i < vr::VRRenderModels()->GetComponentCount(modelName.data()); ++i) {
            /* Get the name of the component. */
            QByteArray componentName(vr::VRRenderModels()->GetComponentName(modelName.data(), i, nullptr, 0), 0);
            vr::VRRenderModels()->GetComponentName(modelName.data(), i, componentName.data(), componentName.length());

            /* The component model name is not the same as the component name, it is used to get the model data and to save it in the loadedModels map. */
            QByteArray componentModelName(vr::VRRenderModels()->GetComponentRenderModelName(modelName.data(), componentName.data(), nullptr, 0), 0);
            vr::VRRenderModels()->GetComponentRenderModelName(modelName.data(), componentName.data(), componentModelName.data(), componentModelName.length());

            /* There isn't any model for this component. */
            if (componentModelName.isEmpty()) continue;

            /* Check the loaded models to see if we already have it. */
            if (loadedComponents.contains(componentModelName)) {
                componentMap[componentName] = loadedComponents[componentModelName];
                continue;
            }

            /* If there was none already loaded, try to load it. */
            vr::RenderModel_t* model;
            vr::RenderModel_TextureMap_t* texture;
            vr::EVRRenderModelError error;

            /* Wait for the model to load. */
            do {
                error = vr::VRRenderModels()->LoadRenderModel_Async(componentModelName.data(), &model);
                QThread::msleep(200);
            } while (error == vr::VRRenderModelError_Loading);

            if (error != vr::VRRenderModelError_None) {
                qDebug("Failed to load component %s for render model %s for device %d - %s", modelName.data(), componentModelName.data(), deviceIndex,
                       vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error));
                continue;
            }

            /* Wait for the texture to load. */
            do {
                error = vr::VRRenderModels()->LoadTexture_Async(model->diffuseTextureId, &texture);
                QThread::msleep(200);
            } while (error == vr::VRRenderModelError_Loading);

            if (error != vr::VRRenderModelError_None) {
                qDebug("Failed to load texture %d for render model %s", model->diffuseTextureId, componentModelName.data());
                vr::VRRenderModels()->FreeRenderModel(model);
                continue;
            }

            /* Create the OpenGL buffers from the loaded data. and save it both in the current model and the list of all loaded components. */
            componentMap[componentName] = loadedComponents[componentModelName] = new ModelComponent(*model, *texture, window->openglContext()->extraFunctions());

            /* We don't need the model data any more, it has been uploaded to the GPU. */
            vr::VRRenderModels()->FreeRenderModel(model); vr::VRRenderModels()->FreeTexture(texture);
        }
    }
};

DV_VRDriver* DV_VRDriver::createOpenVRDriver(DVWindow* window) {
    return new DV_VRDriver_OpenVR(window);
}
