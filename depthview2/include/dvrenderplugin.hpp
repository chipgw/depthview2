#pragma once

#include <QtPlugin>

class QOpenGLExtraFunctions;
class QQmlContext;
class QQuickItem;
class DVInputInterface;
class DVRenderInterface;

class DVRenderPlugin {
public:
    virtual ~DVRenderPlugin() { }

    /* Set up the plugin and return true if it can be used. */
    virtual bool init(QOpenGLExtraFunctions* f) = 0;
    /* Delete and clean up anything used by the plugin. */
    virtual bool deinit() = 0;

    /* Return an error string describing why init(), deinit(), or render() failed. */
    virtual QString getErrorString() = 0;

    /* Render the scene using the plugin. drawModeName is one of the options returned by drawModeNames(). */
    virtual bool render(const QString& drawModeName, DVRenderInterface* renderInterface) = 0;

    /* If the plugin needs to do anything right after frame swap, do it here... */
    virtual void frameSwapped(QOpenGLExtraFunctions* f) = 0;

    /* Return an item to go inside the "Plugin Options" menu. */
    virtual QQuickItem* getConfigMenuObject() = 0;
    virtual bool initConfigMenuObject(QQmlContext* qmlContext) = 0;

    /* Return a list of names that this plugin supports. */
    virtual QStringList drawModeNames() = 0;

    /* Return true if the window should keep the mouse inside the window. */
    virtual bool shouldLockMouse() = 0;

    /* Get the size the FBO shoud be for the given window size. */
    virtual QSize getRenderSize(const QSize& windowSize) = 0;

    /* Poll any input devices tied to this plugin for input. */
    virtual bool pollInput(DVInputInterface* inputInterface) = 0;
};

#define DVRenderPlugin_iid "com.chipgw.DepthView.RenderPlugin"
Q_DECLARE_INTERFACE(DVRenderPlugin, DVRenderPlugin_iid)
