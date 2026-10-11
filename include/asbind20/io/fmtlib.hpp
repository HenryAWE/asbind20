/**
 * @brief Compatibility layer across different format libraries
 */

#ifndef ASBIND20_IO_FMTLIB_HPP
#define ASBIND20_IO_FMTLIB_HPP

#pragma once

#include <string_view>
#include "../detail/config.hpp"

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

#define ASBIND20_IO_STATICALLY_WIDEN(char_t, str) \
    (::asbind20::io::detail::statically_widen<char_t>(str, L##str))
} // namespace asbind20::io

#if defined(ASBIND20_HAS_LIB_FORMAT) && !defined(ASBIND20_USE_FMTLIB)

#    define ASBIND20_HAS_FORMAT_LIBRARY "<format>"

#    include <format>

namespace asbind20::io
{
namespace fmtlib
{
#    if __cpp_lib_format >= 202207L
    template <typename... Args>
    using format_string = std::format_string<Args...>;

    constexpr std::string_view format_string_to_view(const auto& fmtstr)
    {
        return fmtstr.get();
    }

#    else
    template <typename... Args>
    using format_string = std::string_view;

    constexpr std::string_view format_string_to_view(const auto& fmtstr)
    {
        return fmtstr;
    }
#    endif

    using ::std::format;
    using ::std::format_args;
    using ::std::format_to;
    using ::std::make_format_args;
    using ::std::vformat;
} // namespace fmtlib
} // namespace asbind20::io

#endif

// Allow users to choose {fmt} as a formatting backend
#if defined(ASBIND20_USE_FMTLIB)

#    define ASBIND20_HAS_FORMAT_LIBRARY "{fmt}"

#    include <fmt/format.h>
#    include <fmt/xchar.h>

namespace asbind20::io
{
namespace fmtlib
{
    using ::fmt::format;
    using ::fmt::format_args;
    using ::fmt::format_string;
    using ::fmt::format_to;
    using ::fmt::make_format_args;
    using ::fmt::vformat;

    constexpr std::string_view format_string_to_view(const auto& fmtstr)
    {
        // {fmt} 10 added `get()`, while {fmt} 12.2 deprecated the implicit
        // conversion to `string_view`, so prefer `get()` when it is available.
        if constexpr(requires { fmtstr.get(); })
        {
            const ::fmt::string_view view = fmtstr.get();
            return {view.data(), view.size()};
        }
        else
        {
            const ::fmt::string_view view = fmtstr;
            return {view.data(), view.size()};
        }
    }
} // namespace fmtlib
} // namespace asbind20::io

#endif

#endif
