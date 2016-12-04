#-------------------------------------------------
#
# Project created by QtCreator 2015-09-14T17:14:41
#
#-------------------------------------------------

TARGET = dv2_gamepadplugin
TEMPLATE = lib

QT += qml quick gamepad

CONFIG += plugin C++11

SOURCES += gamepadplugin.cpp

HEADERS += gamepadplugin.hpp

# This is relative to build directory.
DESTDIR       = ../../depthview2/plugins

# Relative to project directory, needed for "dvinputplugin.hpp" and "dvinputinterface.hpp".
INCLUDEPATH += ../../depthview2/include

RESOURCES += gamepadplugin.qrc
