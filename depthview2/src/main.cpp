#include <QApplication>
#include <QDir>
#include <QCommandLineParser>
#include <QMessageBox>
#include "version.hpp"
#include "dvwindow.hpp"

void registerFileTypes();

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setOrganizationName("chipgw");
    app.setApplicationName("DepthView2");
    app.setApplicationVersion(version::number.toString());

    /* QML needs a stencil buffer. */
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);

    QCommandLineParser parser;

    /* TODO - More arguments. */
    parser.addOptions({
            {"register", QCoreApplication::translate("main", "")},
            { {"f", "fullscreen"},
              QCoreApplication::translate("main", "")},
            { {"d", "startdir"},
              QCoreApplication::translate("main", ""),
              QCoreApplication::translate("main", "directory")},
            { {"r", "renderer"},
              QCoreApplication::translate("main", ""),
              QCoreApplication::translate("main", "renderer")},
        });

    parser.parse(app.arguments());

    if(parser.isSet("register")){
        registerFileTypes();
        return 0;
    }

    /* If not started in a specific directory default to the user's home path. */
    if(QDir::currentPath() == app.applicationDirPath())
        QDir::setCurrent(QDir::homePath());

    DVWindow window;
    window.show();
    window.doCommandLine(parser);

    return app.exec();
}

/* File association code. */

#if defined(Q_OS_WIN32)
/* Windows-specific code used for file association. */
#include <Windows.h>
#pragma comment(lib, "advapi32")

void addRegistryEntry(const QString& path, const QString& value, QString& error){
    /* TODO - handle errors properly. */
    HKEY key;

    LSTATUS result = RegCreateKeyExA(HKEY_CURRENT_USER, LPCSTR(path.toLocal8Bit().constData()), 0, nullptr,
                                     REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &key, nullptr);

    if (result == ERROR_SUCCESS) {
        result = RegSetValueExA(key, nullptr, 0, REG_SZ, LPBYTE(value.toLocal8Bit().constData()), DWORD(value.size()));
        if (result != ERROR_SUCCESS)
            error += "<p>Error setting value for \"" + path + "\"!</p>";

        RegCloseKey(key);
    } else {
        error += "<p>Error creating key \"" + path + "\"!</p>";
    }
}
#endif

void registerFileTypes() {
    QString error;

#if defined(Q_OS_WIN32)
    QString progID = "chipgw.DepthView." + version::number.toString();

    addRegistryEntry("Software\\Classes\\.jps", progID, error);
    addRegistryEntry("Software\\Classes\\.pns", progID, error);

    /* TODO - Add video formats. */

    addRegistryEntry("Software\\Classes\\" + progID, "Stereo 3D Image", error);

    QString command = "\"" + QDir::toNativeSeparators(QApplication::applicationFilePath()) + "\" \"%1\"";
    addRegistryEntry("Software\\Classes\\" + progID + "\\shell\\open\\command", command, error);

#else
    /* TODO - make other platforms work. */
    error = tr("File association is currently unsupported on your platform!");
#endif

    if (error.isNull())
        QMessageBox::information(nullptr, QObject::tr("Success!"), QObject::tr("Successfully associated .jps and .pns files with DepthView."));
    else
        QMessageBox::warning(nullptr, QObject::tr("Error setting file association!"), error);
}
