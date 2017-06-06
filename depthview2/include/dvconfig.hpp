#pragma once

#ifndef __has_cpp_attribute
#define __has_cpp_attribute(X) 0
#endif

/* Find a fallthrough attribute to use. */
#if __has_cpp_attribute(fallthrough)
#define DV_FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#define DV_FALLTHROUGH [[gnu::fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough)
#define DV_FALLTHROUGH [[clang::fallthrough]]
#else
/* Just do nothing if none was found. */
#define DV_FALLTHROUGH
#endif
