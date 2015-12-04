#include "include/dvqmlcommunication.hpp"
#include <QWindow>
#include <QStorageInfo>
#include <QApplication>

DVQmlCommunication::DVQmlCommunication(QWindow* parent) : QObject(parent),
    settings(QSettings::IniFormat, QSettings::UserScope, QApplication::organizationName(), QApplication::applicationName()),
    m_mirrorLeft(false), m_mirrorRight(false), m_greyFac(0.0), m_drawMode(DVDrawMode::Anaglyph), m_anamorphicDualView(false), owner(parent) {
    connect(owner, &QWindow::windowStateChanged, this, &DVQmlCommunication::ownerWindowStateChanged);

    if (settings.contains("Bookmarks"))
        m_bookmarks = settings.value("Bookmarks").toStringList();

    if (settings.contains("DrawMode"))
        m_drawMode = DVDrawMode::fromString(settings.value("DrawMode").toByteArray());
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
        emit pluginModeChanged(mode);
    }
}

void DVQmlCommunication::addPluginModes(const QStringList &modes) {
    pluginModes.append(modes);
}

QStringList DVQmlCommunication::getPluginModes() const {
    return pluginModes;
}

void DVQmlCommunication::addBookmark(QString bookmark) {
    /* TODO - Maybe we should make sure it exists? */
    if (!m_bookmarks.contains(bookmark)) {
        m_bookmarks.append(bookmark);
        bookmarksChanged(m_bookmarks);
        settings.setValue("Bookmarks", m_bookmarks);
    }
}

void DVQmlCommunication::deleteBookmark(QString bookmark) {
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

    return paths;
}
