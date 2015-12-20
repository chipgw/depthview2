TEMPLATE = subdirs

# Only build test plugin in debug mode.
CONFIG(debug,debug|release) {
    SUBDIRS += testplugin
}
