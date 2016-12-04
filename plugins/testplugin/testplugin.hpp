#pragma once

#include "dvrenderplugin.hpp"
#include <QObject>
#include <QQmlProperty>

class QOpenGLShaderProgram;

class TestPlugin : public QObject, public DVRenderPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DVRenderPlugin_iid)
    Q_INTERFACES(DVRenderPlugin)

    QQuickItem* configMenuObject;

public:
    bool init(QOpenGLExtraFunctions* f, QQmlEngine* qmlEngine);
    bool deinit();

    bool render(const QString& drawModeName, QOpenGLExtraFunctions* f);

    void frameSwapped(QOpenGLExtraFunctions* f);

    QStringList drawModeNames();

    QQuickItem* getConfigMenuObject();

    bool shouldLockMouse();

    QSize getRenderSize(const QSize& windowSize);

    bool pollInput(DVInputInterface* inputInterface);

private:
    QOpenGLShaderProgram* shader;

    QQmlProperty logRenderStart;
    QQmlProperty logRenderEnd;
    QQmlProperty logFrameSwap;
};
