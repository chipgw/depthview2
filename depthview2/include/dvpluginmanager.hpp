#pragma once

#include <QList>
#include <QDir>
#include <QAbstractListModel>
#include <QSqlRecord>

class DVInputPlugin;
class DVWindow;
class DVInputInterface;

class QSettings;
class QQmlEngine;

class DVPluginManager : public QAbstractListModel {
    Q_OBJECT

    QSettings& settings;
    QQmlEngine* qmlEngine;

    QString m_pluginMode;
    QDir pluginsDir;

    /* Any loaded plugins. */
    QMap<QString, struct DVPluginInfo*> plugins;
    /* Any inited plugins of the specific type. */
    QList<DVInputPlugin*> inputPlugins;

    /* Store a list of render modes supported by loaded plugins. */
    QList<QString> pluginModes;

    QSqlRecord getRecordForPlugin(const QString& pluginName, bool create = false) const;
    void storePluginEnabled(const QString &pluginName, bool enable);

    Q_PROPERTY(QStringList modes READ getModes NOTIFY modesChanged)

    Q_PROPERTY(QList<QObject*> pluginConfigMenus READ getPluginConfigMenus NOTIFY enabledPluginsChanged)

public:
    explicit DVPluginManager(QObject* parent, QSettings& s);

    /* Load plugins and init any that are enabled in the database. */
    void loadPlugins(QQmlEngine* engine);
    /* Load an instance of a plugin into memory based on the detected type. */
    bool loadPlugin(const QString& pluginName);
    /* Call init() on specified input plugin, and prepare it for use. */
    bool initInputPlugin(const QString& pluginName);
    /* Call deinit() of all loaded plugins, so as to garbage collect anything they created. */
    void unloadPlugins();

    Q_INVOKABLE bool enablePlugin(QString pluginFileName);
    Q_INVOKABLE bool disablePlugin(QString pluginFileName);

    Q_INVOKABLE void resetPluginDatabase();

    /* Get input from all loadedinput plugins. */
    void doPluginInput(DVInputInterface* inputInterface);

    QStringList getModes() const;
    QObjectList getPluginConfigMenus() const;

    /* QObject should be const, but QML does not know how to do const. */
    Q_INVOKABLE void savePluginSettings(QString pluginTitle, QObject* settingsObject);
    Q_INVOKABLE void loadPluginSettings(QString pluginTitle, QObject* settingsObject);

    /* Begin model stuff. */
    enum Roles {
        PluginFileNameRole = Qt::UserRole,
        PluginDisplayNameRole,
        PluginDescriptionRole,
        PluginVersionRole,
        PluginTypeRole,
        PluginEnabledRole,
        PluginErrorRole
    };

    QHash<int, QByteArray> roleNames() const;

    /* The function that gives QML the different properties for a given file in the current dir. */
    QVariant data(const QModelIndex &index, int role) const;

    /* How many files are in the current dir? */
    int rowCount(const QModelIndex& parent) const;

signals:
    void modesChanged();
    void enabledPluginsChanged();
};
