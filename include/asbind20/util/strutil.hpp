/**
 * @file strutil.hpp
 * @author HenryAWE
 * @brief String utilities
 */

#ifndef ASBIND20_UTIL_STRUTIL_HPP
#define ASBIND20_UTIL_STRUTIL_HPP

#pragma once

#include <iosfwd>
#include <cassert>
#include <string>
#include <string_view>
#include "../detail/config.hpp"
#ifdef ASBIND20_HAS_LIB_FORMAT
#    include <format>
#endif

namespace asbind20
{
/**
 * @brief String reference with a guaranteed zero at the end
 */
class cstring_ref
{
public:
    using size_type = std::size_t;
    using traits_type = std::char_traits<char>;

    constexpr cstring_ref() noexcept
        : m_cstr(nullptr) {}

    constexpr cstring_ref(std::nullptr_t) noexcept = delete;

    constexpr cstring_ref(const cstring_ref&) noexcept = default;

    constexpr cstring_ref(const std::string& str) noexcept
        : m_cstr(str.c_str()) {}

    constexpr cstring_ref(const char* str) noexcept
        : m_cstr(str) {}

    template <std::size_t N>
    constexpr explicit cstring_ref(const char (&arr)[N]) noexcept
        : cstring_ref(arr)
    {}

    constexpr ~cstring_ref() = default;

    cstring_ref& operator=(const cstring_ref&) noexcept = default;

    constexpr bool operator==(const cstring_ref& rhs) const noexcept
    {
        return std::string_view(*this) == rhs;
    }

    constexpr friend bool operator==(cstring_ref lhs, std::string_view rhs) noexcept
    {
        return std::string_view(lhs) == rhs;
    }

    constexpr friend bool operator==(std::string_view lhs, cstring_ref rhs) noexcept
    {
        return lhs == std::string_view(rhs);
    }

    constexpr friend bool operator==(cstring_ref lhs, const std::string& rhs) noexcept
    {
        return std::string_view(lhs) == std::string_view(rhs);
    }

    constexpr friend bool operator==(const std::string& lhs, cstring_ref rhs) noexcept
    {
        return std::string_view(lhs) == std::string_view(rhs);
    }

    constexpr friend bool operator==(cstring_ref lhs, const char* rhs) noexcept
    {
        return std::string_view(lhs) == rhs;
    }

    constexpr friend bool operator==(const char* lhs, cstring_ref rhs) noexcept
    {
        return lhs == std::string_view(rhs);
    }

    explicit operator bool() const noexcept
    {
        return m_cstr != nullptr;
    }

    /**
     * @warning The cost of this function is O(n) !
     */
    [[nodiscard]]
    constexpr size_type size() const noexcept
    {
        if(m_cstr == nullptr) [[unlikely]]
            return 0;
        return traits_type::length(m_cstr);
    }

    [[nodiscard]]
    constexpr bool empty() const noexcept
    {
        if(m_cstr == nullptr)
            return true;
        return m_cstr[0] == '\0';
    }

    [[nodiscard]]
    constexpr const char* c_str() const noexcept
    {
        return m_cstr;
    }

    [[nodiscard]]
    constexpr const char* safe_c_str() const noexcept
    {
        if(m_cstr == nullptr) [[unlikely]]
            return "";
        return m_cstr;
    }

    [[nodiscard]]
    constexpr const char* data() const noexcept
    {
        return m_cstr;
    }

    explicit constexpr operator const char*() const noexcept
    {
        return c_str();
    }

    constexpr operator std::string_view() const noexcept
    {
        if(m_cstr == nullptr) [[unlikely]]
            return std::string_view();
        return std::string_view(m_cstr);
    }

    constexpr void remove_prefix(size_type n) noexcept
    {
        ASBIND20_ASSERT(n <= size());
        m_cstr += n;
    }

    [[nodiscard]]
    constexpr cstring_ref trim_prefix(size_type n) const noexcept
    {
        auto tmp(*this);
        tmp.remove_prefix(n);
        return tmp;
    }

