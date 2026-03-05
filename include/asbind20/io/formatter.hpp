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
    OutputIt copy_cstr_to(const char* cstr, OutputIt out)
    {
        for(const char* p = cstr; *p != '\0'; ++p)
        {
            *out = *p;
            ++out;
        }

        return out;
    }

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

#define ASBIND20_IO_STATICALLY_WIDEN(char_t, str) \
    (::asbind20::io::detail::statically_widen<char_t>(str, L##str))

    template <typename CharT, typename OutputIt>
    auto output_fallback(OutputIt out, std::string_view type_name, int underlying)
    {
        out = std::copy(type_name.cbegin(), type_name.cend(), out);

#ifdef ASBIND20_HAS_LIB_FORMAT
        return std::format_to(
            std::move(out), ASBIND20_IO_STATICALLY_WIDEN(CharT, "({})"), underlying
        );

#else
        *out = '(';
        ++out;
        {
            auto str = std::to_string(underlying);
            out = std::copy(str.cbegin(), str.cend(), out);
        }
        *out = ')';
        ++out;

        return out;
#endif
    }
} // namespace detail

template <typename CharT = char, typename OutputIt>
auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asEContextState state, bool skip_prefix, OutputIt out
)
{
    const char* cstr = asbind20::detail::state_to_cstr(state);
    if(!cstr) [[unlikely]]
    {
        using namespace std::string_view_literals;
        return detail::output_fallback<CharT>(
            out, "asEContextState"sv, static_cast<int>(state)
        );
    }

    if(skip_prefix)
        cstr += sizeof("asEXECUTION"); // Skip "asEXECUTION_"
    return detail::copy_cstr_to(cstr, std::move(out));
}

template <typename CharT = char, typename OutputIt>
auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asERetCodes ret, bool skip_prefix, OutputIt out
)
{
    const char* cstr = asbind20::detail::ret_to_cstr(ret);
    if(!cstr) [[unlikely]]
    {
        using namespace std::string_view_literals;
        return detail::output_fallback<CharT>(
            out, "asERetCodes"sv, static_cast<int>(ret)
        );
    }

    if(skip_prefix)
        cstr += 2; // Skip "as"
    return detail::copy_cstr_to(cstr, std::move(out));
}

template <typename CharT = char, typename OutputIt>
auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asEMsgType msg_type, bool skip_prefix, OutputIt out
)
{
    const char* cstr = asbind20::detail::msg_type_to_cstr(msg_type);
    if(!cstr) [[unlikely]]
    {
        using namespace std::string_view_literals;
        return detail::output_fallback<CharT>(
            out, "asEMsgType"sv, static_cast<int>(msg_type)
        );
    }

    if(skip_prefix)
        cstr += sizeof("asMSGTYPE"); // Skip "asMSGTYPE_"
    return detail::copy_cstr_to(cstr, std::move(out));
}

template <typename CharT, typename OutputIt>
auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asETokenClass tc, bool skip_prefix, OutputIt out
)
{
    const char* cstr = asbind20::detail::tc_to_cstr(tc);
    if(!cstr) [[unlikely]]
    {
        using namespace std::string_view_literals;
        return detail::output_fallback<CharT>(
            out, "asETokenClass"sv, static_cast<int>(tc)
        );
    }

    if(skip_prefix)
        cstr += sizeof("asTC"); // Skip "asTC_"
    return detail::copy_cstr_to(cstr, std::move(out));
}

class angelscript_enum_formatter_base
{
public:
    constexpr angelscript_enum_formatter_base() noexcept = default;
    constexpr angelscript_enum_formatter_base(const angelscript_enum_formatter_base&) noexcept = default;

    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        using char_type = typename ParseContext::char_type;

        auto it = ctx.begin();
        if(it == ctx.end() || *it == char_type('}'))
            return it;

        if(char_type ch = *it; ch == char_type('?'))
        {
            full_representation = true;
            ++it;
        }
        else if(ch == char_type('d'))
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
} // namespace asbind20::io

#ifdef ASBIND20_HAS_LIB_FORMAT

template <typename ASEnum, typename CharT>
requires(
    std::same_as<ASEnum, AS_NAMESPACE_QUALIFIER asEContextState> ||
    std::same_as<ASEnum, AS_NAMESPACE_QUALIFIER asERetCodes> ||
    std::same_as<ASEnum, AS_NAMESPACE_QUALIFIER asEMsgType> ||
    std::same_as<ASEnum, AS_NAMESPACE_QUALIFIER asETokenClass>
)
class std::formatter<ASEnum, CharT> :
    public asbind20::io::angelscript_enum_formatter_base
{
public:
    constexpr formatter() noexcept = default;
    constexpr formatter(const formatter&) noexcept = default;

    template <typename FormatContext>
    constexpr auto format(ASEnum val, FormatContext& ctx) const
        -> typename FormatContext::iterator
    {
        if(this->print_underlying)
        {
            return std::format_to(
                ctx.out(), ASBIND20_IO_STATICALLY_WIDEN(char, "{:d}"), static_cast<int>(val)
            );
        }

        return asbind20::io::copy_debug_representation_to<CharT>(
            val, !this->full_representation, ctx.out()
        );
    }
};

#endif

#undef ASBIND20_IO_STATICALLY_WIDEN

#endif
