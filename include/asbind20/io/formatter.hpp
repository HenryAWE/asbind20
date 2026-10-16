/**
 * @file formatter.hpp
 * @author HenryAWE
 * @brief Formatter for printing AngelScript types.
 *        This header provides utility for implementing formatter even if `\<format\>` is not available,
 *        e.g., integration with 3rd party library like {fmt}.
 */

#ifndef ASBIND20_IO_FORMATTER_HPP
#define ASBIND20_IO_FORMATTER_HPP

#pragma once

#include <cstddef>
#include <limits>
#include <charconv>
#include <algorithm>
#include <iterator>
#include "../detail/config.hpp"
#include "../detail/include_as.hpp"
#include "to_string.hpp"
#include "fmtlib.hpp"

namespace asbind20::io
{
namespace detail
{
    template <typename OutputIt>
    constexpr OutputIt copy_cstr_to(const char* cstr, OutputIt out)
    {
        for(const char* p = cstr; *p != '\0'; ++p)
        {
            *out = *p;
            ++out;
        }

        return out;
    }

    template <typename CharT, typename OutputIt>
    constexpr auto output_fallback(
        OutputIt out, std::string_view type_name, int underlying
    ) -> OutputIt
    {
        out = std::copy(type_name.cbegin(), type_name.cend(), out);

        *out = '(';
        ++out;
        {
            char buf[std::numeric_limits<int>::digits10 + 2];
            auto result = std::to_chars(buf, buf + std::size(buf), underlying);
            ASBIND20_ASSERT(result.ec == std::errc{});
            out = std::copy(buf, result.ptr, out);
        }
        *out = ')';
        ++out;

        return out;
    }

    /**
     * @brief Output iterator that only counts the characters written to it
     *
     * It is used for measuring a field before it can be padded to a width.
     * All copies share the counter, so both @c "*it++ = c" and @c "*it = c"
     * are counted exactly once.
     */
    template <typename CharT>
    class counting_output_iterator
    {
    public:
        using value_type = CharT;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::output_iterator_tag;

        explicit constexpr counting_output_iterator(std::size_t& count) noexcept
            : m_count(&count) {}

        constexpr counting_output_iterator& operator*() noexcept
        {
            return *this;
        }

        constexpr counting_output_iterator& operator++() noexcept
        {
            return *this;
        }

        constexpr counting_output_iterator operator++(int) noexcept
        {
            return *this;
        }

        constexpr counting_output_iterator& operator=(CharT) noexcept
        {
            ++(*m_count);
            return *this;
        }

    private:
        std::size_t* m_count;
    };
} // namespace detail

template <typename CharT = char, typename OutputIt>
constexpr auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asEEngineProp prop, bool skip_prefix, OutputIt out
) -> OutputIt
{
    const char* cstr = asbind20::detail::engine_prop_to_cstr(prop);
    if(!cstr) [[unlikely]]
    {
        using namespace std::string_view_literals;
        return detail::output_fallback<CharT>(
            out, "asEEngineProp"sv, static_cast<int>(prop)
        );
    }

    if(skip_prefix)
        cstr += sizeof("asEP"); // Skip "asEP_"
    return detail::copy_cstr_to(cstr, std::move(out));
}

template <typename CharT = char, typename OutputIt>
constexpr auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asEContextState state, bool skip_prefix, OutputIt out
) -> OutputIt
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
constexpr auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asERetCodes ret, bool skip_prefix, OutputIt out
) -> OutputIt
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
constexpr auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asEMsgType msg_type, bool skip_prefix, OutputIt out
) -> OutputIt
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
constexpr auto copy_debug_representation_to(
    AS_NAMESPACE_QUALIFIER asETokenClass tc, bool skip_prefix, OutputIt out
) -> OutputIt
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

