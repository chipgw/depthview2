#-------------------------------------------------
#
# Project created by QtCreator 2015-09-14T17:14:41
#
#-------------------------------------------------

TARGET = dv2_testplugin
TEMPLATE = lib

CONFIG += plugin C++11

SOURCES += testplugin.cpp

HEADERS += testplugin.hpp

# This is relative to build directory.
DESTDIR       = ../../depthview2/plugins

# Relative to project directory, needed for "dvrenderplugin.hpp".
INCLUDEPATH += ../../depthview2/include

RESOURCES += testplugin.qrc
