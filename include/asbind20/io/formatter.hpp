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
#include "../detail/config.hpp"
#include "../detail/include_as.hpp"
#include "to_string.hpp"
#ifdef ASBIND20_HAS_LIB_FORMAT
#    include <format>
#endif

namespace asbind20::io
{
namespace detail
{
    template <typename OutputIt>
    auto output_fallback(OutputIt out, std::string_view type_name, int underlying)
    {
#ifdef ASBIND20_HAS_LIB_FORMAT
        return std::format_to(std::move(out), "{}({})", type_name, underlying);

#else
        out = std::copy(type_name.begin(), type_name.end(), out);
        *out = '(';
        ++out;
        {
            auto str = std::to_string(underlying);
            out = std::copy(str.begin(), str.end(), out);
        }

        *out = ')';
        ++out;

        return out;
#endif
    }

    template <typename OutputIt>
    OutputIt copy_cstr_to(const char* cstr, OutputIt out)
    {
        for(const char* p = cstr; *p != '\0'; ++p)
        {
            *out = *p;
            ++out;
        }

        return out;
    }
} // namespace detail

template <typename OutputIt>
auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asEContextState state, bool skip_prefix, OutputIt out
)
{
    const char* cstr = asbind20::detail::state_to_cstr(state);
    if(!cstr) [[unlikely]]
    {
        using namespace std::string_view_literals;
        return detail::output_fallback(
            out, "asEContextState"sv, static_cast<int>(state)
        );
    }

    if(skip_prefix)
        cstr += sizeof("asEXECUTION"); // Skip "asEXECUTION_"
    return detail::copy_cstr_to(cstr, std::move(out));
}

template <typename OutputIt>
auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asERetCodes ret, bool skip_prefix, OutputIt out
)
{
    const char* cstr = asbind20::detail::ret_to_cstr(ret);
    if(!cstr) [[unlikely]]
    {
        using namespace std::string_view_literals;
        return detail::output_fallback(
            out, "asERetCodes"sv, static_cast<int>(ret)
        );
    }

    if(skip_prefix)
        cstr += 2; // Skip "as"
    return detail::copy_cstr_to(cstr, std::move(out));
}

template <typename OutputIt>
auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asEMsgType msg_type, bool skip_prefix, OutputIt out
)
{
    const char* cstr = asbind20::detail::msg_type_to_cstr(msg_type);
    if(!cstr) [[unlikely]]
    {
        using namespace std::string_view_literals;
        return detail::output_fallback(
            out, "asEMsgType"sv, static_cast<int>(msg_type)
        );
    }

    if(skip_prefix)
        cstr += sizeof("asMSGTYPE"); // Skip "asMSGTYPE_"
    return detail::copy_cstr_to(cstr, std::move(out));
}

template <typename OutputIt>
auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asETokenClass tc, bool skip_prefix, OutputIt out
)
{
    const char* cstr = asbind20::detail::tc_to_cstr(tc);
    if(!cstr) [[unlikely]]
    {
        using namespace std::string_view_literals;
        return detail::output_fallback(
            out, "asETokenClass"sv, static_cast<int>(tc)
        );
    }

    if(skip_prefix)
        cstr += sizeof("asTC"); // Skip "asTC_"
    return detail::copy_cstr_to(cstr, std::move(out));
}
} // namespace asbind20::io

#ifdef ASBIND20_HAS_LIB_FORMAT

class angelscript_enum_formatter_base
{
public:
    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto it = ctx.begin();
        if(it == ctx.end() || *it == '}')
            return it;

        if(char ch = *it; ch == '?')
        {
            full_representation = true;
            ++it;
        }
        else if(ch == 'd')
        {
            print_underlying = true;
            ++it;
        }

        return it;
    }

protected:
    bool full_representation = false;
    bool print_underlying = false;
};

template <typename ASEnum>
requires(
    std::same_as<ASEnum, AS_NAMESPACE_QUALIFIER asEContextState> ||
    std::same_as<ASEnum, AS_NAMESPACE_QUALIFIER asERetCodes> ||
    std::same_as<ASEnum, AS_NAMESPACE_QUALIFIER asEMsgType> ||
    std::same_as<ASEnum, AS_NAMESPACE_QUALIFIER asETokenClass>
)
class std::formatter<ASEnum> :
    public angelscript_enum_formatter_base
{
public:
    auto format(ASEnum val, std::format_context& ctx) const
        -> std::format_context::iterator
    {
        if(this->print_underlying)
        {
            return std::format_to(
                ctx.out(), "{:d}", static_cast<int>(val)
            );
        }

        return asbind20::io::copy_debug_representation_to(
            val, !this->full_representation, ctx.out()
        );
    }
};

#endif

#endif
