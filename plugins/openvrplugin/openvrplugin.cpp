#include "openvrplugin.hpp"
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLBuffer>
#include <QQmlComponent>
#include <QQuickItem>
#include <QDebug>
#include <cmath>

bool OpenVRPlugin::init(QOpenGLExtraFunctions* f, QQmlEngine* qmlEngine) {
    Q_UNUSED(f)
    Q_INIT_RESOURCE(openvrplugin);

    if (!vr::VR_IsHmdPresent()) {
        qDebug("No HMD detected...");
        return false;
    }

    /* This wil be inited on first usage. */
    vrSystem = nullptr;

    mirrorShader = new QOpenGLShaderProgram;

    mirrorShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/OpenVR/glsl/mirror.vsh");
    mirrorShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/OpenVR/glsl/mirror.fsh");

    mirrorShader->link();

    /* Bind so we set the texture sampler uniform values. */
    mirrorShader->bind();

    /* Left image is TEXTURE0. */
    mirrorShader->setUniformValue("textureL", 0);
    /* Right image is TEXTURE1. */
    mirrorShader->setUniformValue("textureR", 1);

    vrSceneShader = new QOpenGLShaderProgram;
    vrSceneShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/OpenVR/glsl/vrscene.vsh");
    vrSceneShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/OpenVR/glsl/vrscene.fsh");

    vrSceneShader->link();

    /* Bind so we set the texture sampler uniform values. */
    vrSceneShader->bind();

    /* Left image is TEXTURE0. */
    vrSceneShader->setUniformValue("textureL", 0);
    /* Right image is TEXTURE1. */
    vrSceneShader->setUniformValue("textureR", 1);

    distortionShader = new QOpenGLShaderProgram;
    distortionShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/OpenVR/glsl/distortion.vsh");
    distortionShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/OpenVR/glsl/distortion.fsh");

    /* Make sure the attributes are bound correctly. */
    distortionShader->bindAttributeLocation("position",     0);
    distortionShader->bindAttributeLocation("uvRedIn",      1);
    distortionShader->bindAttributeLocation("uvGreenIn",    2);
    distortionShader->bindAttributeLocation("uvBlueIn",     3);

    distortionShader->link();

    /* Bind so we set the texture sampler uniform values. */
    distortionShader->bind();

    /* The source texture will be bound to TEXTURE0. */
    distortionShader->setUniformValue("texture", 0);

    QQmlComponent component(qmlEngine);

    component.loadUrl(QUrl(QStringLiteral("qrc:/OpenVR/Config.qml")));

    /* Wait for it to load... */
    while(component.isLoading());

    /* The program can't run if there was an error. */
    if (component.isError()) {
        qDebug(qPrintable(component.errorString()));
        return false;
    }

    configMenu = qobject_cast<QQuickItem*>(component.create());

    /* Critical error! abort! abort! */
    if (configMenu == nullptr)
        return false;

    screenDistance = QQmlProperty(configMenu, "screenDistance");
    screenHeight = QQmlProperty(configMenu, "screenHeight");
    screenSize = QQmlProperty(configMenu, "screenSize");
    curvedScreen = QQmlProperty(configMenu, "curvedScreen");
    lockMouse = QQmlProperty(configMenu, "lockMouse");
    screenDistance.connectNotifySignal(this, SLOT(updateScreen()));
    screenHeight.connectNotifySignal(this, SLOT(updateScreen()));
    screenSize.connectNotifySignal(this, SLOT(updateScreen()));
    curvedScreen.connectNotifySignal(this, SLOT(updateScreen()));

    qDebug("OpenVR plugin base inited.");

    return true;
}

bool OpenVRPlugin::deinit() {
    delete mirrorShader;
    delete vrSceneShader;
    delete distortionShader;

    delete leftEyeRenderFBO;
    delete rightEyeRenderFBO;
    delete leftEyeResolveFBO;
    delete rightEyeResolveFBO;

    vr::VR_Shutdown();

    qDebug("OpenVR shutdown.");

    return true;
}

