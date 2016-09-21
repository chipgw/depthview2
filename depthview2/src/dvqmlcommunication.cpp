#include "dvqmlcommunication.hpp"
#include "version.hpp"
#include <QWindow>
#include <QDir>
#include <QApplication>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QSettings>

DVQmlCommunication::DVQmlCommunication(QWindow* parent, QSettings& s) : QObject(parent), settings(s), owner(parent) {
    /* We need to detect when the window state changes sowe can updatethe fullscreen property accordingly. */
    connect(owner, &QWindow::windowStateChanged, this, &DVQmlCommunication::ownerWindowStateChanged);

    m_drawMode = settings.contains("DrawMode") ? DVDrawMode::fromString(settings.value("DrawMode").toByteArray()) : DVDrawMode::Anaglyph;

    /* TODO - We can't check if it's valid from here, as the plugins are not inited yet, but it should check somehow... */
    if (settings.contains("PluginMode"))
        m_pluginMode = settings.value("PluginMode").toString();

    m_greyFac = settings.contains("GreyFac") ? settings.value("GreyFac").toReal() : 0.0;

    m_anamorphicDualView = settings.contains("Anamorphic") ? settings.value("Anamorphic").toBool() : false;

    m_mirrorLeft = settings.contains("MirrorLeft") ? settings.value("MirrorLeft").toBool() : false;
    m_mirrorRight = settings.contains("MirrorRight") ? settings.value("MirrorRight").toBool() : false;

    doCommandLine();
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
    emit pluginModesChanged();
}

QStringList DVQmlCommunication::getPluginModes() const {
    return pluginModes;
}

QStringList DVQmlCommunication::getModes() const {
    return QStringList() << "Anaglyph"
                         << "Side-by-Side"
                         << "Top/Bottom"
                         << "Interlaced Horizontal"
                         << "Interlaced Vertical"
                         << "Checkerboard"
                         << "Mono Left"
                         << "Mono Right"
                         << pluginModes;
}

void DVQmlCommunication::doCommandLine() {
    QCommandLineParser parser;

    /* TODO - More arguments. */
    parser.addOptions({
            { {"f", "fullscreen"},
              QCoreApplication::translate("main", "")},
            { {"d", "startdir"},
              QCoreApplication::translate("main", ""),
              QCoreApplication::translate("main", "directory")},
            { {"r", "renderer"},
              QCoreApplication::translate("main", ""),
              QCoreApplication::translate("main", "renderer")},
        });

    parser.parse(QApplication::arguments());

    /* We use one string to hold all warning messages, so we only have to show one dialog. */
    QString warning;

    if(parser.isSet("f"))
        setFullscreen(true);

    if(parser.isSet("d") && !QDir::setCurrent(parser.value("d")))
        warning += tr("<p>Invalid directory \"%1\" passed to \"--startdir\" argument!</p>").arg(parser.value("d"));

    if(parser.isSet("r")){
        const QString &renderer = parser.value("r");

        int mode = getModes().indexOf(renderer);

        if(mode == -1)
            warning += tr("<p>Invalid renderer \"%1\" passed to \"--renderer\" argument!</p>").arg(renderer);

        if (mode >= DVDrawMode::Plugin) {
            m_pluginMode = renderer;
            m_drawMode = DVDrawMode::Plugin;
        } else {
            m_drawMode = DVDrawMode::Type(mode);
        }
    }

    /* If there weren't any warnings we don't show the dialog. */
    if(!warning.isEmpty())
        /* TODO - Perhaps this should be done within QML? */
        QMessageBox::warning(nullptr, tr("Invalid Command Line!"), warning);
}

QString DVQmlCommunication::versionString() {
    return version::number.toString();
}

QString DVQmlCommunication::buildType() {
    return version::build_type;
}

QString DVQmlCommunication::buildCompiler() {
    return version::compiler;
}
