#include <QApplication>
#include <QDir>
#include <QCommandLineParser>
#include <QSurfaceFormat>
#include <QQmlApplicationEngine>
#include "version.hpp"
#include "dvwindowhook.hpp"
#include "dvqmlcommunication.hpp"

int main(int argc, char* argv[]) {
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
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

    parser.addOptions({
#ifdef DV_FILE_ASSOCIATION
            {"register", QCoreApplication::translate("main", "")},
#endif
            { {"f", "fullscreen"},
              QCoreApplication::translate("main", "Open the window in fullscreen mode.")},
            { {"d", "startdir"},
              QCoreApplication::translate("main", "Start the application in the specified directory. (Ignored if a file is opened)"),
              QCoreApplication::translate("main", "directory")},
            { {"r", "renderer"},
              QCoreApplication::translate("main", "Set the render mode."),
              QCoreApplication::translate("main", "renderer")},
            { {"l", "list-modes"},
              QCoreApplication::translate("main", "List valid render modes to console during startup.")}
        });

    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "A file to open.");

    parser.process(app.arguments());

    /* Print the valid modes to the console. */
    if (parser.isSet("l")) {
        QString modes;

        for (int i = 0; i < DVDrawMode::metaEnum().keyCount(); ++i)
            modes.append(DVDrawMode::metaEnum().key(i)).append(", ");

        /* remove the last ", " at the end. */
        modes.chop(2);

        qDebug("Valid render modes: %s", qPrintable(modes));

        return 0;
    }
#ifdef DV_FILE_ASSOCIATION
    if (parser.isSet("register")){
        DVQmlCommunication::registerFileTypes();
        return 0;
    }
#endif

    QQmlApplicationEngine applicationEngine;

    DVWindowHook windowHook(&applicationEngine);
    windowHook.doCommandLine(parser);

    return app.exec();
}
