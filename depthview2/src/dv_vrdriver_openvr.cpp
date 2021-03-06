#include "dv_vrdriver_openvr.hpp"

DV_VRDriver_OpenVR::DV_VRDriver_OpenVR(DVRenderer* w, DVVirtualScreenManager* m) : DV_VRDriver(w, m) {
    if (!vr::VR_IsHmdPresent()) {
        setError("No HMD detected.");
        return;
    }

    vrSceneShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glsl/openvrscene.vsh");
    vrSceneShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glsl/openvrscene.fsh");

    vrSceneShader.link();

    lineTexture = new QOpenGLTexture(QImage(":/images/vrline.png"));

    qDebug("OpenVR inited.");
}

DV_VRDriver_OpenVR::~DV_VRDriver_OpenVR() {
    vr::VR_Shutdown();

    /* None of these would be inited if the vr system is invaid. */
    if (vrSystem == nullptr) return;

    delete renderFBO[0]; delete renderFBO[1];

    /* Delete all loaded render models. */
    for (ModelComponent* m : loadedComponents) delete m;

    qDebug("OpenVR shutdown.");
}

bool DV_VRDriver_OpenVR::initVRSystem(QOpenGLExtraFunctions* f) {
    vr::EVRInitError error = vr::VRInitError_None;
    vrSystem = vr::VR_Init(&error, vr::VRApplication_Scene);

    if (error != vr::VRInitError_None) {
        vrSystem = nullptr;
        return setError("Error initing VR system.");
    }

    if (!vr::VRCompositor()) {
        vrSystem = nullptr;
        return setError("Error getting compositor.");
    }

    /* Get the size for the FBOs. */
    vrSystem->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);

    /* Set up an FBO for each eye. */
    renderFBO[0] = new QOpenGLFramebufferObject(int(renderWidth), int(renderHeight), QOpenGLFramebufferObject::Depth);
    renderFBO[1] = new QOpenGLFramebufferObject(int(renderWidth), int(renderHeight), QOpenGLFramebufferObject::Depth);

    eyeTextures[vr::Eye_Left] = {
        reinterpret_cast<void*>(static_cast<intptr_t>(renderFBO[vr::Eye_Left]->texture())),
        vr::TextureType_OpenGL,
        vr::ColorSpace_Gamma
    };
    eyeTextures[vr::Eye_Right] = {
        reinterpret_cast<void*>(static_cast<intptr_t>(renderFBO[vr::Eye_Right]->texture())),
        vr::TextureType_OpenGL,
        vr::ColorSpace_Gamma
    };

    QVector<QVector2D> verts;
    QVector<GLushort> indexes;

    /* Load the models for attached devices. */
    for (uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i) setupDeviceModel(i, f);

    /* Set up the input actions. */
    vr::VRInput()->SetActionManifestPath((qApp->applicationDirPath() + "/openvr_actionmanifest.json").toLocal8Bit());

    vr::VRInput()->GetActionSetHandle("/actions/image", &inputActionSets[DVInputMode::ImageViewer]);
    vr::VRInput()->GetActionSetHandle("/actions/video", &inputActionSets[DVInputMode::VideoPlayer]);
    vr::VRInput()->GetActionSetHandle("/actions/file",  &inputActionSets[DVInputMode::FileBrowser]);

    inputActions[DVInputMode::ImageViewer] += DigitalAction("/actions/image/in/zoomfit",    &DVInputInterface::zoomFit);
    inputActions[DVInputMode::ImageViewer] += DigitalAction("/actions/image/in/zoomactual", &DVInputInterface::zoomActual);
    inputActions[DVInputMode::ImageViewer] += DigitalAction("/actions/image/in/browser",    &DVInputInterface::openFileBrowser);
    inputActions[DVInputMode::ImageViewer] += DigitalAction("/actions/image/in/fileinfo",   &DVInputInterface::fileInfo);
    inputActions[DVInputMode::ImageViewer] += DigitalAction("/actions/image/in/next",       &DVInputInterface::nextFile);
    inputActions[DVInputMode::ImageViewer] += DigitalAction("/actions/image/in/prev",       &DVInputInterface::previousFile);

    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/playpause",  &DVInputInterface::playPauseVideo);
    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/play",       &DVInputInterface::playVideo);
    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/pause",      &DVInputInterface::pauseVideo);
    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/mute",       &DVInputInterface::mute);
    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/volup",      &DVInputInterface::volumeUp);
    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/voldown",    &DVInputInterface::volumeDown);
    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/seekfwd",    &DVInputInterface::seekForward);
    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/seekback",   &DVInputInterface::seekBack);
    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/takesnap",   &DVInputInterface::takeSnapshot);
    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/browser",    &DVInputInterface::openFileBrowser);
    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/fileinfo",   &DVInputInterface::fileInfo);
    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/next",       &DVInputInterface::nextFile);
    inputActions[DVInputMode::VideoPlayer] += DigitalAction("/actions/video/in/prev",       &DVInputInterface::previousFile);

    inputActions[DVInputMode::FileBrowser] += DigitalAction("/actions/file/in/forward",     &DVInputInterface::goForward);
    inputActions[DVInputMode::FileBrowser] += DigitalAction("/actions/file/in/back",        &DVInputInterface::goBack);
    inputActions[DVInputMode::FileBrowser] += DigitalAction("/actions/file/in/goup",        &DVInputInterface::goUp);
    inputActions[DVInputMode::FileBrowser] += DigitalAction("/actions/file/in/accept",      &DVInputInterface::accept);
    inputActions[DVInputMode::FileBrowser] += DigitalAction("/actions/file/in/cancel",      &DVInputInterface::cancel);
    inputActions[DVInputMode::FileBrowser] += DigitalAction("/actions/file/in/left",        &DVInputInterface::left);
    inputActions[DVInputMode::FileBrowser] += DigitalAction("/actions/file/in/right",       &DVInputInterface::right);
    inputActions[DVInputMode::FileBrowser] += DigitalAction("/actions/file/in/up",          &DVInputInterface::up);
    inputActions[DVInputMode::FileBrowser] += DigitalAction("/actions/file/in/down",        &DVInputInterface::down);

    vr::VRInput()->GetActionSetHandle("/actions/cursor", &mouseActionSet);
    mouseClickAction = DigitalAction("/actions/cursor/in/leftbtn");
    mousePanAction = DigitalAction("/actions/cursor/in/pan");
    mousePose = PoseAction("/actions/cursor/in/point");

    return vrSystem != nullptr;
}

