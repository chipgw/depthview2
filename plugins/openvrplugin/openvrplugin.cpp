#include "openvrplugin.hpp"
#include "dvrenderinterface.hpp"
#include <QOpenGLExtraFunctions>
#include <QSGTextureProvider>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQmlContext>
#include <QDebug>
#include <cmath>

bool OpenVRPlugin::init(QOpenGLExtraFunctions*, QQmlContext* qmlContext) {
    Q_INIT_RESOURCE(openvrplugin);

    if (!vr::VR_IsHmdPresent()) {
        errorString = "No HMD detected.";
        return false;
    }

    /* These wil be inited on first usage. */
    vrSystem = nullptr;
    renderFBO[0] = renderFBO[1] = nullptr;
    resolveFBO[0] = resolveFBO[1] = nullptr;

    mirrorShader = new QOpenGLShaderProgram;

    mirrorShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/OpenVR/glsl/mirror.vsh");
    mirrorShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/OpenVR/glsl/mirror.fsh");

    mirrorShader->link();

    vrSceneShader = new QOpenGLShaderProgram;
    vrSceneShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/OpenVR/glsl/vrscene.vsh");
    vrSceneShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/OpenVR/glsl/vrscene.fsh");

    vrSceneShader->link();

    distortionShader = new QOpenGLShaderProgram;
    distortionShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/OpenVR/glsl/distortion.vsh");
    distortionShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/OpenVR/glsl/distortion.fsh");

    /* Make sure the attributes are bound correctly. */
    distortionShader->bindAttributeLocation("position",     0);
    distortionShader->bindAttributeLocation("uvRedIn",      1);
    distortionShader->bindAttributeLocation("uvGreenIn",    2);
    distortionShader->bindAttributeLocation("uvBlueIn",     3);

    distortionShader->link();

    QQmlComponent component(qmlContext->engine());

    component.loadUrl(QUrl(QStringLiteral("qrc:/OpenVR/OpenVRConfig.qml")));

    /* Wait for it to load... */
    while(component.isLoading());

    /* The program can't run if there was an error. */
    if (component.isError()) {
        errorString = component.errorString();
        return false;
    }

    configMenu = qobject_cast<QQuickItem*>(component.create(qmlContext));

    /* Critical error! abort! abort! */
    if (configMenu == nullptr) {
        errorString = "Unable to create configuration QML component.";
        return false;
    }

    QObject* obj = QQmlProperty(configMenu, "settings").read().value<QObject*>();
    screenDistance = QQmlProperty(obj, "screenDistance");
    screenHeight = QQmlProperty(obj, "screenHeight");
    screenSize = QQmlProperty(obj, "screenSize");
    screenCurve = QQmlProperty(obj, "screenCurve");
    lockMouse = QQmlProperty(obj, "lockMouse");
    renderSizeFac = QQmlProperty(obj, "renderSizeFac");
    backgroundMode = QQmlProperty(obj, "backgroundSourceMode");
    backgroundSwap = QQmlProperty(obj, "backgroundSwap");
    backgroundPan = QQmlProperty(obj, "backgroundPan");

    screenDistance.connectNotifySignal(this, SLOT(updateScreen()));
    screenHeight.connectNotifySignal(this, SLOT(updateScreen()));
    screenSize.connectNotifySignal(this, SLOT(updateScreen()));
    screenCurve.connectNotifySignal(this, SLOT(updateScreen()));

    qDebug("OpenVR plugin base inited.");

    return true;
}

bool OpenVRPlugin::deinit() {
    delete mirrorShader;
    delete vrSceneShader;
    delete distortionShader;

    delete renderFBO[0]; delete renderFBO[1];
    delete resolveFBO[0]; delete resolveFBO[1];

    vr::VR_Shutdown();

    qDebug("OpenVR shutdown.");

    return true;
}

QString OpenVRPlugin::getErrorString() {
    return errorString;
}

bool OpenVRPlugin::initVR(DVRenderInterface* renderInterface) {
    vr::EVRInitError error = vr::VRInitError_None;
    vrSystem = vr::VR_Init(&error, vr::VRApplication_Scene);

    if (error != vr::VRInitError_None) {
        vrSystem = nullptr;
        errorString = "Error initing VR system.";
        return false;
    }

    /* TODO - Actually use this. Also, support tracked controllers. (Gonna be hard, considering I have none...) */
    renderModels = (vr::IVRRenderModels*)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &error);

    if (renderModels == nullptr) {
        vrSystem = nullptr;
        errorString = "Error getting render model interface.";
        return false;
    }

    if (!vr::VRCompositor()) {
        vrSystem = nullptr;
        errorString = "Error getting compositor.";
        return false;
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

    backgroundImage = configMenu->findChild<QQuickItem*>("backgroundImage");
    /* We need to attach this to the windows root item or else the image will not be usable unless the settings window is open. */
    backgroundImage->setParentItem(renderInterface->getRootItem());

    return true;
}

