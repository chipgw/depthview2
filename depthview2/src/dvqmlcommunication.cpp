#include "include/dvqmlcommunication.hpp"
#include <QWindow>
#include <QStorageInfo>
#include <QApplication>

#ifdef DV_PORTABLE
/* Portable builds store settings in a "DepthView.conf" next to the application executable. */
#define SETTINGS_ARGS QApplication::applicationDirPath() + "/DepthView.conf", QSettings::IniFormat
#else
/* Non-portable builds use an ini file in "%APPDATA%/chipgw" or "~/.config/chipgw". */
#define SETTINGS_ARGS QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName()
#endif

DVQmlCommunication::DVQmlCommunication(QWindow* parent) : QObject(parent), settings(SETTINGS_ARGS), owner(parent), currentHistory(-1), driveTimer(this) {
    /* We need to detect when the window state changes sowe can updatethe fullscreen property accordingly. */
    connect(owner, &QWindow::windowStateChanged, this, &DVQmlCommunication::ownerWindowStateChanged);

    if (settings.contains("Bookmarks"))
        m_bookmarks = settings.value("Bookmarks").toStringList();

    m_drawMode = settings.contains("DrawMode") ? DVDrawMode::fromString(settings.value("DrawMode").toByteArray()) : DVDrawMode::Anaglyph;

    /* TODO - We can't check if it's valid from here, as the plugins are not inited yet, but it should check somehow... */
    if (settings.contains("PluginMode"))
        m_pluginMode = settings.value("PluginMode").toString();

    m_greyFac = settings.contains("GreyFac") ? settings.value("GreyFac").toReal() : 0.0;

    m_anamorphicDualView = settings.contains("Anamorphic") ? settings.value("Anamorphic").toBool() : false;

    m_mirrorLeft = settings.contains("MirrorLeft") ? settings.value("MirrorLeft").toBool() : false;
    m_mirrorRight = settings.contains("MirrorRight") ? settings.value("MirrorRight").toBool() : false;

    /* TODO - Figure out a way to detect when there is actually a change rather than just putting it on a timer. */
    connect(&driveTimer, &QTimer::timeout, this, &DVQmlCommunication::storageDevicePathsChanged);
    driveTimer.start(8000);
}

bool DVQmlCommunication::isLeft() const {
    return m_isLeft;
}

DVDrawMode::Type DVQmlCommunication::drawMode() const {
    return m_drawMode;
}

void DVQmlCommunication::setDrawMode(DVDrawMode::Type mode) {
    /* Only emit if changed. */
    if(m_drawMode != mode) {
        m_drawMode = mode;
        settings.setValue("DrawMode", DVDrawMode::toString(mode));
        emit drawModeChanged(mode);
    }
}

bool DVQmlCommunication::anamorphicDualView() const {
    return m_anamorphicDualView;
}

void DVQmlCommunication::setAnamorphicDualView(bool anamorphic) {
    /* Only emit if changed. */
    if(m_anamorphicDualView != anamorphic) {
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
        owner->setWindowState(fullscreen ? Qt::WindowFullScreen : Qt::WindowMaximized);

    /* Signal will be emitted because of the state change. */
}

void DVQmlCommunication::ownerWindowStateChanged(Qt::WindowState windowState) {
    /* TODO - Somehow maybe this should only emit the signal if fullscreen is what changed? */
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

void DVQmlCommunication::setPluginMode(QString mode) {
    /* Only set if valid. */
    if (mode != m_pluginMode && pluginModes.contains(mode)) {
        m_pluginMode = mode;
        settings.setValue("PluginMode", mode);
        emit pluginModeChanged(mode);
    }
}

void DVQmlCommunication::addPluginModes(const QStringList& modes) {
    pluginModes.append(modes);
}

QStringList DVQmlCommunication::getPluginModes() const {
    return pluginModes;
}

void DVQmlCommunication::addBookmark(QString bookmark) {
    /* Don't add existing bookmarks, and don't add directories that don't exist. */
    if (!m_bookmarks.contains(bookmark) && dirExists(decodeURL(bookmark))) {
        m_bookmarks.append(bookmark);
        bookmarksChanged(m_bookmarks);
        settings.setValue("Bookmarks", m_bookmarks);
    }
}

void DVQmlCommunication::deleteBookmark(QString bookmark) {
    /* Only emit signal and update setting if something was actually deleted. */
    if (m_bookmarks.removeAll(bookmark) != 0) {
        bookmarksChanged(m_bookmarks);
        settings.setValue("Bookmarks", m_bookmarks);
    }
}

QStringList DVQmlCommunication::bookmarks() const {
    return m_bookmarks;
}

QStringList DVQmlCommunication::getStorageDevicePaths() const {
    QStringList paths;

    for (QStorageInfo info : QStorageInfo::mountedVolumes())
        paths.append(info.rootPath() + ';' + info.displayName());

    /* There are duplicates on Android for some reason... */
    paths.removeDuplicates();

    return paths;
}

bool DVQmlCommunication::fileExists(QString file) const {
    return QFile::exists(file);
}
bool DVQmlCommunication::dirExists(QString dir) const {
    return QDir(dir).exists();
}

QUrl DVQmlCommunication::encodeURL(QString url) const {
    return QUrl::fromLocalFile(url);
}
QString DVQmlCommunication::decodeURL(QUrl url) const {
    return url.toLocalFile();
}

QString DVQmlCommunication::goBack() {
    --currentHistory;
    emit historyChanged();
    return browserHistory[currentHistory];
}

QString DVQmlCommunication::goForward() {
    ++currentHistory;
    emit historyChanged();
    return browserHistory[currentHistory];
}

void DVQmlCommunication::pushHistory(QString value) {
    /* We ignore if value is empty or equal to the current history item. */
    if (!value.isEmpty() && (browserHistory.isEmpty() || browserHistory[currentHistory] != value)) {
        /* If the next item is equal to the passed value we keep the current history and just increment it. */
        if (canGoForward() && browserHistory[currentHistory + 1] == value)
            ++currentHistory;
        /* Same for back, just decrement instead of increment. */
        else if (canGoBack() && browserHistory[currentHistory - 1] == value)
            --currentHistory;
        else {
            ++currentHistory;

            /* If there are any forward entries, they must be cleared before adding the new entry. */
            if (canGoForward())
                browserHistory.erase(browserHistory.begin() + currentHistory, browserHistory.end());

            browserHistory.append(value);
        }

        /* At this point no matter which branch it went through it has changed. */
        emit historyChanged();
    }
}

bool DVQmlCommunication::canGoBack() const {
    /* We can go back if the list isn't empty and we aren't at the first item in the list. */
    return !browserHistory.isEmpty() && currentHistory > 0;
}

bool DVQmlCommunication::canGoForward() const {
    /* We can go forward if currentHistory isn't the last item in the list. */
    return !browserHistory.isEmpty() && currentHistory < (browserHistory.size() - 1);
}