DV_VRDriver::RayHit DV_VRDriver_OpenVR::poseScreenPoint(vr::TrackedDevicePose_t pose) {
    /* The tracking data isn't valid, we can't do anything. */
    if (!pose.bPoseIsValid) return RayHit();

    QMatrix4x4 deviceToTracking = QMatrix4x4(QMatrix4x3(*pose.mDeviceToAbsoluteTracking.m));
    QMatrix4x4 aimToTracking = deviceToTracking ;//* getComponentMatrix(dev, vr::k_pch_Controller_Component_Tip, false);

    Ray ray;
    ray.origin = aimToTracking * QVector3D();
    ray.direction = (aimToTracking * QVector4D(0.0f, 0.0f, -0.1f, 0.0f)).toVector3D();

    return screenTrace(ray);
}

void DV_VRDriver_OpenVR::sendMousePress(const QPointF& point, Qt::MouseButton button, DVInputInterface* input) {
    /* Don't press if already down. */
    if (mouseButtonsDown.testFlag(button)) return;

    mouseButtonsDown |= button;

    QCoreApplication::postEvent(input->inputEventObject(), new QMouseEvent(QEvent::MouseButtonPress, point, point, QPointF(),
                                                                           button, Qt::LeftButton, nullptr, Qt::MouseEventSynthesizedByApplication));
}

