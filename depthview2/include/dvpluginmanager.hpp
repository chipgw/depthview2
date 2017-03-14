#pragma once

#include <QList>
#include <QDir>

class DVInputPlugin;
class DVRenderPlugin;
class DVWindow;
class DVInputInterface;

class QSettings;
class QQmlEngine;
class QOpenGLContext;

class DVPluginManager : public QObject {
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

    Q_PROPERTY(QString pluginMode READ pluginMode WRITE setPluginMode NOTIFY pluginModeChanged)
    Q_PROPERTY(QStringList pluginModes READ getPluginModes NOTIFY pluginModesChanged)
    Q_PROPERTY(QStringList modes READ getModes NOTIFY pluginModesChanged)

    Q_PROPERTY(QObject* pluginConfigMenu READ getPluginConfigMenu NOTIFY pluginModeChanged)
    Q_PROPERTY(QList<QObject*> pluginConfigMenus READ getPluginConfigMenus NOTIFY pluginModesChanged)

public:
    explicit DVPluginManager(QObject* parent, QSettings& s);
    void postQmlInit();

    /* Load static and dynamic plugins and init them. */
    void loadPlugins(QQmlEngine* engine, QOpenGLContext *context);
    bool loadPlugin(const QString& pluginName);
    bool initPlugin(const QString& pluginName);
    /* Call deinit() of all loaded plugins, so as to garbage collect anything they created. */
    void unloadPlugins();

    /* Functions that get the current plugin and interface with it. */
    DVRenderPlugin* getCurrentRenderPlugin() const;
    bool doPluginRender();
    QSize getPluginSize(QSize inputSize);

    /* Returns true if the window should hold the mouse. */
    bool onFrameSwapped();

    void doPluginInput(DVInputInterface* inputInterface);

    QString pluginMode() const;
    void setPluginMode(const QString& mode);

    QStringList getPluginModes() const;
    QStringList getModes() const;
    QObject* getPluginConfigMenu() const;
    QObjectList getPluginConfigMenus() const;

    /* QObject should be const, but QML does not know how to do const. */
    Q_INVOKABLE void savePluginSettings(QString pluginTitle, QObject* settingsObject);
    Q_INVOKABLE void loadPluginSettings(QString pluginTitle, QObject* settingsObject);

signals:
    void pluginModeChanged(QString mode);
    void pluginModesChanged();
};
