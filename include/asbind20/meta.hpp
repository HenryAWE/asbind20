#pragma once

#include <array>
#include <concepts>
#include <type_traits>

namespace asbind20
{
namespace detail
{
    class static_string_tag
    {};
} // namespace detail

template <std::size_t Size>
class static_string : public detail::static_string_tag
{
public:
    constexpr static_string() = default;

    constexpr static_string(const static_string&) = default;

    constexpr static_string(char ch, std::size_t sz)
    {
        for(std::size_t i = 0; i < Size; ++i)
        {
            raw_data[i] = i < sz ? ch : '\0';
        }
    }

    explicit constexpr static_string(char ch)
        : static_string(ch, length()) {}

    template <std::size_t N1, std::size_t N2>
    constexpr static_string(const static_string<N1>& str1, const static_string<N2>& str2)
    {
        // Copy data from first string
        for(std::size_t i = 0; i < str1.length(); ++i)
        {
            raw_data[i] = str1.c_str()[i];
        }

        std::size_t off = str1.length();
        // Copy data from second string
        for(std::size_t i = 0; i < str2.length(); ++i)
        {
            raw_data[i + off] = str2.c_str()[i];
        }

        raw_data[Size - 1] = '\0';
    }

    consteval static_string(const char (&arr)[Size])
    {
        for(std::size_t i = 0; i < Size; ++i)
        {
            raw_data[i] = arr[i];
        }
    }

    constexpr static_string(const char* cstr, std::size_t sz)
    {
        for(std::size_t i = 0; i < Size; ++i)
        {
            raw_data[i] = i < sz ? cstr[i] : '\0';
        }
        raw_data[Size - 1] = '\0';
    }

    // WARNING: This includes the trailing zero!
    constexpr static std::size_t size() noexcept
    {
        return Size;
    }

    consteval static std::size_t length() noexcept
    {
        return Size - 1; // Remove trailing zero
    }

    consteval static bool empty() noexcept
    {
        return Size <= 1;
    }

    constexpr const char* c_str() const noexcept
    {
        return raw_data.data();
    }

    constexpr std::string_view to_string_view() const noexcept
    {
        return std::string_view(raw_data.data(), length());
    }

    constexpr operator std::string_view() const noexcept
    {
        return to_string_view();
    }

    template <std::size_t Idx>
    constexpr char get() const noexcept
    {
        static_assert(Idx < Size, "out of range");
        return raw_data[Idx];
    }

    constexpr std::array<char, Size> to_array() const noexcept
    {
        return raw_data;
    }

    template <std::size_t Pos, std::size_t N = std::size_t(-1)>
    consteval auto substr() const noexcept
    {
        constexpr std::size_t len = length();
        static_assert(Pos < len, "out of range");

        constexpr std::size_t avail = len - Pos;
        constexpr std::size_t new_size = std::min(avail, N) + 1;
        return static_string<new_size>(raw_data.data() + Pos, new_size);
    }

    template <std::size_t N>
    consteval auto append(const static_string<N>& other) const noexcept
    {
        constexpr std::size_t new_size = length() + other.length() + 1;
        return static_string<new_size>(*this, other);
    }

    // Internal, DO NOT USE!
    // Because we need static_string as a non-type template argument (NTTP),
    // this data member cannot be private.
    std::array<char, Size> raw_data;
};

template <std::size_t N1, std::size_t N2>
consteval auto operator+(const static_string<N1>& str1, const static_string<N2>& str2) noexcept
{
    return str1.append(str2);
}

// CTAD
static_string() -> static_string<0>;
static_string(char) -> static_string<2>;

namespace detail
{
    template <typename T>
    concept static_str = std::is_base_of_v<static_string_tag, T>;
} // namespace detail

template <detail::static_str auto... Strings>
constexpr auto static_concat() noexcept
{
    if constexpr(sizeof...(Strings) == 0)
        return static_string<0>();
    else
    {
        return (Strings + ...);
    }
}

namespace detail
{
    template <static_str auto Sep, static_str auto String, static_str auto... Strings>
    constexpr auto static_join_impl()
    {
        if constexpr(sizeof...(Strings) == 0)
            return String;
        else
        {
            return static_concat<String, Sep, static_join_impl<Sep, Strings...>()>();
        }
    }
} // namespace detail

template <detail::static_str auto Sep, detail::static_str auto... Strings>
constexpr auto static_join() noexcept
{
    if constexpr(sizeof...(Strings) == 0)
        return static_string<0>();
    else
    {
        return detail::static_join_impl<Sep, Strings...>();
    }
}

namespace detail
{
    template <typename T>
    class func_traits_impl;

#define ASBIND20_FUNC_TRAITS_IMPL_GLOBAL(noexcept_)                  \
    template <typename R, typename... Args>                          \
    class func_traits_impl<R(Args...) noexcept_>                     \
    {                                                                \
    public:                                                          \
        using return_type = R;                                       \
        using args_tuple = std::tuple<Args...>;                      \
        using class_type = void;                                     \
        static constexpr bool is_const_v = false;                    \
        static constexpr bool is_noexcept_v = #noexcept_[0] != '\0'; \
    };                                                               \
    template <typename R, typename... Args>                          \
    class func_traits_impl<R (*)(Args...) noexcept_>                 \
    {                                                                \
    public:                                                          \
        using return_type = R;                                       \
        using args_tuple = std::tuple<Args...>;                      \
        using class_type = void;                                     \
        static constexpr bool is_const_v = false;                    \
        static constexpr bool is_noexcept_v = #noexcept_[0] != '\0'; \
    }

