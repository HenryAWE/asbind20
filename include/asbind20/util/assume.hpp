#ifndef ASBIND20_UTIL_ASSUME_HPP
#define ASBIND20_UTIL_ASSUME_HPP

#include <version>

#if __has_cpp_attribute(assume)
#    define ASBIND20_ASSUME(expr) [[assume(expr)]]

// Fallback implementation from P1774R8
// See: https://wg21.link/P1774
#elif defined(__clang__)
#    define ASBIND20_ASSUME(expr) __builtin_assume(expr)
#elif defined(__GNUC__) && !defined(__ICC)
// clang-format off
#    define ASBIND20_ASSUME(expr) if (expr) {} else { __builtin_unreachable(); }
// clang-format on
#elif defined(_MSC_VER) || defined(__ICC)
#    define ASBIND20_ASSUME(expr) __assume(expr)
#else
#    define ASBIND20_ASSUME(expr)
#endif


#endif
