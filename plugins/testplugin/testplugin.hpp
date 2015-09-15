#pragma once

#include "dvrenderplugin.hpp"
#include <QObject>

class QOpenGLShaderProgram;

class TestPlugin : public QObject, public DVRenderPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DVRenderPlugin_iid)
    Q_INTERFACES(DVRenderPlugin)

public:
    bool init();
    bool deinit();

    bool render(const QString &drawModeName, QOpenGLFunctions *f);

    QStringList drawModeNames();

private:
    QOpenGLShaderProgram* shader;
};
