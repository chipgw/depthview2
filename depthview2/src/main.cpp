#include <QApplication>
#include <QDir>
#include "dvwindow.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

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
