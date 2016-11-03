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
    bool init(QOpenGLFunctions* f, QQmlEngine* qmlEngine);
    bool deinit();

    bool render(const QString& drawModeName, QOpenGLFunctions* f);

    void frameSwapped(QOpenGLFunctions* f);

    QStringList drawModeNames();

    QQuickItem* getConfigMenuObject();

private:
    QOpenGLShaderProgram* shader;

    QQmlProperty logRenderStart;
    QQmlProperty logRenderEnd;
    QQmlProperty logFrameSwap;
};
