#include "version.hpp"
#include "dvwindow.hpp"
#include "dvqmlcommunication.hpp"
#include "dvrenderplugin.hpp"
#include <QApplication>
#include <QOpenGLContext>
#include <QPluginLoader>
#include <QDir>

void DVWindow::loadPlugins() {
    /* Start with the path the application is in. */
    QDir pluginsDir(qApp->applicationDirPath());

#if defined(Q_OS_WIN)
    /* If we're in a "debug" or "release" folder go up a level, because that's where plugins are copied by the build system. */
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    /* I haven't actually tried this on Mac. This is just what the Qt plugin example said to do... */
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif

    /* Go into thhe "plugins" folder from there. */
    pluginsDir.cd("plugins");

#if defined(Q_OS_WIN)
    /* On Windows we can add the plugin dir to the DLL search path dynamically.
     * On Linux LD_LIBRARY_PATH must be set before running the program to get the same effect. */
    SetDllDirectory(pluginsDir.absolutePath().toStdWString().c_str());
#endif

    qDebug("Loading plugins from \"%s\"...", qPrintable(pluginsDir.absolutePath()));

    /* Try to load all files in the directory. */
    for (const QString& filename : pluginsDir.entryList(QDir::Files)) {
        QPluginLoader loader(pluginsDir.absoluteFilePath(filename));
        QObject *obj = loader.instance();
        DVRenderPlugin* plugin = nullptr;

        /* If it can't be cast to a DVRenderPlugin* it isn't a valid plugin. */
        if (obj != nullptr && (plugin = qobject_cast<DVRenderPlugin*>(obj)) != nullptr) {
            qDebug("Found plugin: \"%s\"", qPrintable(filename));

            if (plugin->init(context()->extraFunctions(), qmlEngine)) {
                for (const QString& mode : plugin->drawModeNames())
                    qmlCommunication->addPluginMode(mode, plugin->getConfigMenuObject());

                renderPlugins.append(plugin);

                qDebug("Loaded plugin: \"%s\"", qPrintable(filename));
            } else {
                qDebug("Plugin: \"%s\" failed to init.", qPrintable(filename));
            }
        } else {
            qDebug("\"%s\" is not a plugin. %s", qPrintable(filename), qPrintable(loader.errorString()));
        }
    }
    qDebug("Done loading plugins.");
}

void DVWindow::unloadPlugins() {
    /* Deinit any/all loaded plugins. */
    for (DVRenderPlugin* plugin : renderPlugins)
        plugin->deinit();

    /* Clear the list. Not that it should be used anymore... */
    renderPlugins.clear();
}

DVRenderPlugin* DVWindow::getCurrentRenderPlugin() {
    for (DVRenderPlugin* plugin : renderPlugins)
        /* Find the first plugin that contains the mode we want. */
        if (plugin->drawModeNames().contains(qmlCommunication->pluginMode()))
            return plugin;

    return nullptr;
}

bool DVWindow::doPluginRender() {
    DVRenderPlugin* plugin = getCurrentRenderPlugin();

    /* Return false if it wasn't found, otherwise let it do its thing. */
    return plugin != nullptr && plugin->render(qmlCommunication->pluginMode(), context()->extraFunctions());
}

void DVWindow::getPluginSize() {
    DVRenderPlugin* plugin = getCurrentRenderPlugin();

    if (plugin != nullptr)
        qmlSize = plugin->getRenderSize(qmlSize);
}

void DVWindow::pluginOnFrameSwapped() {
    DVRenderPlugin* plugin = getCurrentRenderPlugin();

    if (plugin != nullptr) {
        /* Let it do its thing. */
        plugin->frameSwapped(context()->extraFunctions());

        /* Do we hold the mouse? */
        holdMouse = plugin->shouldLockMouse();
    }
}
