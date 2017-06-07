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
#include <QQuickItem>

DVQmlCommunication::DVQmlCommunication(QWindow* parent, QSettings& s) : QObject(parent),
    settings(s), owner(parent), lastWindowState(Qt::WindowNoState), m_swapEyes(false), imageTarget(nullptr) {
    /* We need to detect when the window state changes sowe can updatethe fullscreen property accordingly. */
    connect(owner, &QWindow::windowStateChanged, this, &DVQmlCommunication::ownerWindowStateChanged);

    m_drawMode = settings.contains("DrawMode") ? DVDrawMode::fromString(settings.value("DrawMode").toByteArray()) : DVDrawMode::Anaglyph;

    m_greyFac = settings.contains("GreyFac") ? settings.value("GreyFac").toReal() : 0.0;

    m_swapEyes = settings.contains("SwapEyes") ? settings.value("SwapEyes").toBool() : false;

    m_anamorphicDualView = settings.contains("Anamorphic") ? settings.value("Anamorphic").toBool() : false;

    m_mirrorLeft = settings.contains("MirrorLeft") ? settings.value("MirrorLeft").toBool() : false;
    m_mirrorRight = settings.contains("MirrorRight") ? settings.value("MirrorRight").toBool() : false;

    /* This constructor gets called before QML is set up, so this works. */
    if (settings.contains("ControlsTheme"))
        QQuickStyle::setStyle(settings.value("ControlsTheme").toString());
}

void DVQmlCommunication::postQmlInit() {
    /* UNUSED */
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

bool DVQmlCommunication::swapEyes() const {
    return m_swapEyes;
}
void DVQmlCommunication::setSwapEyes(bool swap) {
    if (swap != m_swapEyes) {
        m_swapEyes = swap;
        settings.setValue("SwapEyes", swap);
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
            /* Start the application again with the same file. */
            QProcess::startDetached(QApplication::applicationFilePath(), QStringList(folderListing->currentURL().toLocalFile()));
            QApplication::quit();
        }
    }
}

QQuickItem* DVQmlCommunication::openImageTarget() {
    return imageTarget;
}

QSGTextureProvider* DVQmlCommunication::openImageTexture() {
    if (imageTarget && imageTarget->isTextureProvider())
        return imageTarget->textureProvider();

    return nullptr;
}

void DVQmlCommunication::setOpenImageTarget(QQuickItem *target) {
    if (target != imageTarget) {
        imageTarget = target;
        emit openImageTargetChanged();
    }
}

QPointF DVQmlCommunication::surroundPan() const {
    return m_surroundPan;
}

void DVQmlCommunication::setSurroundPan(QPointF val) {
    if (val != m_surroundPan) {
        m_surroundPan = val;
        m_surroundPan.setY(qBound(-89.0, m_surroundPan.y(), 89.0));

        emit surroundPanChanged();
    }
}

qreal DVQmlCommunication::surroundFOV() const {
    return m_surroundFOV;
}

void DVQmlCommunication::setSurroundFOV(qreal val) {
    if (val != m_surroundFOV) {
        /* This is about the same limits as the value of zoom has [0.2, 4.0], based on the way it is converted. */
        m_surroundFOV = qBound(7.5, val, 105.0);

        emit surroundFOVChanged();
    }
}

#ifdef DV_FILE_ASSOCIATION
void DVQmlCommunication::registerFileTypes() {
    fileassociation::registerFileTypes();
}
#endif
