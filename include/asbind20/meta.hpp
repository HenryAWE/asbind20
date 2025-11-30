/**
 * @file meta.hpp
 * @author HenryAWE
 * @brief Tools for meta-programming
 */

#ifndef ASBIND20_META_HPP
#define ASBIND20_META_HPP

#pragma once

#include <string_view>
#include <tuple>
#include <concepts>
#include <type_traits>
#include "utility.hpp"

namespace asbind20
{
/**
 * @brief Tools for meta-programming
 */
namespace meta
{
    namespace detail
    {
        template <typename T, typename... Args>
        concept check_placement_new = requires(void* mem) {
            new(mem) T(std::declval<Args>()...);
        };
    } // namespace detail

    /**
     * @brief Check if a type is constructible without requiring it to be destructible,
     *        i.e., constructible at a specific memory address by placement new.
     *
     * `std::is_constructible` implicitly requires the type to be destructible,
     * which is not necessary for generating a constructor/factory for types with non-public destructor.
     *
     * See: https://stackoverflow.com/questions/28085847/stdis-constructible-on-type-with-non-public-destructor
     */
    template <typename T, typename... Args>
    struct is_constructible_at :
        public std::bool_constant<detail::check_placement_new<T, Args...>>
    {};

    template <typename T, typename... Args>
    constexpr inline bool is_constructible_at_v = is_constructible_at<T, Args...>::value;

    template <typename Target, typename Tuple>
    struct contains;

    template <typename Target, typename... Ts>
    struct contains<Target, std::tuple<Ts...>> :
        std::bool_constant<false || (std::same_as<Target, Ts> || ...)>
    {};

    template <typename Target, typename Tuple>
    constexpr inline bool contains_v = contains<Target, Tuple>::value;

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

        constexpr fixed_string(const char (&str)[Size + 1])
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

        static constexpr size_type size() noexcept
        {
            return Size;
        };

        static constexpr bool empty() noexcept
        {
            return Size == 0;
        }

        constexpr const value_type* data() const noexcept
        {
            return internal_data;
        }

        constexpr const value_type* c_str() const noexcept
        {
            return data();
        }

        constexpr operator const char*() const noexcept
        {
            return c_str();
        }

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

    /**
     * @brief Get the script type flags.
     *
     * This function produces the same result as `asGetTypeTraits`,
     * but this function is available at compile-time.
     */
    template <typename T>
    consteval auto get_script_type_flags() noexcept
        -> AS_NAMESPACE_QUALIFIER asQWORD
    {
        using namespace std;

        constexpr bool flag_c = is_default_constructible_v<T> && !is_trivially_default_constructible_v<T>;
        constexpr bool flag_d = is_destructible_v<T> && !is_trivially_destructible_v<T>;
        constexpr bool flag_a = is_copy_assignable_v<T> && !is_trivially_copy_assignable_v<T>;
        constexpr bool flag_k = is_copy_constructible_v<T> && !is_trivially_copy_constructible_v<T>;

        constexpr bool is_float = is_floating_point_v<T>;
        constexpr bool is_primitive = is_integral_v<T> || is_pointer_v<T> || is_enum_v<T>;
        constexpr bool is_class = is_class_v<T>;
        constexpr bool is_array = is_array_v<T>;

        if(is_float)
            return AS_NAMESPACE_QUALIFIER asOBJ_APP_FLOAT;
        if(is_primitive)
            return AS_NAMESPACE_QUALIFIER asOBJ_APP_PRIMITIVE;

        if(is_class)
        {
            AS_NAMESPACE_QUALIFIER asQWORD flags =
                AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS;
            if(flag_c)
                flags |=AS_NAMESPACE_QUALIFIER  asOBJ_APP_CLASS_CONSTRUCTOR;
            if(flag_d)
                flags |=AS_NAMESPACE_QUALIFIER  asOBJ_APP_CLASS_DESTRUCTOR;
            if(flag_a)
                flags |=AS_NAMESPACE_QUALIFIER  asOBJ_APP_CLASS_ASSIGNMENT;
            if(flag_k)
                flags |=AS_NAMESPACE_QUALIFIER  asOBJ_APP_CLASS_COPY_CONSTRUCTOR;
            return flags;
        }

        if(is_array)
            return AS_NAMESPACE_QUALIFIER asOBJ_APP_ARRAY;

        // Unknown type traits
        return 0;
    }
} // namespace meta

namespace detail
{
    template <typename T>
    class func_traits_impl;

#define ASBIND20_FUNC_TRAITS_IMPL_GLOBAL(call_conv, noexcept_, ...)  \
    template <typename R, typename... Args>                          \
    class func_traits_impl<R(call_conv*)(Args...) noexcept_>         \
    {                                                                \
    public:                                                          \
        using return_type = R;                                       \
        using args_tuple = std::tuple<Args...>;                      \
        using class_type = void;                                     \
        static constexpr bool is_const_v = false;                    \
        static constexpr bool is_noexcept_v = #noexcept_[0] != '\0'; \
    }

#ifdef ASBIND20_HAS_STANDALONE_STDCALL
    ASBIND20_FUNC_TRAITS_IMPL_GLOBAL(ASBIND20_CDECL, noexcept);
    ASBIND20_FUNC_TRAITS_IMPL_GLOBAL(ASBIND20_CDECL, );
    ASBIND20_FUNC_TRAITS_IMPL_GLOBAL(ASBIND20_STDCALL, noexcept);
    ASBIND20_FUNC_TRAITS_IMPL_GLOBAL(ASBIND20_STDCALL, );
#else
    ASBIND20_FUNC_TRAITS_IMPL_GLOBAL(, noexcept);
    ASBIND20_FUNC_TRAITS_IMPL_GLOBAL(, , );
#endif


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

/**
 * @brief Function traits
 *
 * @tparam T Function type
 */
template <typename T>
class function_traits : public detail::func_traits_impl<std::decay_t<std::remove_cvref_t<T>>>
{
    using my_base = detail::func_traits_impl<std::decay_t<std::remove_cvref_t<T>>>;

public:
    using type = T;
    using return_type = typename my_base::return_type;
    using args_tuple = typename my_base::args_tuple;
    using class_type = typename my_base::class_type;

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

