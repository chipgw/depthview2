#include <QApplication>
#include <QDir>
#include <version.hpp>
#include "dvwindow.hpp"

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

    /* If not started in a specific directory default to the user's home path. */
    if(QDir::currentPath() == app.applicationDirPath())
        QDir::setCurrent(QDir::homePath());

    DVWindow window;
    window.show();

    return app.exec();
}
