#pragma once

#include "dvrenderplugin.hpp"
#include <QObject>

class QOpenGLShaderProgram;

class TestPlugin : public QObject, public DVRenderPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DVRenderPlugin_iid)
    Q_INTERFACES(DVRenderPlugin)

public:
    bool init(QOpenGLFunctions* f);
    bool deinit();

    bool render(const QString& drawModeName, QOpenGLFunctions* f);

    void frameSwapped(QOpenGLFunctions* f);

    QStringList drawModeNames();

private:
    QOpenGLShaderProgram* shader;
};