    /**
     * @brief `void` if `Idx` is invalid
     *
     * @tparam Idx Argument index
     */
    template <std::size_t Idx>
    using arg_type_optional = detail::safe_tuple_elem_t<Idx, args_tuple>;

    using first_arg_type = arg_type_optional<0>;

    using last_arg_type = arg_type_optional<arg_count_v - 1>;
};

namespace meta
{
    namespace detail
    {
        template <typename F>
        struct is_stdcall_helper
        {
            using type = std::false_type;
        };

#ifdef ASBIND20_HAS_STANDALONE_STDCALL

        template <typename Return, typename... Args>
        struct is_stdcall_helper<Return(ASBIND20_STDCALL*)(Args...)>
        {
            using type = std::true_type;
        };

#endif
    } // namespace detail

    template <typename F>
    struct is_stdcall :
        public detail::is_stdcall_helper<std::decay_t<F>>::type
    {};

    template <typename F>
    inline constexpr bool is_stdcall_v = is_stdcall<F>::value;
} // namespace meta

template <typename T>
requires(std::is_arithmetic_v<T> && !std::same_as<std::remove_cv_t<T>, char>)
consteval auto name_of() noexcept
{
    if constexpr(std::same_as<T, bool>)
        return meta::fixed_string("bool");
    else if constexpr(std::integral<T>)
    {
        if constexpr(std::is_unsigned_v<T>)
        {
            if constexpr(sizeof(T) == 1)
                return meta::fixed_string("uint8");
            else if constexpr(sizeof(T) == 2)
                return meta::fixed_string("uint16");
            else if constexpr(sizeof(T) == 4)
                return meta::fixed_string("uint");
            else if constexpr(sizeof(T) == 8)
                return meta::fixed_string("uint64");
            else
                static_assert(!sizeof(T), "Invalid integral");
        }
        else if constexpr(std::is_signed_v<T>)
        {
            if constexpr(sizeof(T) == 1)
                return meta::fixed_string("int8");
            else if constexpr(sizeof(T) == 2)
                return meta::fixed_string("int16");
            else if constexpr(sizeof(T) == 4)
                return meta::fixed_string("int");
            else if constexpr(sizeof(T) == 8)
                return meta::fixed_string("int64");
            else
                static_assert(!sizeof(T), "Invalid integral");
        }
        else // Neither signed nor unsigned
            static_assert(!sizeof(T), "Invalid integral");
    }
    else if constexpr(std::floating_point<T>)
    {
        if constexpr(std::same_as<T, float>)
            return meta::fixed_string("float");
        else if constexpr(std::same_as<T, double>)
            return meta::fixed_string("double");
        else
            static_assert(!sizeof(T), "Invalid floating point");
    }
    else
        static_assert(!sizeof(T), "Invalid arithmetic");
}

template <typename T>
concept has_static_name =
    std::is_arithmetic_v<T> &&
    !std::same_as<std::remove_cv_t<T>, char>;

namespace meta
{
    template <typename T>
    requires(has_static_name<std::remove_cvref_t<T>>)
    consteval auto full_fixed_name_of()
    {
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;

        constexpr auto type_name = []()
        {
            constexpr auto name = name_of<std::remove_cvref_t<T>>();
            if constexpr(is_const)
                return fixed_string("const ") + name;
            else
                return name;
        }();

        if constexpr(std::is_reference_v<T>)
        {
            if constexpr(is_const)
                return type_name + fixed_string("&in");
            else
                return type_name + fixed_string("&");
        }
        else
            return type_name;
    }

    /**
     * @brief Get string representation of an enum value in fixed string
     *
     * @note This function has limitations. @sa static_enum_name
     *
     * @tparam Value Enum value
     */
    template <auto Value>
    auto fixed_enum_name() noexcept
    {
        constexpr std::string_view name_view = static_enum_name<Value>();
        constexpr std::size_t size = name_view.size();

        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            return fixed_string<size>(name_view[Is]...);
        }(std::make_index_sequence<size>());
    }
} // namespace meta
} // namespace asbind20

#endif
