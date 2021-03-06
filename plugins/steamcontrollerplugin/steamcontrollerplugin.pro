#-------------------------------------------------
#
# Project created by QtCreator 2015-09-14T17:14:41
#
#-------------------------------------------------

TARGET = dv2_steamcontrollerplugin
include(../plugin.pri)

SOURCES += steamcontrollerplugin.cpp

HEADERS += steamcontrollerplugin.hpp

win32: {
    contains(QMAKE_TARGET.arch, x86_64): LIBS += -L$$PWD/../../../steamsdk/redistributable_bin/win64/ -lsteam_api64
    else: LIBS += -L$$PWD/../../../steamsdk/redistributable_bin/ -lsteam_api
}else{
    LIBS += -L$$PWD/../../../steamsdk/redistributable_bin/linux64/ -L$$PWD/../../../steamsdk/redistributable_bin/linux32/ -lsteam_api
}

INCLUDEPATH += $$PWD/../../../steamsdk/public/steam
DEPENDPATH += $$PWD/../../../steamsdk/public/steam

DISTFILES += steamcontrollerplugin.json
