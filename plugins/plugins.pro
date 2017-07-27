TEMPLATE = subdirs

# Only build test plugin in debug mode.
CONFIG(debug,debug|release) {
    # SUBDIRS += testplugin
}

# Add "PLUGINS=<plugins>" (with quotes) to qmake arguments to enable the plugin(s) in the specified folders.
SUBDIRS += $$PLUGINS
