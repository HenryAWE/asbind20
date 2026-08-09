#ifndef ASBIND20_UTIL_UNREACHABLE_HPP
#define ASBIND20_UTIL_UNREACHABLE_HPP

#include <version>
#include <utility>

namespace asbind20::util
{
/**
 * @brief Backport of C++23 `std::unreachable()`
 */
[[noreturn]]
inline void unreachable()
{
#ifdef __cpp_lib_unreachable
    std::unreachable();
#elif defined _MSC_VER
    __assume(false);
#elif defined __GNUC__ || defined __clang__
    __builtin_unreachable();
#else
    // According to cppreference,
    // an empty function body and the [[noreturn]] attribute
    // is enough to raise undefined behavior.

#    ifndef NDEBUG
    std::terminate();
#    endif
#endif
}
} // namespace asbind20::detail

#endif
