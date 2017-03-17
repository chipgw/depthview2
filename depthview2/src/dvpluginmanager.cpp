#include "version.hpp"
#include "dvpluginmanager.hpp"
#include "dvinputplugin.hpp"
#include "dvrenderplugin.hpp"
#include "dvenums.hpp"
#include <QQuickItem>
#include <QQmlContext>
#include <QMetaObject>
#include <QApplication>
#include <QOpenGLContext>
#include <QPluginLoader>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <algorithm>

struct DVPluginInfo {
    QQmlContext* context = nullptr;

    QPluginLoader loader;

    DVPluginType::Type pluginType = DVPluginType::InvalidPlugin;

    DVRenderPlugin* renderPlugin = nullptr;
    DVInputPlugin* inputPlugin = nullptr;

    bool loaded = false;
    bool inited = false;
};

DVPluginManager::DVPluginManager(QObject* parent, QSettings& s) : QAbstractListModel(parent), settings(s) {
    /* Check to see if the table exists. */
    if (QSqlDatabase::database().record("plugins").isEmpty()) {
        /* TODO - Handle potential changes/additions to database fields. */
        resetPluginDatabase();
    }
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

    /* Tell the model system that we're going to be changing all the things. */
    beginResetModel();

    /* Try to load all files in the directory using the filters defined above. */
    for (const QString& filename : pluginsDir.entryList()) {
        /* If the file isn't a valid library for this platform, don't bother. */
        if (!QLibrary::isLibrary(filename)) continue;

        DVPluginInfo* plugin = new DVPluginInfo;
        plugin->loader.setFileName(pluginsDir.absoluteFilePath(filename));

        QString iid = plugin->loader.metaData().value("IID").toString();
        if (iid == DVRenderPlugin_iid)
            plugin->pluginType = DVPluginType::RenderPlugin;
        else if (iid == DVInputPlugin_iid)
            plugin->pluginType = DVPluginType::InputPlugin;
        else {
            qDebug("\"%s\" is not a valid plugin. Invalid IID \"%s\"!", qPrintable(filename), qPrintable(iid));
            delete plugin;
            continue;
        }

        plugins.insert(filename, plugin);

        QSqlRecord pluginRecord = getRecordForPlugin(filename);

        if (!pluginRecord.isEmpty() && pluginRecord.value("enabled").toBool()) {
            loadPlugin(filename);

            if (plugin->pluginType == DVPluginType::RenderPlugin)
                initRenderPlugin(filename);
            else if (plugin->pluginType == DVPluginType::InputPlugin)
                initInputPlugin(filename);
        }
    }

    qDebug("Done loading plugins.");


    /* Tell the model system that we've finished changing all the things. */
    endResetModel();
}

bool DVPluginManager::loadPlugin(const QString& pluginName) {
    DVPluginInfo* plugin = plugins[pluginName];
    plugin->loader.load();
    QObject *obj = plugin->loader.instance();

    if (obj == nullptr) {
        qDebug("\"%s\" is not a plugin. %s", qPrintable(pluginName), qPrintable(plugin->loader.errorString()));
        return false;
    }

    if (plugin->pluginType == DVPluginType::RenderPlugin)
        plugin->renderPlugin = qobject_cast<DVRenderPlugin*>(obj);
    else if (plugin->pluginType == DVPluginType::InputPlugin)
        plugin->inputPlugin = qobject_cast<DVInputPlugin*>(obj);

    plugin->loaded = plugin->renderPlugin != nullptr || plugin->inputPlugin != nullptr;

    if (plugin->renderPlugin != nullptr)
        qDebug("Found render plugin: \"%s\"", qPrintable(pluginName));
    else if (plugin->inputPlugin != nullptr)
        qDebug("Found input plugin: \"%s\"", qPrintable(pluginName));

    return plugin->loaded;
}

bool DVPluginManager::initRenderPlugin(const QString &pluginName) {
    DVPluginInfo* plugin = plugins[pluginName];

    if (plugin->pluginType != DVPluginType::RenderPlugin)
        return false;

    plugin->context = new QQmlContext(qmlEngine, this);

    if (plugin->renderPlugin != nullptr && plugin->renderPlugin->init(openglContext->extraFunctions(), plugin->context)) {
        /* Add to the list of usable render plugins. */
        renderPlugins.append(plugin->renderPlugin);
        pluginModes.append(plugin->renderPlugin->drawModeNames());

        plugin->inited = true;
    }

    qDebug(plugin->inited ? "Loaded plugin: \"%s\"" : "Plugin: \"%s\" failed to init.", qPrintable(pluginName));
    return plugin->inited;
}

bool DVPluginManager::initInputPlugin(const QString &pluginName) {
    DVPluginInfo* plugin = plugins[pluginName];

    if (plugin->pluginType != DVPluginType::InputPlugin)
        return false;

    plugin->context = new QQmlContext(qmlEngine, this);

    if (plugin->inputPlugin != nullptr && plugin->inputPlugin->init(plugin->context)) {
        /* Add to the list of usable input plugins. */
        inputPlugins.append(plugin->inputPlugin);

        plugin->inited = true;
    }

    qDebug(plugin->inited ? "Loaded plugin: \"%s\"" : "Plugin: \"%s\" failed to init.", qPrintable(pluginName));
    return plugin->inited;
}

