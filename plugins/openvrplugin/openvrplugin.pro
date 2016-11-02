#-------------------------------------------------
#
# Project created by QtCreator 2015-09-14T17:14:41
#
#-------------------------------------------------

TARGET = dv2_openvrplugin
TEMPLATE = lib

CONFIG += plugin C++11

SOURCES += openvrplugin.cpp

HEADERS += openvrplugin.hpp

# This is relative to build directory.
DESTDIR       = ../../depthview2/plugins

# Relative to project directory, needed for "dvrenderplugin.hpp".
INCLUDEPATH += ../../depthview2/include

RESOURCES += openvrplugin.qrc

unix|win32: LIBS += -L$$PWD/../../../openvr/lib/win64/ -lopenvr_api

INCLUDEPATH += $$PWD/../../../openvr/headers
DEPENDPATH += $$PWD/../../../openvr/headers