void OpenVRPlugin::calculateEyeDistortion(vr::EVREye eye, QVector<QVector2D>& verts, QVector<GLushort>& indexes, int offset) {
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

bool OpenVRPlugin::render(const QString&, DVRenderInterface* renderInterface) {
    if (vrSystem == nullptr && !initVR(renderInterface))
        return false;

    vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0 );

    QOpenGLExtraFunctions* f = renderInterface->getOpenGLFunctions();

    /* Draw left eye to monitor as if in mono left mode. */
    mirrorShader->bind();
    /* TODO - This doesn't contain the current image if it's a surround image. */
    f->glBindTexture(GL_TEXTURE_2D, renderInterface->getInterfaceLeftEyeTexture());
    renderInterface->renderStandardQuad();

    QRectF currentTextureLeft, currentTextureRight;
    QSGTexture* currentTexture = nullptr;
    qreal currentTexturePan = 0;

    if (renderInterface->isSurround()) {
        currentTexture = renderInterface->getCurrentTexture(currentTextureLeft, currentTextureRight);
        currentTexturePan = renderInterface->getSurroundPan().x();
        /* Snap the pan value to multiples of 22.5 degrees to limit nausea. */
        currentTexturePan -= fmod(currentTexturePan, 22.5);
    }

    /* if the current image isn't a loaded or isn't surround, try to use the set background image. */
    if (currentTexture == nullptr && backgroundImage && backgroundImage->textureProvider() && backgroundImage->textureProvider()->texture()) {
        currentTexture = backgroundImage->textureProvider()->texture();
        currentTexturePan = backgroundPan.read().toReal();

        renderInterface->getTextureRects(currentTextureLeft, currentTextureRight, currentTexture,
                                         backgroundSwap.read().toBool(), (DVSourceMode::Type)backgroundMode.read().toInt());
    }

    /* Get the tracked position of the user's head. */
    QMatrix4x4 head;
    if (trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
        head = QMatrix4x4(QMatrix4x3(*trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m)).inverted();

    vrSceneShader->bind();
    f->glViewport(0, 0, renderWidth, renderHeight);

    /* TODO - Configurable background color. */
    f->glClearColor(0.1f, 0.1f, 0.1f, 0.0f);

    renderEyeScene(vr::Eye_Left, renderInterface, head, currentTexture, currentTextureLeft, currentTexturePan);
    renderEyeScene(vr::Eye_Right, renderInterface, head, currentTexture, currentTextureRight, currentTexturePan);

    /* Get ready to render distortion. */
    distortionShader->bind();
    distortionVBO->bind();
    distortionIBO->bind();
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

void OpenVRPlugin::renderEyeScene(vr::EVREye eye, DVRenderInterface* renderInterface, const QMatrix4x4& head, QSGTexture* imgTexture, QRectF imgRect, qreal imgPan) {
    QOpenGLExtraFunctions* f = renderInterface->getOpenGLFunctions();

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
        vrSceneShader->setUniformValue("cameraMatrix", eyeMat * sphereMat);
        vrSceneShader->setUniformValue("rect", imgRect.x(), imgRect.y(), imgRect.width(), imgRect.height());

        imgTexture->bind();

        /* Use the sphere provided by the normal surround rendering. */
        renderInterface->renderStandardSphere();

        if (renderInterface->isSurround()) {
            f->glEnable(GL_BLEND);
            f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
    }
    vrSceneShader->setUniformValue("cameraMatrix", eyeMat);

    vrSceneShader->setUniformValue("rect", 0.0f, 0.0f, 1.0f, 1.0f);

    f->glBindTexture(GL_TEXTURE_2D, renderInterface->getInterfaceLeftEyeTexture());

    /* Draw the screen to eye FBO. */
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, screen.data());
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, screenUV.data());
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, screen.size());

    renderFBO[eye]->release();
}

bool OpenVRPlugin::renderEyeDistortion(vr::EVREye eye, QOpenGLExtraFunctions *f) {
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

void OpenVRPlugin::frameSwapped(QOpenGLExtraFunctions*) {
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
    /* Check to see if the aspect ratio is the same, if not, we update the screen. */
    if (aspectRatio != float(windowSize.width()) / float(windowSize.height())) {
        aspectRatio = float(windowSize.width()) / float(windowSize.height());
        updateScreen();
    }
    qreal sizeFactor = renderSizeFac.read().toReal();
    return windowSize * (sizeFactor > 0.5 ? sizeFactor : 1.0);
}

float interpolate(float v1, float v2, float a) {
    return v1 * a + v2 * (1.0f-a);
}

void OpenVRPlugin::updateScreen() {
    /* Get the properties from QML. */
    float distance = screenDistance.read().toFloat();
    float z = screenHeight.read().toFloat();
    float size = screenSize.read().toFloat();

    float height = size / aspectRatio;

    /* Get rid of any previous geometry. */
    screen.clear();
    screenUV.clear();

    float curviness = screenCurve.read().toFloat();

    constexpr int halfSteps = 50;

    /* size/distance gives the angle of half the screen, so this is angle of each step.  */
    float step = size / (distance * halfSteps);

    for (int current = -halfSteps; current <= halfSteps; ++current) {
        /* Interpolate between the cylindrical coordinate and the flat coordinate based on curviness. */
        float xCoord = interpolate(std::sin(current * step) * distance, current * size / halfSteps, curviness);
        float yCoord = interpolate(std::cos(current * step) * -distance, -distance, curviness);

        /* Calculate the coordinate of the vertex. */
        screen += {{xCoord, z - height, yCoord}, {xCoord, z + height, yCoord}};

        /* Map current from [-halfStep, halfStep] to [0, 1]. */
        float U = 0.5f * current / halfSteps + 0.5f;

        screenUV += {{U, 0.0f}, {U, 1.0f}};
    }
}

bool OpenVRPlugin::pollInput(DVInputInterface*) {
    /* TODO - Use tracked motion controllers... */
    return false;
}
