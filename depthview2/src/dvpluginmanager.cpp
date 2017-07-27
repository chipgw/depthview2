#include "version.hpp"
#include "dvpluginmanager.hpp"
#include "dvinputplugin.hpp"
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

    DVInputPlugin* inputPlugin = nullptr;

    bool loaded = false;
    bool inited = false;

    QString errorString;
};

DVPluginManager::DVPluginManager(QObject* parent, QSettings& s) : QAbstractListModel(parent), settings(s) {
    /* Check to see if the table exists. */
    if (QSqlDatabase::database().record("plugins").isEmpty())
        resetPluginDatabase();
}

void DVPluginManager::postQmlInit() {
    /* Nothing */
}

void DVPluginManager::loadPlugins(QQmlEngine* engine) {
    qmlEngine = engine;

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
        if (iid == DVInputPlugin_iid) {
            plugin->pluginType = DVPluginType::InputPlugin;
            qDebug("Found input plugin: \"%s\"", qPrintable(filename));
        } else {
            qDebug("\"%s\" is not a valid plugin. Invalid IID \"%s\"!", qPrintable(filename), qPrintable(iid));
            delete plugin;
            continue;
        }

        plugins.insert(filename, plugin);

        QSqlRecord pluginRecord = getRecordForPlugin(filename);

        /* If there's a record for the plugin and it's set to enabled, load and init it. */
        if (!pluginRecord.isEmpty() && pluginRecord.value("enabled").toBool() &&
                loadPlugin(filename) && plugin->pluginType == DVPluginType::InputPlugin)
                initInputPlugin(filename);
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
        plugin->errorString = "Unable to load \"" + pluginName + "\" as a plugin. " + plugin->loader.errorString();
        qWarning("%s", qPrintable(plugin->errorString));
        return false;
    }

    if (plugin->pluginType == DVPluginType::InputPlugin)
        plugin->inputPlugin = qobject_cast<DVInputPlugin*>(obj);

    return (plugin->loaded = plugin->inputPlugin != nullptr);
}

bool DVPluginManager::initInputPlugin(const QString &pluginName) {
    DVPluginInfo* plugin = plugins[pluginName];

    if (plugin->pluginType != DVPluginType::InputPlugin)
        return false;

    plugin->context = new QQmlContext(qmlEngine, this);

    if (plugin->inputPlugin != nullptr && plugin->inputPlugin->init(plugin->context)) {
        /* Add to the list of usable input plugins. */
        inputPlugins.append(plugin->inputPlugin);

        qDebug("Loaded plugin: \"%s\"", qPrintable(pluginName));
        return plugin->inited = true;
    }

    plugin->errorString = "Plugin \"" + pluginName + "\" failed to init. " + plugin->inputPlugin->getErrorString();
    qWarning("%s", qPrintable(plugin->errorString));
    return false;
}

void DVPluginManager::unloadPlugins() {
    /* Tell the model system that we're going to be changing all the things. */
    beginResetModel();

    /* Deinit any/all loaded plugins. */
    for (DVInputPlugin* plugin : inputPlugins)
        plugin->deinit();
    /* TODO - Perhaps these should be deleted? Or will they be reused? (Probably not...) */
    for (DVPluginInfo* plugin : plugins)
        plugin->inited = false;

    /* Clear the list. Not that it should be used anymore... */
    inputPlugins.clear();

    /* Tell the model system that we've finished changing all the things. */
    endResetModel();
}

bool DVPluginManager::enablePlugin(QString pluginFileName) {
    auto plugin = plugins.find(pluginFileName);

    if (plugin.value()->loaded && plugin.value()->inited)
        /* Plugin is already enabled. */
        return true;

    if (loadPlugin(pluginFileName) && initInputPlugin(pluginFileName)) {
        /* If it loaded correctly remember to load it on startup. */
        storePluginEnabled(pluginFileName, true);

        emit enabledPluginsChanged();
    }

    QModelIndex changedIndex = createIndex(std::distance(plugins.begin(), plugin), 0);
    /* Emit this signal to update the pluginEnabled value if it worked and the pluginError value if it didn't. */
    emit dataChanged(changedIndex, changedIndex);

    return plugin.value()->inited;
}

bool DVPluginManager::disablePlugin(QString pluginFileName) {
    auto plugin = plugins.find(pluginFileName);

    /* Remove from auto-load. */
    storePluginEnabled(pluginFileName, false);

    if (plugin.value()->pluginType == DVPluginType::InputPlugin) {
        /* Remove the input plugin from the list and deinit it. */
        inputPlugins.removeAll(plugin.value()->inputPlugin);
        plugin.value()->inputPlugin->deinit();
    }
    /* The plugin isn't inited any more. */
    plugin.value()->inited = false;

    /* For render plugins updates the draw mode list, for all plugins removes the config object from the settings window. */
    emit enabledPluginsChanged();

    QModelIndex changedIndex = createIndex(std::distance(plugins.begin(), plugin), 0);
    /* Emit this signal to update the pluginEnabled value if it worked and the pluginError value if it didn't. */
    emit dataChanged(changedIndex, changedIndex);

    return true;
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

void DVPluginManager::doPluginInput(DVInputInterface* inputInterface) {
    /* Get input from ALL input plugins. */
    for (DVInputPlugin* plugin : inputPlugins)
        plugin->pollInput(inputInterface);
}

QStringList DVPluginManager::getModes() const {
    return QStringList() << "Anaglyph"
                         << "Side-by-Side"
                         << "Top/Bottom"
                         << "Interlaced Horizontal"
                         << "Interlaced Vertical"
                         << "Checkerboard"
                         << "Mono";
}

QObjectList DVPluginManager::getPluginConfigMenus() const {
    QObjectList list;

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
    names[PluginErrorRole]          = "pluginError";

    return names;
}

QVariant DVPluginManager::data(const QModelIndex& index, int role) const {
    QVariant data;

    /* Make sure the index is valid. */
    if (plugins.size() > index.row()) {
        auto plugin = plugins.begin() + index.row();

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
            if (plugin.value()->pluginType == DVPluginType::InputPlugin)
                data = tr("Input Plugin");
            else
                data = tr("Invalid plugin");
            break;
        case PluginEnabledRole:
            data = plugin.value()->loaded && plugin.value()->inited;
            break;
        case PluginErrorRole:
            data = plugin.value()->errorString;
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
