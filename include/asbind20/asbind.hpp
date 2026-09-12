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

#define ASBIND20_VERSION_MAJOR 2
#define ASBIND20_VERSION_MINOR 1
#define ASBIND20_VERSION_PATCH 0

// clang-format on

#define ASBIND20_VERSION_STRING "2.1.0"

// IWYU pragma: begin_exports

#include <cstring>
#include "detail/config.hpp"
#include "detail/include_as.hpp"
#include "utility.hpp"
#include "policies.hpp"
#include "bind.hpp"
#include "invoke.hpp"
#include "debugging.hpp"
#include "io/stream.hpp"
#include "io/to_string.hpp"

// IWYU pragma: end_exports

namespace asbind20
{
/**
 * @brief Get the string representing library version and configuration
 */
[[nodiscard]]
constexpr const char* library_version() noexcept
{
    constexpr const char* ver_str =
        ASBIND20_VERSION_STRING
    // TODO: Rename the macro to ASBIND20_CONFIG_NO_RUNTIME_BIND_CHECKS
#ifdef ASBIND20_CONFIG_NO_THROW_ON_BAD_BINDING
        " NO_RUNTIME_BIND_CHECKS"
#endif
#ifdef ASBIND20_CONFIG_NO_COMPILE_TIME_CHECKS
        " NO_COMPTIME_CHECKS"
#endif
#ifndef NDEBUG
        " DEBUG"
#endif
        ;

    return ver_str;
}

[[nodiscard]]
inline const char* script_library_version()
{
    return AS_NAMESPACE_QUALIFIER asGetLibraryVersion();
}

[[nodiscard]]
inline const char* script_library_options()
{
    return AS_NAMESPACE_QUALIFIER asGetLibraryOptions();
}

/**
 * @brief Check if `asGetLibraryOptions()` returns `"AS_MAX_PORTABILITY"`
 */
[[nodiscard]]
inline bool has_max_portability(
    const char* options = script_library_options()
)
{
    return std::strstr(options, "AS_MAX_PORTABILITY") != nullptr;
}

/**
 * @brief Check if `asGetLibraryOptions()` doesn't return `"AS_NO_EXCEPTIONS"`
 */
[[nodiscard]]
inline bool has_exceptions(
    const char* options = script_library_options()
)
{
    return std::strstr(options, "AS_NO_EXCEPTIONS") == nullptr;
}

/**
 * @brief Check if `asGetLibraryOptions()` doesn't return `"AS_NO_THREADS"`
 */
[[nodiscard]]
inline bool has_threads(
    const char* options = script_library_options()
)
{
    return std::strstr(options, "AS_NO_THREADS") == nullptr;
}

/**
 * @brief Check if `asGetLibraryVersion()` contains "DEBUG"
 */
[[nodiscard]]
inline bool script_debug_mode(
    const char* version = script_library_version()
)
{
    return std::strstr(version, "DEBUG") != nullptr;
}
} // namespace asbind20

#endif