void DV_VRDriver_OpenVR::sendMouseRelease(const QPointF& point, Qt::MouseButton button, DVInputInterface* input) {
    /* Don't release if not down. */
    if (!mouseButtonsDown.testFlag(button)) return;

    mouseButtonsDown ^= button;
    QCoreApplication::postEvent(input->inputEventObject(), new QMouseEvent(QEvent::MouseButtonRelease, point, point, QPointF(),
                                                                           button, mouseButtonsDown, nullptr, Qt::MouseEventSynthesizedByApplication));
}

void DV_VRDriver_OpenVR::sendMouseMove(const QPointF& point, DVInputInterface* input) {
    QCoreApplication::postEvent(input->inputEventObject(), new QMouseEvent(QEvent::MouseMove, point, point, QPointF(),
                                                                           Qt::NoButton, mouseButtonsDown, nullptr, Qt::MouseEventSynthesizedByApplication));
}

void DV_VRDriver_OpenVR::handleVREvents(QOpenGLExtraFunctions* f, DVInputInterface* input) {
    vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

    vr::VRActiveActionSet_t active[2] {
        { inputActionSets[input->inputMode()], vr::k_ulInvalidInputValueHandle, 0 },
        { mouseActionSet, vr::k_ulInvalidInputValueHandle, 0 },
    };
    vr::VRInput()->UpdateActionState(active, sizeof(vr::VRActiveActionSet_t), 2);

    for (auto& action : inputActions[input->inputMode()]) {
        action.update(input);
    }
    mouseClickAction.update(input);
    mousePanAction.update(input);
    mousePose.update();

    RayHit mouseHit = poseScreenPoint(mousePose.current.pose);
    QPointF mousePoint = mouseHit.isValid ? manager->pointFromScreenUV(mouseHit.uvCoord) : QPointF();

    if (mouseHit.isValid)
        sendMouseMove(mousePoint, input);
    else if (wasLastHitValid) {
        /* If the mouse has left the screen, send a release event for every pressed button. */
        for (int i = 1; i < Qt::MaxMouseButton; i *= 2)
            sendMouseRelease(mousePoint, Qt::MouseButton(i), input);

        /* Move the cursor off the screen so it won't be visible nor will it keep the top or bottom menu open. */
        sendMouseMove(manager->pointFromScreenUV({-0.5f, 0.5f}), input);
    }

    if (!panTrackingVector.isNull()) {
        /* Get the angle between the direction vector between last frame and this frame, ignoring the z axis. */
        qreal angleDelta = qRadiansToDegrees(qAtan2(panTrackingVector.z(), panTrackingVector.x())
                                             - qAtan2(mouseHit.ray.direction.z(), mouseHit.ray.direction.x()));

        manager->setSurroundPan(manager->surroundPan() + angleDelta);

        /* Store the new direction vector for next frame. */
        panTrackingVector = mouseHit.ray.direction;
    }

    if (mouseClickAction.current.bChanged && mouseHit.isValid) {
        if  (mouseClickAction.current.bState)
            sendMousePress(mousePoint, Qt::LeftButton, input);
        else
            sendMouseRelease(mousePoint, Qt::LeftButton, input);
    }

    if (mousePanAction.current.bChanged) {
        if (mousePanAction.current.bState && input->inputMode() != DVInputMode::FileBrowser)
            panTrackingVector = mouseHit.ray.direction;
        else
            /* Set it to a null vector to stop panning. */
            panTrackingVector = QVector3D();
    }

    /* Handle events from OpenVR. */
    vr::VREvent_t e;
    while (vrSystem->PollNextEvent(&e, sizeof(e))) {
        switch (e.eventType) {
        case vr::VREvent_TrackedDeviceActivated:
            /* Load the model for the newly attached device. */
            setupDeviceModel(e.trackedDeviceIndex, f);
            break;
        }
    }

    wasLastHitValid = mouseHit.isValid;
}

