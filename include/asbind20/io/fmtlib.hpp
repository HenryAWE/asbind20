/**
 * @brief Compatibility layer across different format libraries
 */

#ifndef ASBIND20_IO_FMTLIB_HPP
#define ASBIND20_IO_FMTLIB_HPP

#pragma once

#include <cstddef>
#include <limits>
#include <concepts>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include "../detail/config.hpp"
#include "../detail/err_handler.hpp"

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
    using ::std::format_error;
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
    using ::fmt::format_error;
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

namespace asbind20::io
{
namespace detail
{
    template <typename... Args>
    [[noreturn]]
    void throw_format_error(Args&&... args)
    {
        using error_type =
#if defined(ASBIND20_HAS_FORMAT_LIBRARY)
            fmtlib::format_error
#else
            std::runtime_error
#endif
            ;

        ::asbind20::detail::throw_<error_type>(std::forward<Args>(args)...);
    }

    template <typename Arg, typename Visitor>
    concept has_member_visit =
        requires(Arg&& arg, Visitor&& visitor) {
            std::forward<Arg>(arg).visit(std::forward<Visitor>(visitor));
        };

    /**
     * Both backends provide a way to visit the stored argument, but the
     * availability of the API differs: basic_format_arg::visit is missing
     * in older versions of {fmt} and in the standard library before C++26,
     * while visit_format_arg was removed from {fmt} 12 and deprecated in
     * C++26, so the free function is found by ADL when it is still available.
     */
    template <typename Arg, typename Visitor>
    constexpr decltype(auto) visit_format_argument(Arg&& arg, Visitor&& visitor)
    {
        if constexpr(has_member_visit<Arg, Visitor>)
        {
            return std::forward<Arg>(arg).visit(std::forward<Visitor>(visitor));
        }
        else
        {
            return visit_format_arg(
                std::forward<Visitor>(visitor), std::forward<Arg>(arg)
            );
        }
    }

    // Retrieve an integral argument from a format context
    template <typename FormatContext>
    [[nodiscard]]
    std::size_t get_integer_argument(const FormatContext& ctx, int arg_id)
    {
        return visit_format_argument(
            ctx.arg(arg_id),
            []<typename T>(T&& value) -> std::size_t
            {
                using value_type = std::remove_cvref_t<T>;
                if constexpr(std::integral<value_type>)
                    return static_cast<std::size_t>(value);
                else
                    throw_format_error("format width is not an integer");
            }
        );
    }
} // namespace detail

namespace fmtlib
{
    /**
     * @brief Data of the common fill, alignment and width specifiers
     *
     * The fields are filled by @c parse, which accepts the leading part of the
     * format specification defined by the C++20 standard library @c \<format\>:
     *
     *     [[fill]align][width]
     *
     * - @c fill is any character other than @c '{' and @c '}', and is only
     *   recognized when it is immediately followed by an alignment character,
     *   e.g. @c "{:*>8}".
     * - @c align is @c '<' (left), @c '^' (middle) or @c '>' (right).
     * - @c width is either a positive integer without leading zeros,
     *   e.g. @c "{:8}", or a nested replacement field, e.g. @c "{:{}d}" or
     *   @c "{:{2}d}", whose argument id is recorded and can be queried with
     *   @c width_arg_id().
     */
    template <typename CharT = char>
    class basic_format_data
    {
    public:
        using char_type = CharT;

        enum alignment : std::int8_t
        {
            align_default = 0,
            align_left,
            align_middle,
            align_right
        };

        [[nodiscard]]
        constexpr bool has_fill() const noexcept
        {
            return m_fill != char_type('\0');
        }

        [[nodiscard]]
        constexpr char_type fill() const noexcept
        {
            return has_fill() ? m_fill : char_type(' ');
        }

        [[nodiscard]]
        constexpr std::size_t width() const noexcept
        {
            return m_width;
        }

        /**
         * @brief Get the width, resolving a dynamic width from the arguments
         */
        template <typename FormatContext>
        [[nodiscard]]
        std::size_t width(const FormatContext& ctx) const
        {
            if(!has_dynamic_width())
                return m_width;
            return detail::get_integer_argument(ctx, m_width_arg_id);
        }

        [[nodiscard]]
        constexpr bool has_dynamic_width() const noexcept
        {
            return m_width_arg_id >= 0;
        }