template <typename T>
concept formattable_angelscript_enum =
    std::same_as<T, AS_NAMESPACE_QUALIFIER asEContextState> ||
    std::same_as<T, AS_NAMESPACE_QUALIFIER asERetCodes> ||
    std::same_as<T, AS_NAMESPACE_QUALIFIER asEMsgType> ||
    std::same_as<T, AS_NAMESPACE_QUALIFIER asETokenClass> ||
    std::same_as<T, AS_NAMESPACE_QUALIFIER asEEngineProp>;

/**
 * @brief Length of the debug representation of an enumerator
 */
template <typename CharT = char, formattable_angelscript_enum ASEnum>
constexpr std::size_t debug_representation_length(ASEnum val, bool skip_prefix)
{
    std::size_t count = 0;
    copy_debug_representation_to<CharT>(
        val, skip_prefix, detail::counting_output_iterator<CharT>(count)
    );
    return count;
}

/**
 * @brief Base class of the formatters for AngelScript enumerators
 *
 * Besides the presentation types of the enumerator, the common fill,
 * alignment and width specifiers are supported through
 * @c io::fmtlib::basic_format_data, e.g. @c "{:*>8?}" or @c "{:{}d}".
 */
template <typename CharT = char>
class angelscript_enum_formatter_base
{
public:
    using char_type = CharT;
    using format_data_type = fmtlib::basic_format_data<CharT>;
    using alignment = typename format_data_type::alignment;

    static constexpr alignment align_default = format_data_type::align_default;
    static constexpr alignment align_left = format_data_type::align_left;
    static constexpr alignment align_middle = format_data_type::align_middle;
    static constexpr alignment align_right = format_data_type::align_right;

    constexpr angelscript_enum_formatter_base() noexcept = default;
    constexpr angelscript_enum_formatter_base(const angelscript_enum_formatter_base&) noexcept = default;

    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        using context_char_type = typename ParseContext::char_type;
        static_assert(
            std::same_as<context_char_type, char_type>,
            "Inconsistent character type"
        );

        // Common fill, alignment and width specifiers
        auto it = m_data.parse(ctx);
        if(it == ctx.end() || *it == context_char_type('}'))
            return it;

        // Presentation type of the enumerator
        if(context_char_type ch = *it; ch == context_char_type('?'))
        {
            m_full_representation = true;
            ++it;
        }
        else if(ch == context_char_type('d'))
        {
            m_base = 10;
            ++it;
        }
        else if(ch == context_char_type('x'))
        {
            m_base = 16;
            ++it;
        }

        return it;
    }

    [[nodiscard]]
    constexpr bool full_representation() const noexcept
    {
        return m_full_representation;
    }

    [[nodiscard]]
    constexpr bool print_underlying() const noexcept
    {
        return m_base != 0;
    }

    [[nodiscard]]
    constexpr const format_data_type& format_data() const noexcept
    {
        return m_data;
    }

protected:
    [[nodiscard]]
    int get_base() const noexcept
    {
        return m_base;
    }

private:
    format_data_type m_data;
    // 0 = Don't print as underlying number
    int m_base = 0;
    bool m_full_representation : 1 = false;
};

template <formattable_angelscript_enum ASEnum, typename CharT = char>
class angelscript_enum_formatter : public angelscript_enum_formatter_base<CharT>
{
public:
    using base_type = angelscript_enum_formatter_base<CharT>;
    using format_data_type = typename base_type::format_data_type;
    using alignment = typename base_type::alignment;

