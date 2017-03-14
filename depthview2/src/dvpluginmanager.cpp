#include "version.hpp"
#include "dvpluginmanager.hpp"
#include "dvinputplugin.hpp"
#include "dvrenderplugin.hpp"
#include <QQuickItem>
#include <QQmlContext>
#include <QMetaObject>
#include <QApplication>
#include <QOpenGLContext>
#include <QPluginLoader>
#include <QSettings>

struct DVPluginInfo {
    QQmlContext* context = nullptr;

    DVRenderPlugin* renderPlugin = nullptr;
    DVInputPlugin* inputPlugin = nullptr;

    bool loaded = false;
    bool inited = false;
};

DVPluginManager::DVPluginManager(QObject* parent, QSettings& s) : QObject(parent), settings(s) {

}

void DVPluginManager::postQmlInit() {
    if (settings.contains("PluginMode")) {
        /* Check to make sure the plugin mode is a valid loaded plugin before setting. */
        QString mode = settings.value("PluginMode").toString();

        if (pluginModes.contains(mode))
            emit pluginModeChanged(m_pluginMode = mode);
        else
            qWarning("Invalid plugin mode \"%s\" set in settings file!", qPrintable(mode));
    }
}

void DVPluginManager::loadPlugins(QQmlEngine* engine, QOpenGLContext* context) {
    qmlEngine = engine;
    openglContext = context;

    /* Start with the path the application is in. */
    pluginsDir.cd(qApp->applicationDirPath());

    /* Plugins start with "dv2_" on Windows and "libdv2_" on Linux. */
    pluginsDir.setNameFilters(QStringList({"dv2_*", "libdv2_*"}));
    pluginsDir.setFilter(QDir::Files);

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

    /* Go into the "plugins" folder from there. */
    pluginsDir.cd("plugins");

#if defined(Q_OS_WIN)
    /* On Windows we can add the plugin dir to the DLL search path dynamically.
     * On Linux LD_LIBRARY_PATH must be set before running the program to get the same effect. */
    SetDllDirectory(pluginsDir.absolutePath().toStdWString().c_str());
#endif

    qDebug("Loading plugins from \"%s\"...", qPrintable(pluginsDir.absolutePath()));

    /* Try to load all files in the directory using the filters defined above. */
    for (const QString& filename : pluginsDir.entryList()) {
        /* If the file isn't a valid library for this platform, don't bother. */
        if (!QLibrary::isLibrary(filename)) continue;

        loadPlugin(filename);

        /* TODO - Make an interface to control which plugins are enabled and only init those. */
        initPlugin(filename);
    }
    qDebug("Done loading plugins.");
}

bool DVPluginManager::loadPlugin(const QString& pluginName) {
    DVPluginInfo* plugin = new DVPluginInfo;

    QPluginLoader loader(pluginsDir.absoluteFilePath(pluginName));
    QObject *obj = loader.instance();

    if (obj == nullptr) {
        qDebug("\"%s\" is not a plugin. %s", qPrintable(pluginName), qPrintable(loader.errorString()));
        return false;
    }

    /* If it can be cast to the plugin type it is a valid plugin, otherwise it will be null. */
    plugin->renderPlugin = qobject_cast<DVRenderPlugin*>(obj);
    plugin->inputPlugin = qobject_cast<DVInputPlugin*>(obj);

    plugin->loaded = plugin->renderPlugin != nullptr || plugin->inputPlugin != nullptr;

    if (plugin->renderPlugin != nullptr)
        qDebug("Found render plugin: \"%s\"", qPrintable(pluginName));
    else if (plugin->inputPlugin != nullptr)
        qDebug("Found input plugin: \"%s\"", qPrintable(pluginName));

    if (plugin->loaded)
        return *plugins.insert(pluginName, plugin);

    delete plugin;

    return false;
}

bool DVPluginManager::initPlugin(const QString &pluginName) {
    DVPluginInfo* plugin = plugins[pluginName];
    plugin->context = new QQmlContext(qmlEngine, this);

    if (plugin->renderPlugin != nullptr) {
        qDebug("Found render plugin: \"%s\"", qPrintable(pluginName));

        if (plugin->renderPlugin->init(openglContext->extraFunctions(), plugin->context)) {
            /* Add to the list of usable render plugins. */
            renderPlugins.append(plugin->renderPlugin);
            pluginModes.append(plugin->renderPlugin->drawModeNames());

            plugin->inited = true;
        }
    } else if (plugin->inputPlugin != nullptr) {
        qDebug("Found input plugin: \"%s\"", qPrintable(pluginName));

        if (plugin->inputPlugin->init(plugin->context)) {
            /* Add to the list of usable input plugins. */
            inputPlugins.append(plugin->inputPlugin);

            plugin->inited = true;
        }
    }

    qDebug(plugin->inited ? "Loaded plugin: \"%s\"" : "Plugin: \"%s\" failed to init.", qPrintable(pluginName));
    return plugin->inited;
}

