#pragma once

#include <qsystemdetection.h>

#if defined(Q_OS_WIN32) && !defined(DV_PORTABLE)

#define DV_FILE_ASSOCIATION

namespace fileassociation {
void registerFileTypes();
}

#endif