    friend std::ostream& operator<<(std::ostream& os, cstring_ref cstr)
    {
        return os << std::string_view(cstr);
    }

    class op_addr_proxy
    {
        friend cstring_ref;

        explicit op_addr_proxy(cstring_ref& dst)
            : m_dst(dst) {}

    public:
        op_addr_proxy(const op_addr_proxy&) noexcept = delete;

        operator const char**() const noexcept
        {
            return &m_dst.m_cstr;
        }

    private:
        cstring_ref& m_dst;
    };

    op_addr_proxy operator&() noexcept
    {
        return op_addr_proxy{*this};
    }

private:
    const char* m_cstr;
};

namespace util
{
    template <std::size_t Size>
    class fixed_string
    {
    public:
        using value_type = char;
        using size_type = std::size_t;

        /**
         * @brief **INTERNAL DATA. DO NOT USE!**
         *
         * This member is exposed for satisfying the NTTP requirements of C++.
         *
         * @note It includes `\0` at the end.
         */
        char internal_data[Size + 1] = {};

        constexpr fixed_string(const fixed_string&) noexcept = default;

        template <std::convertible_to<char>... Chars>
        requires(sizeof...(Chars) == Size)
        explicit constexpr fixed_string(Chars... chs)
            : internal_data{static_cast<char>(chs)..., '\0'}
        {}

        // explicit(false) on purpose
        constexpr explicit(false) fixed_string(const char (&str)[Size + 1])
        {
            for(size_type i = 0; i < Size; ++i)
            {
                internal_data[i] = str[i];
            }
            internal_data[Size] = '\0';
        }

        constexpr bool operator==(const fixed_string&) const noexcept = default;

        template <std::size_t N>
        requires(N != Size)
        constexpr bool operator==(const fixed_string<N>&) const noexcept
        {
            return false;
        }

        [[nodiscard]]
        static constexpr size_type size() noexcept
        {
            return Size;
        };

        [[nodiscard]]
        static constexpr bool empty() noexcept
        {
            return Size == 0;
        }

        [[nodiscard]]
        constexpr const value_type* data() const noexcept
        {
            return internal_data;
        }

        [[nodiscard]]
        constexpr const value_type* c_str() const noexcept
        {
            return data();
        }

        constexpr operator cstring_ref() const noexcept
        {
            return cstring_ref(c_str());
        }

        [[nodiscard]]
        constexpr std::string_view view() const noexcept
        {
            return std::string_view(data(), size());
        }

        constexpr operator std::string_view() const noexcept
        {
            return view();
        }
    };

    template <std::convertible_to<char>... Chars>
    fixed_string(Chars...) -> fixed_string<sizeof...(Chars)>;

    template <std::size_t N>
    fixed_string(const char (&str)[N]) -> fixed_string<N - 1>;

    template <std::size_t SizeL, std::size_t SizeR>
    constexpr auto operator+(const fixed_string<SizeL>& lhs, const fixed_string<SizeR>& rhs) -> fixed_string<SizeL + SizeR>
    {
        if constexpr(SizeL == 0)
            return rhs;
        else if constexpr(SizeR == 0)
            return lhs;
        else
        {
            return [&]<std::size_t... I1, std::size_t... I2>(std::index_sequence<I1...>, std::index_sequence<I2...>)
            {
                return fixed_string<SizeL + SizeR>(lhs.internal_data[I1]..., rhs.internal_data[I2]...);
            }(std::make_index_sequence<SizeL>(), std::make_index_sequence<SizeR>());
        }
    }

    template <typename T>
    struct is_fixed_string : public std::false_type
    {};

    template <std::size_t Size>
    struct is_fixed_string<fixed_string<Size>> : public std::true_type
    {};

    template <std::size_t Size>
    std::ostream& operator<<(std::ostream& os, const fixed_string<Size>& str)
    {
        os << std::string_view(str);
        return os;
    }