void DVPluginManager::unloadPlugins() {
    /* Tell the model system that we're going to be changing all the things. */
    beginResetModel();

    /* Deinit any/all loaded plugins. */
    for (DVRenderPlugin* plugin : renderPlugins)
        plugin->deinit();
    for (DVInputPlugin* plugin : inputPlugins)
        plugin->deinit();
    /* TODO - Perhaps these should be deleted? Or will they be reused? (Probably not...) */
    for (DVPluginInfo* plugin : plugins)
        plugin->inited = false;

    /* Clear the list. Not that it should be used anymore... */
    renderPlugins.clear();
    inputPlugins.clear();

    /* Tell the model system that we've finished changing all the things. */
    endResetModel();
}

bool DVPluginManager::enablePlugin(QString pluginFileName) {
    if (loadPlugin(pluginFileName) && (initRenderPlugin(pluginFileName) || initInputPlugin(pluginFileName))) {
        /* If it loaded correctly remember to load it on startup. */
        storePluginEnabled(pluginFileName, true);

        QModelIndex changedIndex = createIndex(std::distance(plugins.begin(), plugins.find(pluginFileName)), 0);
        emit dataChanged(changedIndex, changedIndex);
        emit pluginModesChanged();

        return true;
    }

    return false;
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

QHash<int, QByteArray> DVPluginManager::roleNames() const {
    QHash<int, QByteArray> names;

    names[PluginFileNameRole]       = "pluginFileName";
    names[PluginDisplayNameRole]    = "pluginDisplayName";
    names[PluginDescriptionRole]    = "pluginDescription";
    names[PluginVersionRole]        = "pluginVersion";
    names[PluginTypeRole]           = "pluginType";
    names[PluginEnabledRole]        = "pluginEnabled";

    return names;
}
#include <QJsonDocument>
QVariant DVPluginManager::data(const QModelIndex& index, int role) const {
    QVariant data;

    if (int(plugins.size()) > index.row()) {
        auto plugin = (plugins.begin() +index.row());

        QJsonObject metaData = plugin.value()->loader.metaData().value("MetaData").toObject();

        /* Set the return value based on the role. */
        switch (role) {
        case PluginFileNameRole:
            data = plugin.key();
            break;
        case PluginDisplayNameRole:
            data = metaData.value("displayName");
            break;
        case PluginDescriptionRole:
            data = metaData.value("description");
            break;
        case PluginVersionRole:
            data = metaData.value("version");
            break;
        case PluginTypeRole:
            if (plugin.value()->pluginType == DVPluginType::RenderPlugin)
                data = tr("Render Plugin");
            else if (plugin.value()->pluginType == DVPluginType::InputPlugin)
                data = tr("Input Plugin");
            else
                data = tr("Invalid plugin");
            break;
        case PluginEnabledRole:
            data = plugin.value()->loaded;
            break;
        }
    }
    return data;
}

int DVPluginManager::rowCount(const QModelIndex&) const {
    return plugins.size();
}

void DVPluginManager::resetPluginDatabase() {
    if (!QSqlDatabase::database().record("plugins").isEmpty()) {
        QSqlQuery query("DROP TABLE plugins");
        if (query.lastError().isValid()) qWarning("Error deleting old table! %s", qPrintable(query.lastError().text()));
    }

    QSqlQuery query("create table plugins (filename string, enabled bool)");
    if (query.lastError().isValid()) qWarning("Error creating table! %s", qPrintable(query.lastError().text()));
}

QSqlRecord DVPluginManager::getRecordForPlugin(const QString& pluginName, bool create) const {
    if (plugins.contains(pluginName)) {
        QSqlQuery query;
        query.prepare("SELECT * FROM plugins WHERE filename = (:filename)");
        query.bindValue(":filename", pluginName);

        if (query.exec() && query.next())
            return query.record();

        /* If it didn't exist, optionally create it. */
        if (create) {
            query.prepare("INSERT INTO plugins (filename, enabled) VALUES (:filename, :enabled)");
            query.bindValue(":filename", pluginName);
            query.bindValue(":enabled", false);

            if (query.exec())
                /* The INSERT query does not return a record, so use this function again to retrieve the record. */
                return getRecordForPlugin(pluginName);

            qWarning("Unable to create record for plugin! %s", qPrintable(query.lastError().text()));
        }
    }

    return QSqlRecord();
}

void DVPluginManager::storePluginEnabled(const QString& pluginName, bool enable) {
    /* Get or create the record for the selected file. */
    QSqlRecord record = getRecordForPlugin(pluginName, true);

    if (record.value("enabled").toBool() == enable)
        return;

    QSqlQuery query;
    query.prepare("UPDATE plugins SET enabled = :enabled WHERE filename = :filename");
    query.bindValue(":enabled", enable);
    /* Use the record's path value. */
    query.bindValue(":filename", record.value(0));

    if (!query.exec())
        qWarning("Unable to update record for file! %s", qPrintable(query.lastError().text()));

    /* The model uses whether or not the plugin is currently loaded to provide the "pluginEnabled" property,
     * so this doesn't emit the dataChanged() signal, enablePlugin() does that... */
}
