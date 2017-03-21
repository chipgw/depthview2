#-------------------------------------------------
#
# Project created by QtCreator 2015-09-14T17:14:41
#
#-------------------------------------------------

TARGET = dv2_steamcontrollerplugin
TEMPLATE = lib

QT += qml quick

CONFIG += plugin C++11

SOURCES += steamcontrollerplugin.cpp

HEADERS += steamcontrollerplugin.hpp

# This is relative to build directory.
DESTDIR       = ../../depthview2/plugins

# Relative to project directory, needed for "dvinputplugin.hpp" and "dvinputinterface.hpp".
INCLUDEPATH += ../../depthview2/include

RESOURCES += steamcontrollerplugin.qrc

win32: {
    contains(QMAKE_TARGET.arch, x86_64): {
        LIBS += -L$$PWD/../../../steamsdk/redistributable_bin/win64/ -lsteam_api64
    } else {
        LIBS += -L$$PWD/../../../steamsdk/redistributable_bin/ -lsteam_api
    }
}else:LIBS += -L$$PWD/../../../steamsdk/redistributable_bin/linux32/ -L$$PWD/../../../steamsdk/redistributable_bin/linux64/ -lsteam_api

INCLUDEPATH += $$PWD/../../../steamsdk/public/steam
DEPENDPATH += $$PWD/../../../steamsdk/public/steam

DISTFILES += \
    steamcontrollerplugin.json
