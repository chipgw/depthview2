TEMPLATE = subdirs

lessThan(QT_MAJOR_VERSION, 5) || lessThan(QT_MINOR_VERSION, 7): error("This program requires Qt 5.7 or later.")

SUBDIRS = depthview2 \
          plugins

DISTFILES += \
    LICENSE \
    README.md