bool OpenVRPlugin::initVR(QOpenGLExtraFunctions* f) {
    Q_UNUSED(f)

    vr::EVRInitError eError = vr::VRInitError_None;
    vrSystem = vr::VR_Init(&eError, vr::VRApplication_Scene);

    if (eError != vr::VRInitError_None) {
        vrSystem = nullptr;
        qDebug("Error initing vr system!");
        return false;
    }

    /* TODO - Actually use this. Also, support tracked controllers. (Gonna be hard, considering I have none...) */
    renderModels = (vr::IVRRenderModels*)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);

    if (renderModels == nullptr) {
        vrSystem = nullptr;
        qDebug("Error getting render model interface!");
        return false;
    }

    if (!vr::VRCompositor()) {
        vrSystem = nullptr;
        qDebug("Error getting compositor!");
        return false;
    }

    /* Get the size for the FBOs. */
    vrSystem->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);

    /* Set up an FBO for each eye. */
    leftEyeRenderFBO = new QOpenGLFramebufferObject(renderWidth, renderHeight);
    rightEyeRenderFBO = new QOpenGLFramebufferObject(renderWidth, renderHeight);
    leftEyeResolveFBO = new QOpenGLFramebufferObject(renderWidth, renderHeight);
    rightEyeResolveFBO = new QOpenGLFramebufferObject(renderWidth, renderHeight);

    /* How many verts in each direction. */
    GLushort lensGridSegmentCountH = 43;
    GLushort lensGridSegmentCountV = 43;

    float w = 1.0f/(lensGridSegmentCountH-1);
    float h = 1.0f/(lensGridSegmentCountV-1);

    float u, v;

    GLushort a,b,c,d;

    QVector<QVector2D> verts;
    QVector<GLushort> indexes;

    /* Calculate the left eye's distortion verts. */
    for (int y = 0; y < lensGridSegmentCountV; ++y) {
        for (int x = 0; x < lensGridSegmentCountH; ++x) {
            /* Calculate the undistorted UV coordinates for the vertex. */
            u = x*w;
            v = y*h;

            /* Place a vertex, taking the UV and mapping from [0, 1] to [-1, 1]. */
            verts.push_back(QVector2D(2.0f*u-1.0f, 2.0f*v-1.0f));

            /* Get the distortion coordinates from OpenVR. Invert v because of how OpenGL handles textures. */
            auto distorted = vrSystem->ComputeDistortion(vr::Eye_Left, u, 1.0f-v);

            /* Put the distortion coordinate into the list, inverting v back into OpenGL land. */
            verts.push_back(QVector2D(distorted.rfRed[0], 1.0f - distorted.rfRed[1]));
            verts.push_back(QVector2D(distorted.rfGreen[0], 1.0f - distorted.rfGreen[1]));
            verts.push_back(QVector2D(distorted.rfBlue[0], 1.0f - distorted.rfBlue[1]));

            if (y == lensGridSegmentCountV-1 || x == lensGridSegmentCountH-1)
                continue;

            /* Calculate the indexes for a nice little quad. */
            a = lensGridSegmentCountH*y+x;
            b = lensGridSegmentCountH*y+x+1;
            c = (y+1)*lensGridSegmentCountH+x+1;
            d = (y+1)*lensGridSegmentCountH+x;

            /* Add the quad as a pair of triangles. */
            indexes.push_back(a);
            indexes.push_back(b);
            indexes.push_back(c);

            indexes.push_back(a);
            indexes.push_back(c);
            indexes.push_back(d);
        }
    }

    /* Indexes for the right eye must be offset by how many verts there were for the left eye. */
    int offset = verts.size() / 4;

    /* Calculate the right eye's distortion verts. */
    for (int y = 0; y < lensGridSegmentCountV; ++y) {
        for (int x = 0; x < lensGridSegmentCountH; ++x) {
            /* Calculate the undistorted UV coordinates for the vertex. */
            u = x*w;
            v = y*h;

            /* Place a vertex, taking the UV and mapping from [0, 1] to [-1, 1]. */
            verts.push_back(QVector2D(2.0f*u-1.0f, 2.0f*v-1.0f));

            /* Get the distortion coordinates from OpenVR. Invert v because of how OpenGL handles textures. */
            auto distorted = vrSystem->ComputeDistortion(vr::Eye_Right, u, 1.0f-v);

            /* Put the distortion coordinate into the list, inverting v back into OpenGL land. */
            verts.push_back(QVector2D(distorted.rfRed[0], 1.0f - distorted.rfRed[1]));
            verts.push_back(QVector2D(distorted.rfGreen[0], 1.0f - distorted.rfGreen[1]));
            verts.push_back(QVector2D(distorted.rfBlue[0], 1.0f - distorted.rfBlue[1]));

            if (y == lensGridSegmentCountV-1 || x == lensGridSegmentCountH-1)
                continue;

            /* Calculate the indexes for a nice little quad. */
            a = lensGridSegmentCountH*y+x +offset;
            b = lensGridSegmentCountH*y+x+1 +offset;
            c = (y+1)*lensGridSegmentCountH+x+1 +offset;
            d = (y+1)*lensGridSegmentCountH+x +offset;

            /* Add the quad as a pair of triangles. */
            indexes.push_back(a);
            indexes.push_back(b);
            indexes.push_back(c);

            indexes.push_back(a);
            indexes.push_back(c);
            indexes.push_back(d);
        }
    }

    /* Upload the distortion verts to the GPU. */
    distortionVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    distortionVBO->create();
    distortionVBO->bind();
    distortionVBO->allocate(verts.data(), verts.size() * sizeof(QVector2D));
    /* Upload the distortion indexes to the GPU. */
    distortionIBO = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    distortionIBO->create();
    distortionIBO->bind();
    distortionIBO->allocate(indexes.data(), indexes.size() * sizeof(GLushort));

    /* Store the number of indexes for rendering. */
    distortionNumIndexes = indexes.size();

    qDebug("OpenVR inited.");

    return true;
}

