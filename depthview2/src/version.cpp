#include "version.hpp"

namespace version {

const QVersionNumber number = QVersionNumber(2, 0);

#ifdef NDEBUG
const char* build_type = "Release";
#else
const char* build_type = "Debug";
#endif

#if defined(__clang__)
const char* compiler = "Clang/LLVM";
#elif defined(__ICC) || defined(__INTEL_COMPILER)
const char* compiler = "Intel ICC/ICPC";
#elif defined(__GNUC__) || defined(__GNUG__)
const char* compiler = "GNU GCC/G++";
#elif defined(__HP_cc) || defined(__HP_aCC)
const char* compiler = "Hewlett-Packard C/aC++";
#elif defined(__IBMC__) || defined(__IBMCPP__)
const char* compiler = "IBM XL C/C++";
#elif defined(_MSC_VER)
const char* compiler = "Microsoft Visual Studio";
#elif defined(__PGI)
const char* compiler = "Portland Group PGCC/PGCPP";
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
const char* compiler = "Oracle Solaris Studio";
#else
const char* compiler = "Unknown";
#endif

const char* git_version = GIT_VERSION;

}
