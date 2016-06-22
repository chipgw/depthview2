#pragma once

#include <QString>
#include <QVersionNumber>

namespace version {
    extern const QVersionNumber number;
    extern const char* build_type;
    extern const char* compiler;

    /* TODO - Figure out how to get qmake to throw the git sha1 in here... */
}

#define DV_URI_VERSION "DepthView", version::number.majorVersion(), version::number.minorVersion()
