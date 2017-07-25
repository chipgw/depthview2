#pragma once

#include "dvrenderplugin.hpp"
#include <QObject>
#include <QQmlProperty>

class QOpenGLShaderProgram;

class TestPlugin : public QObject, public DVRenderPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DVRenderPlugin_iid FILE "testplugin.json")
    Q_INTERFACES(DVRenderPlugin)

    QQuickItem* configMenuObject;

    QString errorString;

public:
    bool init(QOpenGLExtraFunctions* f);
    bool deinit();

    QString getErrorString();

    bool render(const QString& drawModeName, DVRenderInterface* renderInterface);

    void frameSwapped(QOpenGLExtraFunctions* f);

    QStringList drawModeNames();

    QQuickItem* getConfigMenuObject();
    bool initConfigMenuObject(QQmlContext* qmlContext);

    bool shouldLockMouse();

    QSize getRenderSize(const QSize& windowSize);

    bool pollInput(DVInputInterface* inputInterface);

private:
    QOpenGLShaderProgram* shader;

    QQmlProperty logRenderStart;
    QQmlProperty logRenderEnd;
    QQmlProperty logFrameSwap;
    QQmlProperty lockMouse;
    QQmlProperty renderSizeFactor;
};