    constexpr angelscript_enum_formatter() noexcept = default;
    constexpr angelscript_enum_formatter(const angelscript_enum_formatter&) noexcept = default;

protected:
    template <typename OutputIt, typename FormatContext>
    OutputIt format_to(OutputIt out, ASEnum val, const FormatContext& ctx) const
    {
        const format_data_type& data = this->format_data();
        const std::size_t width = data.width(ctx);

        if(this->print_underlying())
        {
            // The underlying value is formatted like an integer, so the
            // default alignment is right
            char buf[std::numeric_limits<int>::digits10 + 3];
            const auto result = std::to_chars(
                buf, buf + std::size(buf), static_cast<int>(val), this->get_base()
            );
            ASBIND20_ASSERT(result.ec == std::errc{});

            return write_padded(
                out,
                buf,
                static_cast<std::size_t>(result.ptr - buf),
                width,
                base_type::align_right
            );
        }

        // The debug representation is formatted like a string, so the default
        // alignment is left
        if(width == 0)
        {
            return io::copy_debug_representation_to<CharT>(
                val, !this->full_representation(), out
            );
        }

        // The representation has to be measured before it can be aligned, so
        // write it twice: once to a counting output iterator and once to the
        // real output
        const std::size_t len = debug_representation_length<CharT>(
            val, !this->full_representation()
        );

        std::size_t left_padding = 0;
        std::size_t right_padding = 0;
        compute_padding(
            data, len, width, base_type::align_left, left_padding, right_padding
        );

        out = std::fill_n(out, left_padding, data.fill());
        out = io::copy_debug_representation_to<CharT>(
            val, !this->full_representation(), out
        );
        out = std::fill_n(out, right_padding, data.fill());
        return out;
    }

private:
    template <typename OutputIt>
    OutputIt write_padded(
        OutputIt out,
        const char* text,
        std::size_t len,
        std::size_t width,
        alignment default_align
    ) const
    {
        const format_data_type& data = this->format_data();

        std::size_t left_padding = 0;
        std::size_t right_padding = 0;
        compute_padding(data, len, width, default_align, left_padding, right_padding);

        out = std::fill_n(out, left_padding, data.fill());
        out = std::copy(text, text + len, out);
        out = std::fill_n(out, right_padding, data.fill());
        return out;
    }

    static constexpr void compute_padding(
        const format_data_type& data,
        std::size_t len,
        std::size_t width,
        alignment default_align,
        std::size_t& left_padding,
        std::size_t& right_padding
    )
    {
        left_padding = 0;
        right_padding = 0;

        if(width <= len)
            return;

        const std::size_t padding = width - len;
        switch(data.align(default_align))
        {
        case base_type::align_middle:
            left_padding = padding / 2;
            right_padding = padding - left_padding;
            break;

        case base_type::align_right:
            left_padding = padding;
            break;

        default:
        case base_type::align_left:
            right_padding = padding;
            break;
        }
    }
};
} // namespace asbind20::io

#if defined(ASBIND20_HAS_LIB_FORMAT) && !defined(ASBIND20_USE_FMTLIB) && !defined(ASBIND20_CONFIG_NO_STD_FORMATTER)

template <asbind20::io::formattable_angelscript_enum ASEnum, typename CharT>
struct std::formatter<ASEnum, CharT> :
    asbind20::io::angelscript_enum_formatter<ASEnum, CharT>
{
    constexpr formatter() noexcept = default;
    constexpr formatter(const formatter&) noexcept = default;

    template <typename FormatContext>
    constexpr auto format(ASEnum val, FormatContext& ctx) const
        -> typename FormatContext::iterator
    {
        static_assert(
            std::is_same_v<typename FormatContext::char_type, CharT>,
            "Inconsistent character type"
        );

        return this->format_to(ctx.out(), val, ctx);
    }
};

#endif

#if defined(ASBIND20_USE_FMTLIB)

template <asbind20::io::formattable_angelscript_enum ASEnum, typename CharT>
struct fmt::formatter<ASEnum, CharT> :
    asbind20::io::angelscript_enum_formatter<ASEnum, CharT>
{
    constexpr formatter() noexcept = default;
    constexpr formatter(const formatter&) noexcept = default;

    template <typename FormatContext>
    constexpr auto format(ASEnum val, FormatContext& ctx) const
        -> typename FormatContext::iterator
    {
        static_assert(
            std::is_same_v<typename FormatContext::char_type, CharT>,
            "Inconsistent character type"
        );

        return this->format_to(ctx.out(), val, ctx);
    }
};

#endif

#endif
