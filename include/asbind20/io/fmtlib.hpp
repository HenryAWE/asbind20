/**
 * @brief Compatibility layer across different format libraries
 */

#ifndef ASBIND20_IO_FMTLIB_HPP
#define ASBIND20_IO_FMTLIB_HPP

#pragma once

#include <string_view>
#include "../detail/config.hpp"

#ifdef ASBIND20_HAS_LIB_FORMAT

#    include <format>

namespace asbind20::io
{
namespace detail
{
    template <typename CharT, std::size_t SizeChar, std::size_t SizeWChar>
    consteval decltype(auto) statically_widen(
        const char (&str)[SizeChar], const wchar_t (&wstr)[SizeWChar]
    )
    {
        if constexpr(std::same_as<CharT, wchar_t>)
            return wstr;
        else
            return str;
    }
} // namespace detail

#    define ASBIND20_IO_STATICALLY_WIDEN(char_t, str) \
        (::asbind20::io::detail::statically_widen<char_t>(str, L##str))

#    if __cpp_lib_format >= 202207L
template <typename... Args>
using format_string = std::format_string<Args...>;

namespace detail
{
    constexpr std::string_view fmt_string_to_view(const auto& fmtstr)
    {
        return fmtstr.get();
    }
} // namespace detail

#    else
template <typename... Args>
using format_string = std::string_view;

namespace detail
{
    constexpr std::string_view fmt_string_to_view(const auto& fmtstr)
    {
        return fmtstr;
    }
} // namespace detail
#    endif

} // namespace asbind20::io

#endif

// TODO: Allow users to choose {fmt} as a formatting backend

#endif