    ASBIND20_FUNC_TRAITS_IMPL_GLOBAL(noexcept);
    ASBIND20_FUNC_TRAITS_IMPL_GLOBAL();

#undef ASBIND20_FUNC_TRAITS_IMPL_GLOBAL

#define ASBIND20_FUNC_TRAITS_IMPL_MEMBER(const_, noexcept_)          \
    template <typename R, typename Class, typename... Args>          \
    class func_traits_impl<R (Class::*)(Args...) const_ noexcept_>   \
    {                                                                \
    public:                                                          \
        using return_type = R;                                       \
        using args_tuple = std::tuple<Args...>;                      \
        using class_type = Class;                                    \
        static constexpr bool is_const_v = #const_[0] != '\0';       \
        static constexpr bool is_noexcept_v = #noexcept_[0] != '\0'; \
    }

    ASBIND20_FUNC_TRAITS_IMPL_MEMBER(const, noexcept);
    ASBIND20_FUNC_TRAITS_IMPL_MEMBER(const, );
    ASBIND20_FUNC_TRAITS_IMPL_MEMBER(, noexcept);
    ASBIND20_FUNC_TRAITS_IMPL_MEMBER(, );

#undef ASBIND20_FUNC_TRAITS_IMPL_MEMBER

    template <typename T>
    concept callable_class = std::is_class_v<T>&& requires()
    {
        &T::operator();
    };

    template <typename T>
    requires callable_class<T>
    class func_traits_impl<T> : public func_traits_impl<decltype(&T::operator())>
    {
    private:
        using my_base = func_traits_impl<decltype(&T::operator())>;

    public:
        using return_type = typename my_base::return_type;
        using args_tuple = typename my_base::args_tuple;
        using class_type = typename my_base::class_type;
    };

    template <
        std::size_t Idx,
        typename Tuple,
        bool Valid = (Idx < std::tuple_size_v<Tuple>)>
    struct safe_tuple_elem;

    template <
        std::size_t Idx,
        typename Tuple>
    struct safe_tuple_elem<Idx, Tuple, true>
    {
        using type = std::tuple_element_t<Idx, Tuple>;
    };

    template <
        std::size_t Idx,
        typename Tuple>
    struct safe_tuple_elem<Idx, Tuple, false>
    {
        // Invalid index
        using type = void;
    };

    template <typename std::size_t Idx, typename Tuple>
    using safe_tuple_elem_t = typename safe_tuple_elem<Idx, Tuple>::type;
} // namespace detail

template <typename T>
class function_traits : public detail::func_traits_impl<std::remove_cvref_t<T>>
{
    using my_base = detail::func_traits_impl<std::remove_cvref_t<T>>;

public:
    using type = T;
    using return_type = my_base::return_type;
    using args_tuple = my_base::args_tuple;
    using class_type = my_base::class_type;

    static constexpr bool is_method_v = !std::is_void_v<class_type>;
    using is_method = std::bool_constant<is_method_v>;

    using is_const = std::bool_constant<my_base::is_const_v>;
    using is_noexcept = std::bool_constant<my_base::is_noexcept_v>;

    static constexpr std::size_t arg_count_v = std::tuple_size_v<args_tuple>;
    using arg_count = std::integral_constant<
        std::size_t,
        arg_count_v>;

    template <std::size_t Idx>
    using arg_type = std::tuple_element_t<Idx, args_tuple>;

    // `void` if `Idx` is invalid
    template <std::size_t Idx>
    using arg_type_optional = detail::safe_tuple_elem_t<Idx, args_tuple>;

    using first_arg_type = arg_type_optional<0>;

    using last_arg_type = arg_type_optional<arg_count_v - 1>;
};
} // namespace asbind20
