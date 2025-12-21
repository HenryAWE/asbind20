#ifndef ASBIND20_RANGES_COMMON_HPP
#define ASBIND20_RANGES_COMMON_HPP

#pragma once

#include <version>
#include <iterator>
#ifdef __cpp_lib_ranges
#    define ASBIND20_HAS_LIB_RANGES __cpp_lib_ranges
#    include <ranges>
#endif

namespace asbind20::ranges
{
namespace detail
{
#ifdef ASBIND20_HAS_LIB_RANGES
    template <typename View>
    using view_interface = std::ranges::view_interface<View>;

#else
    // Placeholder if standard library doesn't provide ranges library
    template <typename View>
    class view_interface
    {};

#endif
} // namespace detail
} // namespace asbind20::ranges

#endif
