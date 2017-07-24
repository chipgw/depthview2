#pragma once

#include <QString>
#include <QVersionNumber>

namespace version {
    extern const QVersionNumber number;
    extern const char* build_type;
    extern const char* compiler;
    extern const char* git_version;
}

#define DV_URI_VERSION "DepthView", version::number.majorVersion(), version::number.minorVersion()
