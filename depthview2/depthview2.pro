TEMPLATE = app

QT += qml quick widgets sql av quickcontrols2

SOURCES += src/main.cpp \
    src/dvqmlcommunication.cpp \
    src/version.cpp \
    src/dvfolderlisting.cpp \
    src/dvthumbnailprovider.cpp \
    src/dvpluginmanager.cpp \
    src/dvfilevalidator.cpp \
    src/dvvirtualscreenmanager.cpp \
    src/dvwindowhook.cpp \
    src/dvrenderer.cpp

RESOURCES += qml.qrc

# Add "CONFIG+=portable" (with quotes) to qmake arguments to enable portable build.
portable:DEFINES += DV_PORTABLE

# Default rules for deployment.
include(deployment.pri)

include(../gitversion.pri)

HEADERS += \
    include/dvenums.hpp \
    include/dvqmlcommunication.hpp \
    include/dvinputplugin.hpp \
    include/version.hpp \
    include/dvfolderlisting.hpp \
    include/dvinputinterface.hpp \
    include/dvthumbnailprovider.hpp \
    include/dvpluginmanager.hpp \
    include/dvfilevalidator.hpp \
    include/dvconfig.hpp \
    include/dvvirtualscreenmanager.hpp \
    include/dv_vrdriver.hpp \
    include/dvwindowhook.hpp \
    include/dvrenderer.hpp

INCLUDEPATH += include

openvr: {
    win32: {
        contains(QMAKE_TARGET.arch, x86_64): LIBS += -L$$PWD/../../openvr/lib/win64/
        else: LIBS += -L$$PWD/../../openvr/lib/win32/
    }
    LIBS += -L$$PWD/../../openvr/lib/linux64/ -lopenvr_api

    INCLUDEPATH += $$PWD/../../openvr/headers
    DEPENDPATH += $$PWD/../../openvr/headers

    SOURCES += src/dv_vrdriver_openvr.cpp
    HEADERS += include/dv_vrdriver_openvr.hpp

    DEFINES += DV_OPENVR
}

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

RC_ICONS = icons/logo.ico

QMAKE_TARGET_DESCRIPTION = "3D Image and Video Player."
QMAKE_TARGET_COPYRIGHT = "\xa9 2015-2019 chipgw, released under the MIT license"
QMAKE_TARGET_PRODUCT = "DepthView"
