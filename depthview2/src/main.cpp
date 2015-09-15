#include <QApplication>
#include "dvwindow.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    /* QML needs a stencil buffer. */
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);

    DVWindow window;
    window.show();

    return app.exec();
}
