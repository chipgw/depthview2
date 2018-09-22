#pragma once

/* Make sure we have a macro to use, even if it doesn't actually do anything. */
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

#if defined(Q_OS_WIN32) && !defined(DV_PORTABLE)
#define DV_FILE_ASSOCIATION
#endif

#include <cstddef>

template <typename T1, typename T2>
inline size_t constexpr offset_of(T1 T2::*member) {
    constexpr T2 object {};
    return size_t(&(object.*member)) - size_t(&object);
}
