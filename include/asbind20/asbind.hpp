#ifndef ASBIND20_ASBIND_HPP
#define ASBIND20_ASBIND_HPP

#pragma once

// clang-format off: Used by CMakeLists.txt for parsing version

#define ASBIND20_VERSION_MAJOR 1
#define ASBIND20_VERSION_MINOR 2
#define ASBIND20_VERSION_PATCH 0

// clang-format on

#define ASBIND20_VERSION_STRING "1.2.0"

#include "utility.hpp"
#include "bind.hpp"
#include "invoke.hpp"
#include "builder.hpp"

namespace asbind20
{
[[nodiscard]]
const char* library_version() noexcept;

/**
 * @brief Check if `asGetLibraryOptions()` returns "AS_MAX_PORTABILITY"
 */
[[nodiscard]]
bool has_max_portability(
    const char* options = AS_NAMESPACE_QUALIFIER asGetLibraryOptions()
);

/**
 * @brief Check if `asGetLibraryOptions()` doesn't return "AS_NO_EXCEPTION"
 */
[[nodiscard]]
bool has_exceptions(
    const char* options = AS_NAMESPACE_QUALIFIER asGetLibraryOptions()
);
} // namespace asbind20

#endif