bool OpenVRPlugin::render(const QString& drawModeName, QOpenGLExtraFunctions* f) {
    Q_UNUSED(drawModeName)

    if (vrSystem == nullptr && !initVR(f))
        return false;

    vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0 );

    /* This is just the default fullscreen quad from the built-in modes. */
    static const float quad[] {
       -1.0f,-1.0f,
        1.0f,-1.0f,
        1.0f, 1.0f,
       -1.0f, 1.0f
    };

    static const float quadUV[] {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    /* Enable the vertex and UV arrays, must be done every frame because of QML resetting things. */
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);

    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, quad);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, quadUV);

    /* Draw left eye to monitor as if in mono left mode. */
    mirrorShader->bind();
    mirrorShader->setUniformValue("left", true);
    f->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    /* Get the tracked position of the user's head. */
    QMatrix4x4 head;
    if (trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
        head = QMatrix4x4(QMatrix4x3(*trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m)).inverted();

    /* A matrix for each eye, to tell where it is relative to the user's head. */
    const vr::HmdMatrix34_t& leftMatrix = vrSystem->GetEyeToHeadTransform(vr::Eye_Left);
    const vr::HmdMatrix34_t& rightMatrix = vrSystem->GetEyeToHeadTransform(vr::Eye_Right);

    /* Get a projection matrix for each eye. */
    const vr::HmdMatrix44_t& leftProj = vrSystem->GetProjectionMatrix(vr::Eye_Left,  0.1f, 100.0f, vr::API_OpenGL);
    const vr::HmdMatrix44_t& rightProj = vrSystem->GetProjectionMatrix(vr::Eye_Right, 0.1f, 100.0f, vr::API_OpenGL);

    /* Convert them all to QMatrix4x4 and combine them. */
    QMatrix4x4 leftEyeMat  = QMatrix4x4(*leftProj.m) *  QMatrix4x4(QMatrix4x3(*leftMatrix.m))  * head;
    QMatrix4x4 rightEyeMat = QMatrix4x4(*rightProj.m) * QMatrix4x4(QMatrix4x3(*rightMatrix.m)) * head;

    /* Get the size of the window. */
    GLint viewport[4];
    f->glGetIntegerv(GL_VIEWPORT, viewport);

    /* Check to see if the aspect ratio is the same, if not, we update the screen. */
    if (aspectRatio != float(viewport[3]) / float(viewport[2])) {
        aspectRatio = float(viewport[3]) / float(viewport[2]);
        updateScreen();
    }

    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, screen.data());
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, screenUV.data());

    vrSceneShader->bind();
    f->glViewport(0, 0, renderWidth, renderHeight);

    /* TODO - Configurable backgrounds and 360 images. */
    f->glClearColor(0.1f, 0.1f, 0.1f, 0.0f);

    /* Setup for the left eye. */
    leftEyeRenderFBO->bind();
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    vrSceneShader->setUniformValue("left", true);
    vrSceneShader->setUniformValue("cameraMatrix", leftEyeMat);

    /* Draw left eye to VR FBO. */
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, screen.size());

    leftEyeRenderFBO->release();

    /* Setup for the right eye. */
    rightEyeRenderFBO->bind();
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    vrSceneShader->setUniformValue("left", false);
    vrSceneShader->setUniformValue("cameraMatrix", rightEyeMat);

    /* Draw right eye to VR FBO. */
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, screen.size());

    rightEyeRenderFBO->release();

    /* Get ready to render distortion. */
    distortionShader->bind();
    distortionVBO->bind();
    distortionIBO->bind();
    f->glClearColor(1.0f, 0.0f, 0.0f, 0.0f);

    /* Set up vertex arrays. */
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);
    f->glEnableVertexAttribArray(2);
    f->glEnableVertexAttribArray(3);
    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QVector2D) * 4, (void*)(0));
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QVector2D) * 4, (void*)(sizeof(QVector2D)));
    f->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(QVector2D) * 4, (void*)(sizeof(QVector2D) * 2));
    f->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(QVector2D) * 4, (void*)(sizeof(QVector2D) * 3));

    f->glDisable(GL_DEPTH_TEST);
    f->glDisable(GL_CULL_FACE);
    f->glActiveTexture(GL_TEXTURE0);

    leftEyeResolveFBO->bind();
    f->glClear(GL_COLOR_BUFFER_BIT);

    /* Render left lens (first half of index array) */
    f->glBindTexture(GL_TEXTURE_2D, leftEyeRenderFBO->texture());
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glDrawElements(GL_TRIANGLES, distortionNumIndexes/2, GL_UNSIGNED_SHORT, 0);

    leftEyeResolveFBO->release();

    rightEyeResolveFBO->bind();
    f->glClear(GL_COLOR_BUFFER_BIT);

    /* Render right lens (second half of index array) */
    f->glBindTexture(GL_TEXTURE_2D, rightEyeRenderFBO->texture());
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glDrawElements(GL_TRIANGLES, distortionNumIndexes/2, GL_UNSIGNED_SHORT, (const void*)distortionNumIndexes);

    rightEyeResolveFBO->release();

    /* All done with distortion. */
    distortionVBO->release();
    distortionIBO->release();
    f->glBindTexture(GL_TEXTURE_2D, 0);

    vr::Texture_t leftEyeTexture = { reinterpret_cast<void*>(static_cast<intptr_t>(leftEyeResolveFBO->texture())), vr::API_OpenGL, vr::ColorSpace_Gamma };
    vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);

    vr::Texture_t rightEyeTexture = { reinterpret_cast<void*>(static_cast<intptr_t>(rightEyeResolveFBO->texture())), vr::API_OpenGL, vr::ColorSpace_Gamma };
    vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);

    /* All's well that ends well... */
    return true;
}