    namespace detail
    {
        template <typename Fn, typename Tuple>
        decltype(auto) with_cstr_impl(Fn&& fn, Tuple&& tp)
        {
            return std::apply(std::forward<Fn>(fn), std::forward<Tuple>(tp));
        }

        template <typename Fn, typename Tuple, typename Arg, typename... Args>
        decltype(auto) with_cstr_impl(Fn&& fn, Tuple&& tp, Arg&& arg, Args&&... args)
        {
            using arg_type = std::remove_cvref_t<Arg>;

            if constexpr(std::same_as<arg_type, std::string_view>)
            {
                if(arg.data()[arg.size()] == '\0')
                {
                    return with_cstr_impl(
                        std::forward<Fn>(fn),
                        std::tuple_cat(
                            tp,
                            std::tuple<const char*>(arg.data())
                        ),
                        std::forward<Args>(args)...
                    );
                }
                else
                {
                    return with_cstr_impl(
                        std::forward<Fn>(fn),
                        std::tuple_cat(
                            tp,
                            std::tuple<const char*>(std::string(arg).c_str())
                        ),
                        std::forward<Args>(args)...
                    );
                }
            }
            else if constexpr(
                std::same_as<arg_type, std::string> ||
                std::same_as<arg_type, cstring_ref> ||
                is_fixed_string<arg_type>::value
            )
            {
                return with_cstr_impl(
                    std::forward<Fn>(fn),
                    std::tuple_cat(
                        tp,
                        std::tuple<const char*>(arg.c_str())
                    ),
                    std::forward<Args>(args)...
                );
            }
            else
            {
                return with_cstr_impl(
                    std::forward<Fn>(fn),
                    std::tuple_cat(
                        tp,
                        std::make_tuple<Arg&&>(std::forward<Arg>(arg))
                    ),
                    std::forward<Args>(args)...
                );
            }
        }
    } // namespace detail

    /**
     * @brief This function will convert `string` and `string_view` in parameters to null-terminated `const char*`
     *        for APIs receiving C-style string.
     *
     * @details This function will make a copy of string view if it is not null-terminated.
     */
    template <typename Fn, typename... Args>
    [[nodiscard]]
    decltype(auto) with_cstr(Fn&& fn, Args&&... args)
    {
        return detail::with_cstr_impl(
            std::forward<Fn>(fn),
            std::tuple<>(),
            std::forward<Args>(args)...
        );
    }
} // namespace util

template <typename T>
concept string_like =
    std::convertible_to<T, std::string_view> ||
    std::convertible_to<T, std::string> ||
    std::convertible_to<std::decay_t<T>, const char*>;

namespace util
{
    template <string_like StringLike>
    constexpr std::string string_like_to_string(StringLike&& str)
    {
        using type = std::remove_cvref_t<StringLike>;
        using std::convertible_to;
        if constexpr(convertible_to<type, std::string>)
        {
            return std::string(std::forward<StringLike>(str));
        }
        else if constexpr(convertible_to<std::decay_t<type>, const char*>)
        {
            const char* cstr = str;
            return cstr;
        }
        else
        {
            std::string_view sv(str);
            return std::string(sv);
        }
    }
} // namespace util
} // namespace asbind20

#ifdef ASBIND20_HAS_LIB_FORMAT

template <>
struct std::formatter<asbind20::cstring_ref> : std::formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(asbind20::cstring_ref str, FormatContext& ctx) const
    {
        using my_base = std::formatter<std::string_view>;
        return my_base::format(static_cast<std::string_view>(str), ctx);
    }
};

template <std::size_t Size>
struct std::formatter<asbind20::util::fixed_string<Size>> : std::formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const asbind20::util::fixed_string<Size>& str, FormatContext& ctx) const
    {
        using my_base = std::formatter<std::string_view>;
        return my_base::format(str.view(), ctx);
    }
};

#endif

#endif
