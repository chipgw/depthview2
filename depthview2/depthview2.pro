TEMPLATE = app

QT += qml quick widgets sql av quickcontrols2

SOURCES += src/main.cpp \
    src/dvwindow.cpp \
    src/dvwindow_gl.cpp \
    src/dvqmlcommunication.cpp \
    src/version.cpp \
    src/dvfolderlisting.cpp \
    src/dvthumbnailprovider.cpp \
    src/fileassociation.cpp \
    src/dvpluginmanager.cpp \
    src/dvfilevalidator.cpp

RESOURCES += qml.qrc

# Add "CONFIG+=portable" (with quotes) to qmake arguments to enable portable build.
portable:DEFINES += DV_PORTABLE

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    include/dvwindow.hpp \
    include/dvenums.hpp \
    include/dvqmlcommunication.hpp \
    include/dvinputplugin.hpp \
    include/dvrenderplugin.hpp \
    include/version.hpp \
    include/dvfolderlisting.hpp \
    include/dvinputinterface.hpp \
    include/dvthumbnailprovider.hpp \
    include/fileassociation.hpp \
    include/dvpluginmanager.hpp \
    include/dvrenderinterface.hpp \
    include/dvfilevalidator.hpp

INCLUDEPATH += include

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

RC_FILE = depthview2.rc
