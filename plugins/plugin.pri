# Plugins are all libs, obviously.
TEMPLATE = lib

# All plugins must reference QML and Quick for their config pages.
QT += qml quick

# Common for all plugins.
CONFIG += plugin C++11

# This is relative to build directory.
DESTDIR = ../../depthview2/plugins

# Relative to project directory, needed for "dvinputplugin.hpp" and "dvinputinterface.hpp".
INCLUDEPATH += ../../depthview2/include
