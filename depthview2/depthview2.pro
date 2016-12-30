TEMPLATE = app

QT += qml quick widgets av

lessThan(QT_MAJOR_VERSION, 5) || lessThan(QT_MINOR_VERSION, 7): error("This program requires Qt 5.7 or later.")

SOURCES += src/main.cpp \
    src/dvwindow.cpp \
    src/dvwindow_plugin.cpp \
    src/dvqmlcommunication.cpp \
    src/version.cpp \
    src/dvfolderlisting.cpp \
    src/dvthumbnailprovider.cpp \
    src/dvthumbnailresponse.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

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
    include/dvthumbnailresponse.hpp

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
