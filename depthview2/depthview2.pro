TEMPLATE = app

QT += qml quick widgets

CONFIG += C++11

SOURCES += src/main.cpp \
    src/dvwindow.cpp \
    src/dvqmlcommunication.cpp \
    src/version.cpp

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
    include/dvrenderplugin.hpp \
    include/version.hpp

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
