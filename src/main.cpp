#include <QApplication>
#include "dvwindow.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    DVWindow window;
    window.show();

    return app.exec();
}
