#pragma once

#include "dvrenderplugin.hpp"
#include <QObject>
#include <QQmlProperty>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <openvr.h>

class QOpenGLShaderProgram;
class QOpenGLBuffer;
class QOpenGLFramebufferObject;

class OpenVRPlugin : public QObject, public DVRenderPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DVRenderPlugin_iid FILE "openvrplugin.json")
    Q_INTERFACES(DVRenderPlugin)

public:
    bool init(QOpenGLExtraFunctions* f, QQmlContext* qmlContext);
    bool deinit();
    bool initVR();

    bool render(const QString& drawModeName, QOpenGLExtraFunctions* f);

    void frameSwapped(QOpenGLExtraFunctions* f);

    QStringList drawModeNames();

    QQuickItem* getConfigMenuObject();

    bool shouldLockMouse();

    QSize getRenderSize(const QSize& windowSize);

    bool pollInput(DVInputInterface* inputInterface);

private:
    QOpenGLFramebufferObject* leftEyeRenderFBO;
    QOpenGLFramebufferObject* leftEyeResolveFBO;
    QOpenGLFramebufferObject* rightEyeRenderFBO;
    QOpenGLFramebufferObject* rightEyeResolveFBO;

    QOpenGLBuffer* distortionVBO;
    QOpenGLBuffer* distortionIBO;
    intptr_t distortionNumIndexes;

    vr::IVRSystem* vrSystem;
    vr::IVRRenderModels* renderModels;
    vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];

    uint32_t renderWidth, renderHeight;

    QOpenGLShaderProgram* mirrorShader;
    QOpenGLShaderProgram* vrSceneShader;
    QOpenGLShaderProgram* distortionShader;

    QQuickItem* configMenu;

    /* Screen options */
    QQmlProperty screenSize;
    QQmlProperty screenDistance;
    QQmlProperty screenHeight;
    QQmlProperty screenCurve;
    QQmlProperty lockMouse;
    QQmlProperty renderSizeFac;

    /* We keep track of the window's aspect ratio and update the screen when it changes. */
    float aspectRatio;

    QVector<QVector3D> screen;
    QVector<QVector2D> screenUV;

public slots:
    void updateScreen();
};
