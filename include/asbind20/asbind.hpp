#ifndef ASBIND20_ASBIND_HPP
#define ASBIND20_ASBIND_HPP

#pragma once

// clang-format off: Used by CMakeLists.txt for parsing version

#define ASBIND20_VERSION_MAJOR 0
#define ASBIND20_VERSION_MINOR 1
#define ASBIND20_VERSION_PATCH 0

// clang-format on

#define ASBIND20_VERSION_STRING "0.1.0"

#include "utility.hpp"
#include "bind.hpp"
#include "invoke.hpp"
#include "builder.hpp"

namespace asbind20
{
[[nodiscard]]
const char* library_version() noexcept;

[[nodiscard]]
const char* library_options() noexcept;
} // namespace asbind20

#endif
