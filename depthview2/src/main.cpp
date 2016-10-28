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
#ifdef Q_OS_WIN32
            {"register", QCoreApplication::translate("main", "")},
#endif
            { {"f", "fullscreen"},
              QCoreApplication::translate("main", "Open the window in fullscreen mode.")},
            { {"d", "startdir"},
              QCoreApplication::translate("main", "Start the application in the specified directory. (Ignored if a file is opened)"),
              QCoreApplication::translate("main", "directory")},
            { {"r", "renderer"},
              /* TODO - List valid modes in help string. */
              QCoreApplication::translate("main", "Set the render mode."),
              QCoreApplication::translate("main", "renderer")},
        });

    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "A file to open.");

    parser.process(app.arguments());

#ifdef Q_OS_WIN32
    if(parser.isSet("register")){
        registerFileTypes();
        return 0;
    }
#endif

    /* If not started in a specific directory default to the user's home path. */
    if(QDir::currentPath() == app.applicationDirPath())
        QDir::setCurrent(QDir::homePath());

    DVWindow window;
    window.show();
    window.doCommandLine(parser);

    return app.exec();
}

/* Windows file association code. */
#ifdef Q_OS_WIN32
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

void registerFileTypes() {
    QString error;

    QString progID = "chipgw.DepthView." + version::number.toString();

    addRegistryEntry("Software\\Classes\\.jps", progID, error);
    addRegistryEntry("Software\\Classes\\.pns", progID, error);

    /* TODO - Add video formats. */

    addRegistryEntry("Software\\Classes\\" + progID, "Stereo 3D Image", error);

    QString command = "\"" + QDir::toNativeSeparators(QApplication::applicationFilePath()) + "\" \"%1\"";
    addRegistryEntry("Software\\Classes\\" + progID + "\\shell\\open\\command", command, error);

    if (error.isNull())
        QMessageBox::information(nullptr, QObject::tr("Success!"), QObject::tr("Successfully associated .jps and .pns files with DepthView."));
    else
        QMessageBox::warning(nullptr, QObject::tr("Error setting file association!"), error);
}
#endif
