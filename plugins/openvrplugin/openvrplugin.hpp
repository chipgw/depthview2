#pragma once

#include "dvrenderplugin.hpp"
#include <QObject>
#include <QQmlProperty>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLBuffer>
#include <QSGTexture>
#include <openvr.h>

class OpenVRPlugin : public QObject, public DVRenderPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DVRenderPlugin_iid FILE "openvrplugin.json")
    Q_INTERFACES(DVRenderPlugin)

public:
    bool init(QOpenGLExtraFunctions* f);
    bool deinit();

    QString getErrorString();

    bool initVR(DVRenderInterface* renderInterface);
    void calculateEyeDistortion(vr::EVREye eye, QVector<QVector2D>& verts, QVector<GLushort>& indexes, int offset);

    bool render(const QString& drawModeName, DVRenderInterface* renderInterface);
    void renderEyeScene(vr::EVREye eye, DVRenderInterface* renderInterface, const QMatrix4x4& head, QSGTexture* imgTexture, QRectF imgRect, qreal imgPan);
    bool renderEyeDistortion(vr::EVREye eye, QOpenGLExtraFunctions* f);

    void frameSwapped(QOpenGLExtraFunctions* f);

    QStringList drawModeNames();

    QQuickItem* getConfigMenuObject();
    bool initConfigMenuObject(QQmlContext* qmlContext);

    bool shouldLockMouse();

    QSize getRenderSize(const QSize& windowSize);

    bool pollInput(DVInputInterface* inputInterface);

private:
    QOpenGLFramebufferObject* renderFBO[2];
    QOpenGLFramebufferObject* resolveFBO[2];

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
    QQuickItem* backgroundImage;

    /* Screen options */
    QQmlProperty screenSize;
    QQmlProperty screenDistance;
    QQmlProperty screenHeight;
    QQmlProperty screenCurve;
    QQmlProperty lockMouse;
    QQmlProperty mirrorUI;
    QQmlProperty snapSurroundPan;
    QQmlProperty renderSizeFac;
    QQmlProperty backgroundMode;
    QQmlProperty backgroundSwap;
    QQmlProperty backgroundPan;

    /* We keep track of the window's aspect ratio and update the screen when it changes. */
    float aspectRatio;

    QVector<QVector3D> screen;
    QVector<QVector2D> screenUV;

    QString errorString;

public slots:
    void updateScreen();
};
