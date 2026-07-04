#ifndef ASBIND20_RANGES_COMMON_HPP
#define ASBIND20_RANGES_COMMON_HPP

#pragma once

#include <version>
// IWYU pragma: begin_exports
#include <iterator>
#ifdef __cpp_lib_ranges
#    define ASBIND20_HAS_LIB_RANGES __cpp_lib_ranges
#    include <ranges>
#endif
// IWYU pragma: end_exports

namespace asbind20::ranges
{
namespace detail
{
#ifdef ASBIND20_HAS_LIB_RANGES
    template <typename View>
    using view_interface = std::ranges::view_interface<View>;

#else
    // Placeholder if standard library doesn't provide ranges library.
    // Because this library is designed to support old toolchain like LLVM-15.
    template <typename View>
    class view_interface
    {};

#endif
} // namespace detail
} // namespace asbind20::ranges

#endif
