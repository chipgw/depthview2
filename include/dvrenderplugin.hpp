#pragma once

#include <QtPlugin>

class QOpenGLFunctions;

class DVRenderPlugin {
public:
    virtual ~DVRenderPlugin() { }

    virtual bool init() = 0;
    virtual bool deinit() = 0;
    virtual bool render(const QString& drawModeName, QOpenGLFunctions* f) = 0;

    virtual QStringList drawModeNames() = 0;
};

#define DVRenderPlugin_iid "com.chipgw.DepthView.RenderPlugin"
Q_DECLARE_INTERFACE(DVRenderPlugin, DVRenderPlugin_iid)
