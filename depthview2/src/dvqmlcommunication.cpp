#include "dvqmlcommunication.hpp"
#include "dvfolderlisting.hpp"
#include "version.hpp"
#include <QWindow>
#include <QDir>
#include <QApplication>
#include <QMessageBox>
#include <QSettings>
#include <QQuickStyle>
#include <QProcess>

DVQmlCommunication::DVQmlCommunication(QWindow* parent, QSettings& s) : QObject(parent),
    settings(s), owner(parent), lastWindowState(Qt::WindowNoState), m_swapEyes(false) {
    /* We need to detect when the window state changes sowe can updatethe fullscreen property accordingly. */
    connect(owner, &QWindow::windowStateChanged, this, &DVQmlCommunication::ownerWindowStateChanged);

    m_drawMode = settings.contains("DrawMode") ? DVDrawMode::fromString(settings.value("DrawMode").toByteArray()) : DVDrawMode::Anaglyph;

    m_greyFac = settings.contains("GreyFac") ? settings.value("GreyFac").toReal() : 0.0;

    m_anamorphicDualView = settings.contains("Anamorphic") ? settings.value("Anamorphic").toBool() : false;

    m_mirrorLeft = settings.contains("MirrorLeft") ? settings.value("MirrorLeft").toBool() : false;
    m_mirrorRight = settings.contains("MirrorRight") ? settings.value("MirrorRight").toBool() : false;

    /* This constructor gets called before QML is set up, so this works. */
    if (settings.contains("ControlsTheme"))
        QQuickStyle::setStyle(settings.value("ControlsTheme").toString());
}

void DVQmlCommunication::postQmlInit() {
    if (settings.contains("PluginMode")) {
        /* Check to make sure the plugin mode is a valid loaded plugin before setting. */
        QString mode = settings.value("PluginMode").toString();
        if (pluginModes.contains(mode))
            emit pluginModeChanged(m_pluginMode = mode);
        else
            qWarning("Invalid plugin mode \"%s\" set in settings file!", qPrintable(mode));
    }
}

DVDrawMode::Type DVQmlCommunication::drawMode() const {
    return m_drawMode;
}

void DVQmlCommunication::setDrawMode(DVDrawMode::Type mode) {
    /* Only emit if changed. */
    if (m_drawMode != mode) {
        m_drawMode = mode;
        settings.setValue("DrawMode", DVDrawMode::toString(mode));
        emit drawModeChanged(mode);
    }
}

void DVQmlCommunication::initDrawMode(DVDrawMode::Type mode) {
    m_drawMode = mode;
    settings.setValue("DrawMode", DVDrawMode::toString(mode));
}

bool DVQmlCommunication::anamorphicDualView() const {
    return m_anamorphicDualView;
}

void DVQmlCommunication::setAnamorphicDualView(bool anamorphic) {
    /* Only emit if changed. */
    if (m_anamorphicDualView != anamorphic) {
        m_anamorphicDualView = anamorphic;
        settings.setValue("Anamorphic", anamorphic);
        emit anamorphicDualViewChanged(anamorphic);
    }
}

bool DVQmlCommunication::mirrorLeft() const {
    return m_mirrorLeft;
}

void DVQmlCommunication::setMirrorLeft(bool mirror) {
    /* Only emit if changed. */
    if (m_mirrorLeft != mirror) {
        m_mirrorLeft = mirror;
        settings.setValue("MirrorLeft", mirror);
        emit mirrorLeftChanged(mirror);
    }
}

bool DVQmlCommunication::mirrorRight() const {
    return m_mirrorRight;
}

void DVQmlCommunication::setMirrorRight(bool mirror) {
    /* Only emit if changed. */
    if (m_mirrorRight != mirror) {
        m_mirrorRight = mirror;
        settings.setValue("MirrorRight", mirror);
        emit mirrorRightChanged(mirror);
    }
}

bool DVQmlCommunication::fullscreen() const {
    return owner->windowState() == Qt::WindowFullScreen;
}

void DVQmlCommunication::setFullscreen(bool fullscreen) {
    /* Only set if changed. */
    if (fullscreen != (owner->windowState() == Qt::WindowFullScreen))
        owner->setWindowState(fullscreen ? Qt::WindowFullScreen : lastWindowState);

    /* Signal will be emitted because of the state change. */
}

void DVQmlCommunication::ownerWindowStateChanged(Qt::WindowState windowState) {
    /* TODO - Sometimes when entering fullscreen it changes to WindowNoState first which breaks it... */
    if (windowState != Qt::WindowFullScreen)
        lastWindowState = windowState;

    emit fullscreenChanged(windowState == Qt::WindowFullScreen);
}

qreal DVQmlCommunication::greyFac() const {
    return m_greyFac;
}

void DVQmlCommunication::setGreyFac(qreal fac) {
    /* Limit to [0, 1] range. */
    fac = qBound(0.0, fac, 1.0);

    /* Only emit if changed. */
    if (fac != m_greyFac) {
        m_greyFac = fac;
        settings.setValue("GreyFac", fac);
        emit greyFacChanged(fac);
    }
}

