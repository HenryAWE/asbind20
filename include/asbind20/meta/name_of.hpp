#ifndef ASBIND20_META_NAME_OF_HPP
#define ASBIND20_META_NAME_OF_HPP

#pragma once

#include "../detail/config.hpp"
#include <algorithm>
#include <concepts>
#include <string_view>
#include <type_traits>
#include "refl_common.hpp"

namespace asbind20::meta
{
/**
 * @brief Get string representation of an arithmetic type
 *
 * @tparam T Arithmetic type
 */
template <typename T>
requires(
    std::same_as<std::remove_cvref_t<T>, T> &&
    !std::same_as<T, char>
)
consteval auto fixed_string_script_typename_of() noexcept
{
    if constexpr(std::same_as<T, bool>)
        return util::fixed_string("bool");
    else if constexpr(std::integral<T>)
    {
        if constexpr(std::is_unsigned_v<T>)
        {
            if constexpr(sizeof(T) == 1)
                return util::fixed_string("uint8");
            else if constexpr(sizeof(T) == 2)
                return util::fixed_string("uint16");
            else if constexpr(sizeof(T) == 4)
                return util::fixed_string("uint");
            else if constexpr(sizeof(T) == 8)
                return util::fixed_string("uint64");
            else
                static_assert(!sizeof(T), "Invalid integral");
        }
        else if constexpr(std::is_signed_v<T>)
        {
            if constexpr(sizeof(T) == 1)
                return util::fixed_string("int8");
            else if constexpr(sizeof(T) == 2)
                return util::fixed_string("int16");
            else if constexpr(sizeof(T) == 4)
                return util::fixed_string("int");
            else if constexpr(sizeof(T) == 8)
                return util::fixed_string("int64");
            else
                static_assert(!sizeof(T), "Invalid integral");
        }
    }
    else if constexpr(std::floating_point<T>)
    {
        if constexpr(std::same_as<T, float>)
            return util::fixed_string("float");
        else if constexpr(std::same_as<T, double>)
            return util::fixed_string("double");
        else
            static_assert(!sizeof(T), "Invalid floating point");
    }
    else
        static_assert(!sizeof(T), "Invalid arithmetic");
}

/**
 * @brief Check if the name of a type is available at compile-time
 *
 * @tparam T Type to check
 */
template <typename T>
concept has_script_typename =
    std::is_arithmetic_v<T> &&
    !std::same_as<std::remove_cv_t<T>, char>;

/**
 * @brief Get string from an enum value
 *
 * @note This function uses compiler extension to get name of enum.
 *       It cannot handle enum value that has same underlying value with another enum.
 *
 * @tparam Value Enum value
 */
template <auto Value>
requires(std::is_enum_v<decltype(Value)>)
consteval std::string_view enum_name_of()
{
    std::string_view name;

#ifdef ASBIND20_HAS_LIB_REFLECTION

#    define ASBIND20_HAS_ENUM_NAME_OF "__cpp_lib_reflection"

    using enum_type = decltype(Value);

    static constexpr auto enumerators =
        std::define_static_array(std::meta::enumerators_of(^^enum_type));

    template for(constexpr auto enumerator : enumerators)
    {
        if([:enumerator:] == Value)
        {
            name = std::meta::identifier_of(enumerator);
        }
    }

    if(name.empty())
        throw std::meta::exception("bad enum value", ^^enum_type);

#elif defined(__clang__) || defined(__GNUC__)
    name = __PRETTY_FUNCTION__;

    std::size_t start = name.find("Value = ") + 8;

#    ifdef __clang__
#        define ASBIND20_HAS_ENUM_NAME_OF "__PRETTY_FUNCTION__ (Clang)"

    std::size_t end = name.find_last_of(']');
#    else // GCC
#        define ASBIND20_HAS_ENUM_NAME_OF "__PRETTY_FUNCTION__ (GCC)"

    std::size_t end = std::min(name.find(';', start), name.find_last_of(']'));
#    endif

    name = std::string_view(name.data() + start, end - start);

#elif defined(_MSC_VER)
#    define ASBIND20_HAS_ENUM_NAME_OF "__FUNCSIG__"

    name = __FUNCSIG__;
    std::size_t start = name.find("enum_name_of<") + 13;
    std::size_t end = name.find_last_of('>');
    name = std::string_view(name.data() + start, end - start);

#else
    static_assert(false, "Not supported");

#endif

    // Remove qualifier
    std::size_t qual_end = name.rfind("::");
    if(qual_end != std::string_view::npos)
    {
        qual_end += 2; // skip "::"
        return name.substr(qual_end);
    }

    return name;
}

namespace detail
{
    template <typename T>
    consteval std::string_view type_name_of_impl()
    {
        // Original implementation:
        // https://gist.github.com/HenryAWE/e7f6a2274be307ac04e557fe062e9ebd

        std::string_view result;

#ifdef ASBIND20_HAS_LIB_REFLECTION

        result = std::meta::display_string_of(std::meta::dealias(^^T));

#elif defined _MSC_VER && !defined __clang__ // clang-cl also defines _MSC_VER
        {
            result = __FUNCSIG__;
            auto start = result.find("type_name_of_impl<");
            start += 18; // strlen("type_name_of_impl<")
            auto stop = result.rfind('>');

            result = result.substr(start, stop - start);
        }

#elif defined __clang__
        {
            result = __PRETTY_FUNCTION__;
            auto start = result.find("T = ");
            start += 4; // strlen("T = ")
            auto stop = result.rfind(']');

            result = result.substr(start, stop - start);
        }

#elif defined __GNUC__
        {
            result = __PRETTY_FUNCTION__;
            auto start = result.find("with T = ");
            start += 9; // strlen("with T = ")
            auto stop = result.find(';', start);
            result = result.substr(start, stop - start);
        }

#else
        static_assert(!sizeof(T), "Unknown compiler");

#endif

        // Remove namespace prefix and template arguments
        {
            result = result.substr(0, result.find('<'));

            auto i = result.find("::");
            while(i != result.npos)
            {
                i += 2; // strlen("::")
                result = result.substr(i);
                i = result.find("::");
            }
        }

        return result;
    }
} // namespace detail

template <typename T>
consteval std::string_view typename_of()
{
    using type = std::remove_cvref_t<std::remove_pointer_t<T>>;
    return detail::type_name_of_impl<type>();
}

template <typename T>
consteval auto fixed_string_typename_of() noexcept
{
    constexpr std::string_view type_name = typename_of<T>();
    constexpr std::size_t size = type_name.size();

    return [&]<std::size_t... Is>(std::index_sequence<Is...>)
    {
        return util::fixed_string<size>(type_name[Is]...);
    }(std::make_index_sequence<size>());
}
} // namespace asbind20::meta

#endif