QMatrix4x4 DV_VRDriver_OpenVR::getComponentMatrix(uint32_t device, const char* componentName, bool render) {
    /* I'm making assumptions here... */
    const vr::RenderModel_ControllerMode_State_t renderModelState = { false };

    /* This is where we get the render model system to put the component matrix. */
    vr::RenderModel_ComponentState_t componentState;

    /* Get the component state information from OpenVR. */
    vr::VRRenderModels()->GetComponentState(modelForDevice[device].data(), componentName, &controllerStates[device], &renderModelState, &componentState);

    return QMatrix4x4(QMatrix4x3(render ? *componentState.mTrackingToComponentRenderModel.m : *componentState.mTrackingToComponentLocal.m));
}

void DV_VRDriver_OpenVR::renderEyeScene(vr::EVREye eye, const QMatrix4x4& head, QSGTexture* imgTexture, QRectF imgRect, qreal imgPan, bool isBackground, QOpenGLExtraFunctions* f) {
    /* A matrix for each eye, to tell where it is relative to the user's head. */
    const vr::HmdMatrix34_t& eyeMatrix = vrSystem->GetEyeToHeadTransform(eye);

    /* Get a projection matrix for each eye. */
    const vr::HmdMatrix44_t& eyeProj = vrSystem->GetProjectionMatrix(eye, 0.1f, 200.0f);

    /* Convert them all to QMatrix4x4 and combine them. */
    QMatrix4x4 eyeMat = QMatrix4x4(*eyeProj.m) * QMatrix4x4(QMatrix4x3(*eyeMatrix.m)).inverted() * head;

    /* Setup for the eye. */
    renderFBO[eye]->bind();
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (imgTexture != nullptr) {
        QMatrix4x4 sphereMat;
        sphereMat.scale(190.0f);
        sphereMat.rotate(float(imgPan), 0.0f, 1.0f, 0.0f);
        vrSceneShader.setUniformValue("cameraMatrix", eyeMat * sphereMat);
        vrSceneShader.setUniformValue("rect", imgRect.x(), imgRect.y(), imgRect.width(), imgRect.height());

        imgTexture->bind();

        if (isBackground)
            vrSceneShader.setUniformValue("outputFac", float(1.0 - backgroundDim));

        /* Use the sphere provided by the normal surround rendering. */
        renderer->renderStandardSphere();

        /* Screen is opaque for backgrounds, but it is transparent for open images. */
        if (!isBackground)
            f->glEnable(GL_BLEND);
    }
    vrSceneShader.setUniformValue("cameraMatrix", eyeMat);
    vrSceneShader.setUniformValue("rect", 0.0f, 0.0f, 1.0f, 1.0f);
    vrSceneShader.setUniformValue("outputFac", 1.0f);

    /* Get the UI texture for the current eye from the renderer. In both eye enums left=0 and right=1. */
    f->glBindTexture(GL_TEXTURE_2D, renderer->getInterfaceTexture(static_cast<DVStereoEye::Type>(eye)));

    /* Draw the screen to eye FBO. */
    vrSceneShader.setAttributeArray(0, screen.data());
    vrSceneShader.setAttributeArray(1, screenUV.data());
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, screen.size());

    /* Don't use blending for tracked models. */
    f->glDisable(GL_BLEND);

    /* Draw a line from the mouse controller to the screen where it's aiming. */
    if (mousePose.current.pose.bPoseIsValid) {
        QMatrix4x4 point_mat = QMatrix4x4(QMatrix4x3(*mousePose.current.pose.mDeviceToAbsoluteTracking.m));

        Ray ray;
        ray.origin = point_mat * QVector3D();
        ray.direction = (point_mat * QVector4D(0.f, 0.f, -1.f, 0.f)).toVector3D();

        const RayHit hit = screenTrace(ray);

        /* Line is rendered in world space. */
        vrSceneShader.setUniformValue("cameraMatrix", eyeMat);

        /* If the hit isn't valid we just draw a line one unit out in the aim direction. */
        QVector3D line[] = { ray.origin, hit.isValid ? hit.hitPoint : (ray.origin + ray.direction) };
        QVector2D lineUV[] = { QVector2D(0.0f, 0.0f), QVector2D(1.0f, 1.0f) };

        vrSceneShader.setAttributeArray(0, line);
        vrSceneShader.setAttributeArray(1, lineUV);

        lineTexture->bind();

        f->glDrawArrays(GL_LINES, 0, sizeof(line) / sizeof(*line));
    }

    for (vr::TrackedDeviceIndex_t device = 0; device < vr::k_unMaxTrackedDeviceCount; ++device) {
        /* Only render valid devices of the controller class. */
        if (!renderModels.contains(modelForDevice[device]) || vrSystem->GetTrackedDeviceClass(device) != vr::TrackedDeviceClass_Controller)
            continue;

        const vr::TrackedDevicePose_t& pose = trackedDevicePose[device];
        /* Don't render when the tracking data isn't valid. */
        if (!pose.bPoseIsValid) continue;

        QMatrix4x4 deviceToTracking = QMatrix4x4(QMatrix4x3(*pose.mDeviceToAbsoluteTracking.m));
        QMatrix4x4 deviceToEye = eyeMat * deviceToTracking;

        const auto& componentsByName = renderModels[modelForDevice[device]];

        /* Go through each component to render it. */
        for (auto component = componentsByName.cbegin(), end = componentsByName.cend(); component != end; ++component) {
            vrSceneShader.setUniformValue("cameraMatrix", deviceToEye * getComponentMatrix(device, component.key().data()));

            component.value()->VBO.bind();
            component.value()->IBO.bind();

            /* This model also has normals, but there isn't any lighting so they're unnecessary. */
            vrSceneShader.setAttributeBuffer(0, GL_FLOAT, offset_of(&vr::RenderModel_Vertex_t::vPosition), 3, sizeof(vr::RenderModel_Vertex_t));
            vrSceneShader.setAttributeBuffer(1, GL_FLOAT, offset_of(&vr::RenderModel_Vertex_t::rfTextureCoord), 2, sizeof(vr::RenderModel_Vertex_t));

            component.value()->texture.bind();

            f->glDrawElements(GL_TRIANGLES, component.value()->vertexCount, GL_UNSIGNED_SHORT, nullptr);

            component.value()->VBO.release();
            component.value()->IBO.release();
        }
    }

    renderFBO[eye]->release();
}