void OpenVRPlugin::frameSwapped(QOpenGLExtraFunctions* f) {
    Q_UNUSED(f)

    if (vrSystem != nullptr)
        vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
}

QStringList OpenVRPlugin::drawModeNames() {
    return QStringList({"OpenVR"});
}

QQuickItem* OpenVRPlugin::getConfigMenuObject() {
    return configMenu;
}

bool OpenVRPlugin::shouldLockMouse() {
    return lockMouse.read().toBool();
}

QSize OpenVRPlugin::getRenderSize(const QSize& windowSize) {
    return windowSize;
}

void OpenVRPlugin::updateScreen() {
    /* Get the properties from QML. */
    float distance = screenDistance.read().toFloat();
    float z = screenHeight.read().toFloat();
    float size = screenSize.read().toFloat();

    float height = size * aspectRatio;

    /* Get rid of any previous geometry. */
    screen.clear();
    screenUV.clear();

    if (curvedScreen.read().toBool()) {
        constexpr int halfSteps = 50;
        float theta = size / distance;

        /* Theta is the angle of half the screen,so this is angle of each step.  */
        float step = theta / halfSteps;

        for (int current = -halfSteps; current < halfSteps; ++current) {
            /* Calculate the coordinate of the vertex. */
            screen += {{std::sin(current * step) * distance, z - height, std::cos(current * step) * -distance},
                       {std::sin(current * step) * distance, z + height, std::cos(current * step) * -distance}};

            /* Map current from [-halfStep, halfStep] to [0, 1]. */
            float U = 0.5f * current / halfSteps + 0.5f;

            screenUV += {{U, 0.0f}, {U, 1.0f}};
        }
    } else {
        /* A simple rectangle... */
        screen += {{-size, z - height, -distance},  /* 2--4 */
                   {-size, z + height, -distance},  /* |\ | */
                   { size, z - height, -distance},  /* | \| */
                   { size, z + height, -distance}}; /* 1--3 */

        screenUV += {{0.0f, 0.0f},
                     {0.0f, 1.0f},
                     {1.0f, 0.0f},
                     {1.0f, 1.0f}};
    }
}

bool OpenVRPlugin::pollInput(DVInputInterface* inputInterface) {
    Q_UNUSED(inputInterface)
    /* TODO - Use tracked motion controllers... */
    return false;
}
