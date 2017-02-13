#pragma once

#include <QtPlugin>

class QQmlContext;
class QQuickItem;
class DVInputInterface;

class DVInputPlugin {
public:
    virtual ~DVInputPlugin() { }

    /* Set up the plugin and return true if it can be used. */
    virtual bool init(QQmlContext* qmlContext) = 0;

    /* Delete and clean up anything used by the plugin. */
    virtual bool deinit() = 0;

    /* If the plugin needs to do anything right after frame swap, do it here... */
    virtual void frameSwapped() = 0;

    /* Return an item to go inside the "Plugin Options" menu. */
    virtual QQuickItem* getConfigMenuObject() = 0;

    /* Poll any input devices tied to this plugin for input. */
    virtual bool pollInput(DVInputInterface* inputInterface) = 0;
};

#define DVInputPlugin_iid "com.chipgw.DepthView.InputPlugin"
Q_DECLARE_INTERFACE(DVInputPlugin, DVInputPlugin_iid)
