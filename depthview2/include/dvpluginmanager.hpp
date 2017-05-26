#pragma once

#include <QList>
#include <QDir>
#include <QAbstractListModel>
#include <QSqlRecord>

class DVInputPlugin;
class DVRenderPlugin;
class DVWindow;
class DVInputInterface;
class DVRenderInterface;

class QSettings;
class QQmlEngine;
class QOpenGLContext;

class DVPluginManager : public QAbstractListModel {
    Q_OBJECT

    QSettings& settings;
    QQmlEngine* qmlEngine;
    QOpenGLContext* openglContext;

    QString m_pluginMode;
    QDir pluginsDir;

    /* Any loaded plugins. */
    QMap<QString, struct DVPluginInfo*> plugins;
    /* Any inited plugins of the specific type. */
    QList<DVRenderPlugin*> renderPlugins;
    QList<DVInputPlugin*> inputPlugins;

    /* Store a list of render modes supported by loaded plugins. */
    QList<QString> pluginModes;

    QSqlRecord getRecordForPlugin(const QString& pluginName, bool create = false) const;
    void storePluginEnabled(const QString &pluginName, bool enable);

    Q_PROPERTY(QString pluginMode READ pluginMode WRITE setPluginMode NOTIFY pluginModeChanged)
    Q_PROPERTY(QStringList pluginModes READ getPluginModes NOTIFY pluginModesChanged)
    Q_PROPERTY(QStringList modes READ getModes NOTIFY pluginModesChanged)

    Q_PROPERTY(QList<QObject*> pluginConfigMenus READ getPluginConfigMenus NOTIFY pluginModesChanged)

public:
    explicit DVPluginManager(QObject* parent, QSettings& s);
    void postQmlInit();

    /* Load static and dynamic plugins and init them. */
    void loadPlugins(QQmlEngine* engine, QOpenGLContext* context);
    bool loadPlugin(const QString& pluginName);
    bool initRenderPlugin(const QString& pluginName);
    bool initInputPlugin(const QString& pluginName);
    /* Call deinit() of all loaded plugins, so as to garbage collect anything they created. */
    void unloadPlugins();

    Q_INVOKABLE bool enablePlugin(QString pluginFileName);
    Q_INVOKABLE bool disablePlugin(QString pluginFileName);

    Q_INVOKABLE void resetPluginDatabase();

    /* Functions that get the current plugin and interface with it. */
    DVRenderPlugin* getCurrentRenderPlugin() const;
    bool doPluginRender(DVRenderInterface* renderInterface);
    QSize getPluginSize(QSize inputSize);

    /* Returns true if the window should hold the mouse. */
    bool onFrameSwapped();

    void doPluginInput(DVInputInterface* inputInterface);

    QString pluginMode() const;
    void setPluginMode(const QString& mode);

    QStringList getPluginModes() const;
    QStringList getModes() const;
    QObjectList getPluginConfigMenus() const;

    /* QObject should be const, but QML does not know how to do const. */
    Q_INVOKABLE void savePluginSettings(QString pluginTitle, QObject* settingsObject);
    Q_INVOKABLE void loadPluginSettings(QString pluginTitle, QObject* settingsObject);

    /* Begin model stuff. */
    enum Roles {
        PluginFileNameRole = Qt::UserRole+1,
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
    void pluginModeChanged(QString mode);
    void pluginModesChanged();
};
