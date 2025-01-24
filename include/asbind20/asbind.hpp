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
} // namespace asbind20

#endif
