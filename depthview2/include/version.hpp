#pragma once

#include <QString>

namespace version{
    extern const int major;
    extern const int minor;
    extern const char* build_type;
    extern const char* compiler;

    const QString versionString();

    /* TODO - Figure out how to get qmake to throw the git sha1 in here... */
}

#define DV_URI_VERSION "DepthView", version::major, version::minor