QString DVQmlCommunication::pluginMode() const {
    return m_pluginMode;
}

void DVQmlCommunication::setPluginMode(const QString& mode) {
    /* Only set if valid. */
    if (mode != m_pluginMode && pluginModes.contains(mode)) {
        m_pluginMode = mode;
        settings.setValue("PluginMode", mode);
        emit pluginModeChanged(mode);
    }
}

void DVQmlCommunication::addPluginMode(const QString& mode, QQuickItem* config) {
    pluginModes[mode] = config;
    emit pluginModesChanged();
}

void DVQmlCommunication::addInputPluginConfig(QQuickItem* config) {
    inputPluginConfig.append(config);
    emit pluginModesChanged();
}

QStringList DVQmlCommunication::getPluginModes() const {
    return pluginModes.keys();
}

QQuickItem* DVQmlCommunication::getPluginConfigMenu() const {
    return pluginModes.contains(m_pluginMode) ? pluginModes[m_pluginMode] : nullptr;
}

QObjectList DVQmlCommunication::getPluginConfigMenus() const {
    QObjectList list;

    for (QQuickItem* item : pluginModes.values())
        list.append((QObject*)item);
    for (QQuickItem* item : inputPluginConfig)
        list.append((QObject*)item);

    return list;
}

QStringList DVQmlCommunication::getModes() const {
    return QStringList() << "Anaglyph"
                         << "Side-by-Side"
                         << "Top/Bottom"
                         << "Interlaced Horizontal"
                         << "Interlaced Vertical"
                         << "Checkerboard"
                         << "Mono"
                         << pluginModes.keys();
}

bool DVQmlCommunication::swapEyes() const {
    return m_swapEyes;
}
void DVQmlCommunication::setSwapEyes(bool swap) {
    if (swap != m_swapEyes) {
        m_swapEyes = swap;
        emit swapEyesChanged();
    }
}

QString DVQmlCommunication::versionString() {
    return version::number.toString() + " Beta";
}

QString DVQmlCommunication::buildType() {
    return version::build_type;
}

QString DVQmlCommunication::buildCompiler() {
    return version::compiler;
}

void DVQmlCommunication::savePluginSettings(QString pluginTitle, QObject* settingsObject) {
    /* Remove spaces from the plugin title. */
    pluginTitle.remove(' ');

    settings.beginGroup(pluginTitle);

    /* Go through all properties of the item excluding the first one, which is the objectName property of QObject. */
    for (int i = 1; i < settingsObject->metaObject()->propertyCount(); ++i)
        settings.setValue(settingsObject->metaObject()->property(i).name(), settingsObject->metaObject()->property(i).read(settingsObject));

    settings.endGroup();
}

void DVQmlCommunication::loadPluginSettings(QString pluginTitle, QObject* settingsObject) {
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

bool DVQmlCommunication::saveWindowState() const {
    /* Default value if it isn't set is true. */
    return settings.value("SaveWindowState", true).toBool();
}

void DVQmlCommunication::setSaveWindowState(bool save) {
    if (!settings.contains("SaveWindowState") || settings.value("SaveWindowState").toBool() != save) {
        settings.setValue("SaveWindowState", save);
        emit saveWindowStateChanged();
    }
}

bool DVQmlCommunication::startupFileBrowser() const {
    /* Default value if it isn't set is false. */
    return settings.value("StartupFileBrowser").toBool();
}

void DVQmlCommunication::setStartupFileBrowser(bool open) {
    if (!settings.contains("StartupFileBrowser") || settings.value("StartupFileBrowser").toBool() != open) {
        settings.setValue("StartupFileBrowser", open);
        emit startupFileBrowserChanged();
    }
}

QString DVQmlCommunication::uiTheme() const {
    /* Either get it from settings, or get the one currently being used. */
    return settings.contains("ControlsTheme") ? settings.value("ControlsTheme").toString() : QQuickStyle::name();
}

QStringList DVQmlCommunication::uiThemes() const {
    return {"Default", "Material", "Universal"};
}

void DVQmlCommunication::setUiTheme(QString theme) {
    if ((!settings.contains("ControlsTheme") || settings.value("ControlsTheme").toString() != theme) && uiThemes().contains(theme)) {
        settings.setValue("ControlsTheme", theme);

        emit uiThemeChanged();

        /* Theme has to be set before any controls are loaded by QML, so a restart is required to apply. */
        if (QMessageBox::question(nullptr, "Restart to apply?", "You must restart to apply the new UI Theme, restart now?",
                                  QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            /* TODO - Send arguments to restore current state. */
            QProcess::startDetached(QApplication::applicationFilePath());
            QApplication::quit();
        }
    }
}

#ifdef DV_FILE_ASSOCIATION
void DVQmlCommunication::registerFileTypes() {
    fileassociation::registerFileTypes();
}
#endif
