#pragma once

#include <QtPlugin>

class QOpenGLFunctions;
class QQmlEngine;
class QQuickItem;

class DVRenderPlugin {
public:
    virtual ~DVRenderPlugin() { }

    /* Set up the plugin and return true if it can be used. */
    virtual bool init(QOpenGLFunctions* f, QQmlEngine* qmlEngine) = 0;
    /* Delete and clean up anything used by the plugin. */
    virtual bool deinit() = 0;

    /* Render the scene using the plugin. drawModeName is one of the options returned by drawModeNames().
     * The left and right image textures are bound to TEXTURE0 and TEXTURE1, respectively. */
    virtual bool render(const QString& drawModeName, QOpenGLFunctions* f) = 0;

    /* If the plugin needs to do anything right after frame swap, do it here... */
    virtual void frameSwapped(QOpenGLFunctions* f) = 0;

    /* Return an item to go inside the "Plugin Options" menu. */
    virtual QQuickItem* getConfigMenuObject() = 0;

    /* Return a list of names that this plugin supports. */
    virtual QStringList drawModeNames() = 0;
};

#define DVRenderPlugin_iid "com.chipgw.DepthView.RenderPlugin"
Q_DECLARE_INTERFACE(DVRenderPlugin, DVRenderPlugin_iid)