void DVPluginManager::unloadPlugins() {
    /* Deinit any/all loaded plugins. */
    for (DVRenderPlugin* plugin : renderPlugins)
        plugin->deinit();
    for (DVInputPlugin* plugin : inputPlugins)
        plugin->deinit();

    /* Clear the list. Not that it should be used anymore... */
    renderPlugins.clear();
    inputPlugins.clear();
}

void DVPluginManager::savePluginSettings(QString pluginTitle, QObject* settingsObject) {
    /* Remove spaces from the plugin title. */
    pluginTitle.remove(' ');

    settings.beginGroup(pluginTitle);

    /* Go through all properties of the item excluding the first one, which is the objectName property of QObject. */
    for (int i = 1; i < settingsObject->metaObject()->propertyCount(); ++i)
        settings.setValue(settingsObject->metaObject()->property(i).name(), settingsObject->metaObject()->property(i).read(settingsObject));

    settings.endGroup();
}

void DVPluginManager::loadPluginSettings(QString pluginTitle, QObject* settingsObject) {
    /* Remove spaces from the plugin title. */
    pluginTitle.remove(' ');

    /* If the group doesn't already exist, do nothing. */
    if (!settings.childGroups().contains(pluginTitle))
        return;

    settings.beginGroup(pluginTitle);

    /* Go through each setting in the group. */
    for (const QString& key : settings.allKeys())
        settingsObject->setProperty(key.toLocal8Bit().data(), settings.value(key));

    settings.endGroup();
}

DVRenderPlugin* DVPluginManager::getCurrentRenderPlugin() const {
    for (DVRenderPlugin* plugin : renderPlugins)
        /* Find the first plugin that contains the mode we want. */
        if (plugin->drawModeNames().contains(m_pluginMode))
            return plugin;

    return nullptr;
}

bool DVPluginManager::doPluginRender() {
    DVRenderPlugin* plugin = getCurrentRenderPlugin();

    /* Return false if it wasn't found, otherwise let it do its thing. */
    return plugin != nullptr && plugin->render(m_pluginMode, openglContext->extraFunctions());
}

QSize DVPluginManager::getPluginSize(QSize inputSize) {
    DVRenderPlugin* plugin = getCurrentRenderPlugin();

    if (plugin != nullptr)
        return plugin->getRenderSize(inputSize);

    return inputSize;
}

bool DVPluginManager::onFrameSwapped() {
    DVRenderPlugin* plugin = getCurrentRenderPlugin();

    if (plugin != nullptr) {
        /* Let it do its thing. */
        plugin->frameSwapped(openglContext->extraFunctions());

        /* Do we hold the mouse? */
        return plugin->shouldLockMouse();
    }

    return false;
}

void DVPluginManager::doPluginInput(DVInputInterface* inputInterface) {
    /* Get input from ALL input plugins. */
    for (DVInputPlugin* plugin : inputPlugins)
        plugin->pollInput(inputInterface);

    /* Only get input from the current render plugin. */
    DVRenderPlugin* plugin = getCurrentRenderPlugin();

    if (plugin != nullptr)
        plugin->pollInput(inputInterface);
}

QString DVPluginManager::pluginMode() const {
    return m_pluginMode;
}

void DVPluginManager::setPluginMode(const QString& mode) {
    /* Only set if valid. */
    if (mode != m_pluginMode && pluginModes.contains(mode)) {
        m_pluginMode = mode;
        settings.setValue("PluginMode", mode);
        emit pluginModeChanged(mode);
    }
}

QStringList DVPluginManager::getModes() const {
    return QStringList() << "Anaglyph"
                         << "Side-by-Side"
                         << "Top/Bottom"
                         << "Interlaced Horizontal"
                         << "Interlaced Vertical"
                         << "Checkerboard"
                         << "Mono"
                         << pluginModes;
}

QStringList DVPluginManager::getPluginModes() const {
    return pluginModes;
}

QObject *DVPluginManager::getPluginConfigMenu() const {
    DVRenderPlugin* plugin = getCurrentRenderPlugin();
    return plugin != nullptr ? plugin->getConfigMenuObject() : nullptr;
}

QObjectList DVPluginManager::getPluginConfigMenus() const {
    QObjectList list;

    for (DVRenderPlugin* item : renderPlugins)
        list.append((QObject*)item->getConfigMenuObject());
    for (DVInputPlugin* item : inputPlugins)
        list.append((QObject*)item->getConfigMenuObject());

    return list;
}
