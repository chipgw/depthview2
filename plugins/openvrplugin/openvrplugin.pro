#------------------------------#
# OpenVR plugin for DepthView2 #
#  (Works with OpenVR v1.0.5)  #
#------------------------------#

TARGET = dv2_openvrplugin
TEMPLATE = lib

QT += qml quick

CONFIG += plugin C++11

SOURCES += openvrplugin.cpp

HEADERS += openvrplugin.hpp

# This is relative to build directory.
DESTDIR       = ../../depthview2/plugins

# Relative to project directory, needed for "dvrenderplugin.hpp".
INCLUDEPATH += ../../depthview2/include

RESOURCES += openvrplugin.qrc

# Adding this gets it to build on Linux, but I haven't been able to get it running, even with just the null driver.
# That's probably because to my knowledge Valve hasn't properly updated Linux SteamVR in a while...
linux-g++: DEFINES += COMPILER_GCC

win32: {
    contains(QMAKE_TARGET.arch, x86_64): {
        LIBS += -L$$PWD/../../../openvr/lib/win64/
    } else {
        LIBS += -L$$PWD/../../../openvr/lib/win32/
    }
}
LIBS += -L$$PWD/../../../openvr/lib/linux32/ -L$$PWD/../../../openvr/lib/linux64/ -lopenvr_api

INCLUDEPATH += $$PWD/../../../openvr/headers
DEPENDPATH += $$PWD/../../../openvr/headers

DISTFILES += \
    openvrplugin.json
