#include "openvrplugin.hpp"
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QQmlComponent>
#include <QQuickItem>
#include <QDebug>

bool OpenVRPlugin::init(QOpenGLFunctions* f, QQmlEngine* qmlEngine) {
    Q_UNUSED(f)
    Q_INIT_RESOURCE(openvrplugin);

    if (!vr::VR_IsHmdPresent()) {
        qDebug("No HMD detected...");
        return false;
    }

    /* This wil be inited on first usage. */
    vrSystem = nullptr;

    mirrorShader = new QOpenGLShaderProgram;

    mirrorShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glsl/mirror.vsh");
    mirrorShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glsl/mirror.fsh");

    mirrorShader->link();

    /* Bind so we set the texture sampler uniform values. */
    mirrorShader->bind();

    /* Left image is TEXTURE0. */
    mirrorShader->setUniformValue("textureL", 0);
    /* Right image is TEXTURE1. */
    mirrorShader->setUniformValue("textureR", 1);

    vrSceneShader = new QOpenGLShaderProgram;
    vrSceneShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glsl/vrscene.vsh");
    vrSceneShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glsl/vrscene.fsh");

    vrSceneShader->link();

    /* Bind so we set the texture sampler uniform values. */
    vrSceneShader->bind();

    /* Left image is TEXTURE0. */
    vrSceneShader->setUniformValue("textureL", 0);
    /* Right image is TEXTURE1. */
    vrSceneShader->setUniformValue("textureR", 1);

    QQmlComponent component(qmlEngine);

    component.loadUrl(QUrl(QStringLiteral("qrc:/Config.qml")));

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

    screenDistanceProp = QQmlProperty(configMenu, "screenDistance");
    screenHeightProp = QQmlProperty(configMenu, "screenHeight");
    screenSizeProp = QQmlProperty(configMenu, "screenSize");
    curvedScreen = QQmlProperty(configMenu, "curvedScreen");

    qDebug("OpenVR plugin base inited.");

    return true;
}

bool OpenVRPlugin::deinit() {
    delete mirrorShader;
    delete vrSceneShader;

    delete leftEyeRenderFBO;
    delete rightEyeRenderFBO;

    vr::VR_Shutdown();

    qDebug("OpenVR shutdown.");

    return true;
}

bool OpenVRPlugin::initVR(QOpenGLFunctions* f) {
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
    leftEyeRenderFBO = new QOpenGLFramebufferObject(renderWidth, renderHeight, QOpenGLFramebufferObject::CombinedDepthStencil);
    rightEyeRenderFBO = new QOpenGLFramebufferObject(renderWidth, renderHeight, QOpenGLFramebufferObject::CombinedDepthStencil);

    qDebug("OpenVR inited.");

    return true;
}

bool OpenVRPlugin::render(const QString& drawModeName, QOpenGLFunctions* f) {
    if (vrSystem == nullptr && !initVR(f))
        return false;

    /* TODO - Use the signals of the properties to update. */
    screenDistance = screenDistanceProp.read().toFloat();
    screenHeight = screenHeightProp.read().toFloat();
    screenSize = screenSizeProp.read().toFloat();

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

//    QVector4D testV(1.0, 1.0, 0.0, 1.0);
//    testV = lEyeMat * testV;
//    qDebug() << testV;

    /* Get the size of the window. */
    GLint viewport[4];
    f->glGetIntegerv(GL_VIEWPORT, viewport);

    float height = (screenSize * viewport[3]) / viewport[2];

    /* TODO - Store this and only update when it changes... */
    QVector<QVector3D> screen;
    QVector<QVector2D> screenUV;

    if (curvedScreen.isValid() && curvedScreen.read().toBool()) {
        constexpr int halfSteps = 50;
        float theta = screenSize / screenDistance;

        /* Theta is the angle of half the screen,so this is angle of each step.  */
        float step = theta / halfSteps;

        for (int current = -halfSteps; current < halfSteps; ++current) {
            /* Calculate the coordinate of the vertex. */
            screen += {{sin(current * step) * screenDistance, screenHeight - height, cos(current * step) * -screenDistance},
                       {sin(current * step) * screenDistance, screenHeight + height, cos(current * step) * -screenDistance}};

            /* Map current from [-halfStep, halfStep] to [0, 1]. */
            float U = 0.5f * current / halfSteps + 0.5f;

            screenUV += {{U, 0.0f}, {U, 1.0f}};
        }
    } else {
        /* A simple rectangle... */
        screen += {{-screenSize, screenHeight - height, -screenDistance},  /* 2--4 */
                   {-screenSize, screenHeight + height, -screenDistance},  /* |\ | */
                   { screenSize, screenHeight - height, -screenDistance},  /* | \| */
                   { screenSize, screenHeight + height, -screenDistance}}; /* 1--3 */

        screenUV += {{0.0f, 0.0f},
                     {0.0f, 1.0f},
                     {1.0f, 0.0f},
                     {1.0f, 1.0f}};
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

    f->glFinish();
    leftEyeRenderFBO->release();

    /* Setup for the right eye. */
    rightEyeRenderFBO->bind();
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    vrSceneShader->setUniformValue("left", false);
    vrSceneShader->setUniformValue("cameraMatrix", rightEyeMat);

    /* Draw right eye to VR FBO. */
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, screen.size());

    f->glFinish();
    rightEyeRenderFBO->release();

    /* TODO - Handle distortion. */

    vr::Texture_t leftEyeTexture = { reinterpret_cast<void*>(static_cast<intptr_t>(leftEyeRenderFBO->texture())), vr::API_OpenGL, vr::ColorSpace_Gamma };
    vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);

    vr::Texture_t rightEyeTexture = { reinterpret_cast<void*>(static_cast<intptr_t>(rightEyeRenderFBO->texture())), vr::API_OpenGL, vr::ColorSpace_Gamma };
    vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);

    return true;
}

void OpenVRPlugin::frameSwapped(QOpenGLFunctions* f) {
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
