TEMPLATE = app

QT += qml quick widgets

CONFIG += C++11

SOURCES += src/main.cpp \
    src/dvwindow.cpp \
    src/dvqmlcommunication.cpp \
    src/dvshortcut.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    include/dvwindow.hpp \
    include/dvqmlcommunication.hpp \
    include/dvshortcut.hpp

INCLUDEPATH += include
