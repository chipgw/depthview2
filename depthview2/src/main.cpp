#include <QApplication>
#include <QDir>
#include <QCommandLineParser>
#include "version.hpp"
#include "dvwindow.hpp"
#include "fileassociation.hpp"

int main(int argc, char* argv[]) {
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
#ifdef DV_FILE_ASSOCIATION
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

#ifdef DV_FILE_ASSOCIATION
    if (parser.isSet("register")){
        fileassociation::registerFileTypes();
        return 0;
    }
#endif

    DVWindow window;
    window.show();
    window.doCommandLine(parser);

    return app.exec();
}
