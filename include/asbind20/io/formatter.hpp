/**
 * @file formatter.hpp
 * @author HenryAWE
 * @brief Formatter for printing AngelScript types.
 *        This header provides utility for implementing formatter even if <format> is not available,
 *        e.g., integration with 3rd party library like {fmt}.
 */

#ifndef ASBIND20_IO_FORMATTER_HPP
#define ASBIND20_IO_FORMATTER_HPP

#pragma once

#include <algorithm>
#include "../detail/include_as.hpp"
#include "to_string.hpp"

namespace asbind20::io
{
namespace detail
{
    template <typename OutputIt>
    auto output_fallback(OutputIt out, std::string_view type_name, int underlying)
    {
        out = std::copy(type_name.begin(), type_name.end(), out);
        *out = '(';
        ++out;

#ifdef ASBIND20_HAS_LIB_FORMAT
        out = std::format_to(out, "{}", underlying);
#else
        {
            auto str = std::to_string(underlying);
            out = std::copy(str.begin(), str.end(), out);
        }
#endif

        *out = ')';
        ++out;

        return out;
    }
} // namespace detail

template <typename OutputIt>
auto copy_debug_representation_to(AS_NAMESPACE_QUALIFIER asEContextState state, OutputIt out)
{
    const char* cstr = asbind20::detail::state_to_cstr(state);
    if(cstr)
    {
        for(const char* p = cstr; *p != '\0'; ++p)
        {
            *out = *p;
            ++out;
        }

        return std::move(out);
    }
    else
    {
        using namespace std::string_view_literals;
        return detail::output_fallback(
            out, "asEContextState"sv, static_cast<int>(state)
        );
    }
}

template <typename OutputIt>
auto copy_debug_representation_to(AS_NAMESPACE_QUALIFIER asERetCodes ret, OutputIt out)
{
    const char* cstr = asbind20::detail::ret_to_cstr(ret);
    if(cstr)
    {
        for(const char* p = cstr; *p != '\0'; ++p)
        {
            *out = *p;
            ++out;
        }

        return std::move(out);
    }
    else
    {
        using namespace std::string_view_literals;
        return detail::output_fallback(
            out, "asERetCodes"sv, static_cast<int>(ret)
        );
    }
}
} // namespace asbind20::io

#ifdef ASBIND20_HAS_LIB_FORMAT

template <>
struct std::formatter<AS_NAMESPACE_QUALIFIER asEContextState>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(AS_NAMESPACE_QUALIFIER asEContextState val, std::format_context& ctx) const
        -> std::format_context::iterator
    {
        return asbind20::io::copy_debug_representation_to(val, ctx.out());
    }
};

template <>
struct std::formatter<AS_NAMESPACE_QUALIFIER asERetCodes>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(AS_NAMESPACE_QUALIFIER asERetCodes val, std::format_context& ctx) const
        -> std::format_context::iterator
    {
        return asbind20::io::copy_debug_representation_to(val, ctx.out());
    }
};

#endif

#endif