        /**
         * @brief Argument id of the dynamic width, or @c -1 if the width is
         *        not from a nested replacement field
         */
        [[nodiscard]]
        constexpr int width_arg_id() const noexcept
        {
            return m_width_arg_id;
        }

        [[nodiscard]]
        constexpr alignment align(alignment default_val = align_default) const noexcept
        {
            if(m_align == align_default)
                return default_val;
            return m_align;
        }

        /**
         * @brief Parse the common fill, alignment and width specifiers
         *
         * Other options, such as sign, alternative form, zero padding, precision
         * and type, are left to the caller, so that this function can be used as
         * the first stage of a formatter's @c parse function.
         *
         * @return An iterator pointing to the first character that is not part
         *         of these specifiers, which is either the terminating @c '}'
         *         of the replacement field or an option handled by the caller
         * @throw The format error of the selected backend if the specification
         *        is invalid, e.g. an unterminated dynamic width
         */
        template <typename ParseContext>
        constexpr auto parse(ParseContext& parse_ctx)
        {
            using context_char_type = typename ParseContext::char_type;
            static_assert(
                std::same_as<context_char_type, char_type>,
                "Inconsistent character type"
            );

            auto it = parse_ctx.begin();
            const auto end = parse_ctx.end();

            // An empty format specification, e.g. "{}"
            if(it == end || *it == char_type('}'))
                return it;

            // [[fill]align]
            if(char_type ch = *it; is_align(ch))
            {
                m_align = to_alignment(ch);
                ++it;
            }
            else if(*it != char_type('{') && it + 1 != end && is_align(it[1]))
            {
                m_fill = *it;
                m_align = to_alignment(it[1]);
                it += 2;
            }

            // [width]
            if(it != end && *it == char_type('{'))
            {
                // Dynamic width from a nested replacement field, e.g. "{:{}d}"
                ++it; // Skip '{'

                int arg_id = -1;
                if(it != end && is_digit(*it))
                {
                    arg_id = 0;
                    for(; it != end && is_digit(*it); ++it)
                    {
                        const int digit = static_cast<int>(*it - char_type('0'));
                        if(arg_id > (std::numeric_limits<int>::max() - digit) / 10)
                            detail::throw_format_error("format width argument ID is too large");
                        arg_id = arg_id * 10 + digit;
                    }
                }

                if(it == end || *it != char_type('}'))
                    detail::throw_format_error("missing '}' in dynamic width");

                ++it; // Skip '}'

                if(arg_id < 0)
                    m_width_arg_id = static_cast<int>(parse_ctx.next_arg_id());
                else
                {
                    parse_ctx.check_arg_id(arg_id);
                    m_width_arg_id = arg_id;
                }
            }
            else if(it != end && is_nonzero_digit(*it))
            {
                // A positive integer without leading zeros
                std::size_t width = 0;
                for(; it != end && is_digit(*it); ++it)
                {
                    const auto digit = static_cast<std::size_t>(*it - char_type('0'));
                    if(width > (std::numeric_limits<std::size_t>::max() - digit) / 10)
                        detail::throw_format_error("format width is too large");
                    width = width * 10 + digit;
                }
                m_width = width;
            }

            return it;
        }

    private:
        static constexpr bool is_digit(char_type ch) noexcept
        {
            return ch >= char_type('0') && ch <= char_type('9');
        }

        static constexpr bool is_nonzero_digit(char_type ch) noexcept
        {
            return ch > char_type('0') && ch <= char_type('9');
        }

        static constexpr bool is_align(char_type ch) noexcept
        {
            return ch == char_type('<') ||
                   ch == char_type('^') ||
                   ch == char_type('>');
        }

        static constexpr alignment to_alignment(char_type ch) noexcept
        {
            switch(ch)
            {
            case char_type('<'):
                return align_left;
            case char_type('^'):
                return align_middle;
            case char_type('>'):
                return align_right;
            default:
                return align_default;
            }
        }

        std::size_t m_width = 0;
        // -1: width is not specified by a nested replacement field
        int m_width_arg_id = -1;
        char_type m_fill = char_type('\0');
        alignment m_align = align_default;
    };
} // namespace fmtlib
} // namespace asbind20::io

#endif
