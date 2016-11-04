#pragma once

#include "dvrenderplugin.hpp"
#include <QObject>
#include <QQmlProperty>
#include <openvr.h>

class QOpenGLShaderProgram;
class QOpenGLFramebufferObject;

class OpenVRPlugin : public QObject, public DVRenderPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DVRenderPlugin_iid)
    Q_INTERFACES(DVRenderPlugin)

public:
    bool init(QOpenGLFunctions* f, QQmlEngine* qmlEngine);
    bool deinit();
    bool initVR(QOpenGLFunctions* f);

    bool render(const QString& drawModeName, QOpenGLFunctions* f);

    void frameSwapped(QOpenGLFunctions* f);

    QStringList drawModeNames();

    QQuickItem* getConfigMenuObject();

    bool shouldLockMouse();

private:
    QOpenGLFramebufferObject* leftEyeRenderFBO;
    QOpenGLFramebufferObject* leftEyeResolveFBO;
    QOpenGLFramebufferObject* rightEyeRenderFBO;
    QOpenGLFramebufferObject* rightEyeResolveFBO;

    vr::IVRSystem* vrSystem;
    vr::IVRRenderModels* renderModels;
    vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];

    uint32_t renderWidth, renderHeight;

    QOpenGLShaderProgram* mirrorShader;
    QOpenGLShaderProgram* vrSceneShader;

    QQuickItem* configMenu;

    /* Screen options */
    float screenSize;
    float screenDistance;
    float screenHeight;
    QQmlProperty screenSizeProp;
    QQmlProperty screenDistanceProp;
    QQmlProperty screenHeightProp;
    QQmlProperty curvedScreen;
};