bool DV_VRDriver_OpenVR::render(QOpenGLExtraFunctions* f, DVInputInterface* input) {
    /* Init VR system on first use. We can't render if VR doesn't init correctly. */
    if (vrSystem == nullptr && !initVRSystem(f)) return false;

    handleVREvents(f, input);

    f->glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
    f->glEnable(GL_DEPTH_TEST);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    QRectF currentTextureLeft, currentTextureRight;
    QSGTexture* currentTexture = nullptr;
    qreal currentTexturePan = 0;
    /* Whether the value of currentTexture is the background image (true) or an opened image (false). */
    bool isBackground = false;

    if (manager->isCurrentFileSurround()) {
        currentTexture = renderer->getCurrentTexture(currentTextureLeft, currentTextureRight);
        currentTexturePan = manager->surroundPan();

        if (snapSurroundPan)
            /* Snap the pan value to multiples of 22.5 degrees to limit nausea. */
            currentTexturePan -= fmod(currentTexturePan, 22.5);
    }

    /* if the current image isn't a loaded or isn't surround, try to use the set background image. */
    if (currentTexture == nullptr && backgroundImageItem && backgroundImageItem->textureProvider()) {
        currentTexture = backgroundImageItem->textureProvider()->texture();
        currentTexturePan = backgroundPan;

        renderer->getTextureRects(currentTextureLeft, currentTextureRight, currentTexture, backgroundSwap, backgroundSourceMode);

        isBackground = true;
    }

    /* Get the tracked position of the user's head. */
    QMatrix4x4 head;
    if (trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
        head = QMatrix4x4(QMatrix4x3(*trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking.m)).inverted();

    vrSceneShader.bind();
    f->glViewport(0, 0, GLsizei(renderWidth), GLsizei(renderHeight));

    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);
    renderEyeScene(vr::Eye_Left, head, currentTexture, currentTextureLeft, currentTexturePan, isBackground, f);
    renderEyeScene(vr::Eye_Right, head, currentTexture, currentTextureRight, currentTexturePan, isBackground, f);

    /* Submit the textures to OpenVR. */
    if (vr::VRCompositor()->Submit(vr::Eye_Left, &eyeTextures[vr::Eye_Left]) != vr::VRCompositorError_None)
        return setError("Error submitting texture to OpenVR.");
    if (vr::VRCompositor()->Submit(vr::Eye_Right, &eyeTextures[vr::Eye_Right]) != vr::VRCompositorError_None)
        return setError("Error submitting texture to OpenVR.");

    return true;
}

