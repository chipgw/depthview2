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
#include <QtMath>
#include <QSGTextureProvider>

namespace {
const QMap<QString, QString> themes = {{"Material Design (Default)","Material"},{"QML Default Style", "Default"},{"Universal Design","Universal"}};
}

DVQmlCommunication::DVQmlCommunication(QObject* parent, QSettings& s) : QObject(parent),
    settings(s), lastWindowState(Qt::WindowNoState), m_swapEyes(false), imageTarget(nullptr) {
    m_drawMode = DVDrawMode::fromString(settings.value("DrawMode", "Anaglyph").toByteArray());

    m_greyFacL = settings.value("GreyFacL", 0.0).toReal();
    m_greyFacR = settings.value("GreyFacR", 0.0).toReal();

    m_swapEyes = settings.value("SwapEyes", false).toBool();

    m_anamorphicDualView = settings.value("Anamorphic", false).toBool();

    m_mirrorLeft = settings.value("MirrorLeft", false).toBool();
    m_mirrorRight = settings.value("MirrorRight", false).toBool();

    /* This constructor gets called before QML is set up, so this works. */
    QQuickStyle::setStyle(themes.value(settings.value("ControlsTheme").toString(), "Material"));
}

void DVQmlCommunication::setDrawMode(DVDrawMode::Type mode) {
    /* Only emit if changed. */
    if (m_drawMode != mode) {
        m_drawMode = mode;
        settings.setValue("DrawMode", DVDrawMode::toString(mode));
        emit drawModeChanged();
    }
}

void DVQmlCommunication::initDrawMode(DVDrawMode::Type mode) {
    m_drawMode = mode;
    settings.setValue("DrawMode", DVDrawMode::toString(mode));
}

void DVQmlCommunication::setAnamorphicDualView(bool anamorphic) {
    /* Only emit if changed. */
    if (m_anamorphicDualView != anamorphic) {
        m_anamorphicDualView = anamorphic;
        settings.setValue("Anamorphic", anamorphic);
        emit anamorphicDualViewChanged(anamorphic);
    }
}

void DVQmlCommunication::setMirrorLeft(bool mirror) {
    /* Only emit if changed. */
    if (m_mirrorLeft != mirror) {
        m_mirrorLeft = mirror;
        settings.setValue("MirrorLeft", mirror);
        emit mirrorLeftChanged(mirror);
    }
}

void DVQmlCommunication::setMirrorRight(bool mirror) {
    /* Only emit if changed. */
    if (m_mirrorRight != mirror) {
        m_mirrorRight = mirror;
        settings.setValue("MirrorRight", mirror);
        emit mirrorRightChanged(mirror);
    }
}

void DVQmlCommunication::setGreyFacL(qreal fac) {
    /* Limit to [0, 1] range. */
    fac = qBound(0.0, fac, 1.0);

    /* Only emit if changed. */
    if (fac != m_greyFacL) {
        m_greyFacL = fac;
        settings.setValue("GreyFacL", fac);
        emit greyFacLChanged(fac);
    }
}

void DVQmlCommunication::setGreyFacR(qreal fac) {
    /* Limit to [0, 1] range. */
    fac = qBound(0.0, fac, 1.0);

    /* Only emit if changed. */
    if (fac != m_greyFacR) {
        m_greyFacR = fac;
        settings.setValue("GreyFacR", fac);
        emit greyFacRChanged(fac);
    }
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
QString DVQmlCommunication::gitVersion() {
    return version::git_version;
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

bool DVQmlCommunication::hardwareAcceleratedVideo() const {
    /* Default value if it isn't set is false. */
    return settings.value("HWAcceleration").toBool();
}

void DVQmlCommunication::setHardwareAcceleratedVideo(bool on) {
    if (!settings.contains("HWAcceleration") || settings.value("HWAcceleration").toBool() != on) {
        settings.setValue("HWAcceleration", on);
        emit hardwareAcceleratedVideoChanged();
    }
}

QString DVQmlCommunication::uiTheme() const {
    /* Either get it from settings, or get the one currently being used. */
    return settings.value("ControlsTheme", themes.key(QQuickStyle::name())).toString();
}

QStringList DVQmlCommunication::uiThemes() const {
    return themes.keys();
}

void DVQmlCommunication::setUiTheme(QString theme) {
    /* Theme is not valid. */
    if (!themes.contains(theme)) return;

    /* If the setting didn't exist but the theme is already the current one, just save it in settings without doing anything else. */
    if (!settings.contains("ControlsTheme") && themes.value(theme) == QQuickStyle::name())
        settings.setValue("ControlsTheme", theme);
    else if ((!settings.contains("ControlsTheme") || settings.value("ControlsTheme").toString() != theme)) {
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

QSGTexture* DVQmlCommunication::openImageTexture() {
    return (imageTarget && imageTarget->isTextureProvider()) ? imageTarget->textureProvider()->texture() : nullptr;
}

void DVQmlCommunication::setSurroundPan(QPointF val) {
    if (val != m_surroundPan) {
        m_surroundPan.setX(val.x() - 360.0 * qFloor(val.x() / 360.0));
        m_surroundPan.setY(qBound(-89.0, val.y(), 89.0));

        emit surroundPanChanged();
    }
}

void DVQmlCommunication::setSurroundFOV(qreal val) {
    if (val != m_surroundFOV) {
        /* This is about the same limits as the value of zoom has [0.2, 4.0], based on the way it is converted. */
        m_surroundFOV = qBound(7.5, val, 105.0);

        emit surroundFOVChanged();
    }
}

/* Windows file association code. */
#ifdef DV_FILE_ASSOCIATION
/* Windows-specific code used for file association. */
#include <Windows.h>
#pragma comment(lib, "advapi32")

void addRegistryEntry(const QString& path, const QString& value, QString& error) {
    HKEY key;

    LSTATUS result = RegCreateKeyExA(HKEY_CURRENT_USER, LPCSTR(path.toLocal8Bit().constData()), 0, nullptr,
                                     REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &key, nullptr);

    if (result != ERROR_SUCCESS) {
        error += "<p>Error creating key \"" + path + "\"!</p>";
        return;
    }

    result = RegSetValueExA(key, nullptr, 0, REG_SZ, LPBYTE(value.toLocal8Bit().constData()), DWORD(value.size()));

    if (result != ERROR_SUCCESS)
        error += "<p>Error setting value for \"" + path + "\"!</p>";

    RegCloseKey(key);
}

void DVQmlCommunication::registerFileTypes() {
    QString error;

    QString progID = "chipgw.DepthView." + version::number.toString();

    addRegistryEntry("Software\\Classes\\.jps", progID, error);
    addRegistryEntry("Software\\Classes\\.pns", progID, error);

    /* TODO - Add video and standard image formats in a way that doesn't override the main association. */

    addRegistryEntry("Software\\Classes\\" + progID, "Stereo 3D Image", error);

    QString command = "\"" + QDir::toNativeSeparators(QApplication::applicationFilePath()) + "\" \"%1\"";
    addRegistryEntry("Software\\Classes\\" + progID + "\\shell\\open\\command", command, error);

    if (error.isNull())
        QMessageBox::information(nullptr, QObject::tr("Success!"), QObject::tr("Successfully associated .jps and .pns files with DepthView."));
    else
        QMessageBox::warning(nullptr, QObject::tr("Error setting file association!"), error);
}

#endif
