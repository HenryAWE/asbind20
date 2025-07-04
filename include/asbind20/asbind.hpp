/**
 * @file asbind.hpp
 * @author HenryAWE
 * @brief C++20 AngelScript binding library
 */

#ifndef ASBIND20_ASBIND_HPP
#define ASBIND20_ASBIND_HPP

#pragma once

#include <cstring>

// clang-format off: Used by CMakeLists.txt for parsing version

#define ASBIND20_VERSION_MAJOR 1
#define ASBIND20_VERSION_MINOR 7
#define ASBIND20_VERSION_PATCH 0

// clang-format on

#define ASBIND20_VERSION_STRING "1.7.0"

// IWYU pragma: begin_exports

#include <cstring>
#include "detail/include_as.hpp"
#include "utility.hpp"
#include "bind.hpp"
#include "invoke.hpp"

// IWYU pragma: end_exports

namespace asbind20
{
/**
 * @brief Get the string representing library version and configuration
 */
[[nodiscard]]
constexpr const char* library_version() noexcept
{
#ifndef NDEBUG
    return ASBIND20_VERSION_STRING " DEBUG";
#else
    return ASBIND20_VERSION_STRING;
#endif
}

/**
 * @brief Check if `asGetLibraryOptions()` returns `"AS_MAX_PORTABILITY"`
 */
[[nodiscard]]
inline bool has_max_portability(
    const char* options = AS_NAMESPACE_QUALIFIER asGetLibraryOptions()
)
{
    return std::strstr(options, "AS_MAX_PORTABILITY") != nullptr;
}

/**
 * @brief Check if `asGetLibraryOptions()` doesn't return `"AS_NO_EXCEPTIONS"`
 */
[[nodiscard]]
inline bool has_exceptions(
    const char* options = AS_NAMESPACE_QUALIFIER asGetLibraryOptions()
)
{
    return std::strstr(options, "AS_NO_EXCEPTIONS") == nullptr;
}

/**
 * @brief Check if `asGetLibraryOptions()` doesn't return `"AS_NO_THREADS"`
 */
[[nodiscard]]
inline bool has_threads(
    const char* options = AS_NAMESPACE_QUALIFIER asGetLibraryOptions()
)
{
    return std::strstr(options, "AS_NO_THREADS") == nullptr;
}
} // namespace asbind20

#endif