void DV_VRDriver_OpenVR::frameSwapped() {
    if (vrSystem != nullptr)
        vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
}

/* Get a tracked device property string. */
QByteArray DV_VRDriver_OpenVR::getTrackedDeviceString(vr::TrackedDeviceIndex_t deviceIndex, vr::TrackedDeviceProperty prop) {
    uint32_t bufferLen = vrSystem->GetStringTrackedDeviceProperty(deviceIndex, prop, nullptr, 0, nullptr);

    if (bufferLen == 0) return "";

    QByteArray buffer(int(bufferLen), 0);
    vrSystem->GetStringTrackedDeviceProperty(deviceIndex, prop, buffer.data(), uint32_t(buffer.length()), nullptr);

    return buffer;
}

void DV_VRDriver_OpenVR::setupDeviceModel(vr::TrackedDeviceIndex_t deviceIndex, QOpenGLExtraFunctions* f) {
    if (deviceIndex >= vr::k_unMaxTrackedDeviceCount || vrSystem->GetTrackedDeviceClass(deviceIndex) != vr::TrackedDeviceClass_Controller)
        return;

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
        QByteArray componentName(int(vr::VRRenderModels()->GetComponentName(modelName.data(), i, nullptr, 0)), 0);
        vr::VRRenderModels()->GetComponentName(modelName.data(), i, componentName.data(), uint32_t(componentName.length()));

        /* The component model name is not the same as the component name, it is used to get the model data and to save it in the loadedModels map. */
        QByteArray componentModelName(int(vr::VRRenderModels()->GetComponentRenderModelName(modelName.data(), componentName.data(), nullptr, 0)), 0);
        vr::VRRenderModels()->GetComponentRenderModelName(modelName.data(), componentName.data(), componentModelName.data(), uint32_t(componentModelName.length()));

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
            QThread::msleep(20);
        } while (error == vr::VRRenderModelError_Loading);

        if (error != vr::VRRenderModelError_None) {
            qDebug("Failed to load component %s for render model %s for device %d - %s", modelName.data(), componentModelName.data(), deviceIndex,
                   vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error));
            continue;
        }

        /* Wait for the texture to load. */
        do {
            error = vr::VRRenderModels()->LoadTexture_Async(model->diffuseTextureId, &texture);
            QThread::msleep(20);
        } while (error == vr::VRRenderModelError_Loading);

        if (error != vr::VRRenderModelError_None) {
            qDebug("Failed to load texture %d for render model %s", model->diffuseTextureId, componentModelName.data());
            vr::VRRenderModels()->FreeRenderModel(model);
            continue;
        }

        /* Create the OpenGL buffers from the loaded data. and save it both in the current model and the list of all loaded components. */
        componentMap[componentName] = loadedComponents[componentModelName] = new ModelComponent(*model, *texture, f);

        /* We don't need the model data any more, it has been uploaded to the GPU. */
        vr::VRRenderModels()->FreeRenderModel(model); vr::VRRenderModels()->FreeTexture(texture);
    }
}

DV_VRDriver* DV_VRDriver::createOpenVRDriver(DVRenderer* renderer, DVVirtualScreenManager* manager) {
    return new DV_VRDriver_OpenVR(renderer, manager);
}
