/**
 * @file validator.hpp
 * @author HenryAWE
 * @brief Validators for checking function signature at compile-time
 */

#ifndef ASBIND20_BIND_VALIDATOR_HPP
#define ASBIND20_BIND_VALIDATOR_HPP

#pragma once

#include <concepts>
#include <type_traits>
#include "../meta.hpp"
#include "../detail/calling_convention.hpp"

namespace asbind20::detail
{
template <typename T, typename Base>
concept ptrref_of =
    (std::is_pointer_v<std::decay_t<T>> || std::is_reference_v<T>) &&
    ((!std::is_pointer_v<T> || std::same_as<std::remove_cv_t<std::remove_pointer_t<T>>, Base>) &&
     (!std::is_reference_v<T> || std::same_as<std::remove_cvref_t<T>, Base>));

namespace validator
{
    struct void_
    {
        using validator_tag = void;

        template <typename U>
        constexpr bool operator()(std::in_place_type_t<U>) const
        {
            return std::is_void_v<U>;
        }
    };

    template <typename T>
    struct by_value
    {
        using validator_tag = void;

        template <typename U>
        constexpr bool operator()(std::in_place_type_t<U>) const
        {
            using type = std::remove_cv_t<U>;
            return std::same_as<T, type>;
        }
    };

    template <typename T>
    struct by_addr
    {
        using validator_tag = void;

        template <typename U>
        constexpr bool operator()(std::in_place_type_t<U>) const
        {
            return ptrref_of<U, T>;
        }
    };

    // For any reference or pointer
    template <>
    struct by_addr<void>
    {
        using validator_tag = void;

        template <typename U>
        constexpr bool operator()(std::in_place_type_t<U>) const
        {
            return std::is_pointer_v<std::decay_t<U>> || std::is_reference_v<U>;
        }
    };

    template <typename T>
    concept descriptor = requires() {
        typename T::validator_tag;
    };
} // namespace validator

template <
    validator::descriptor Return,
    validator::descriptor... Args>
class signature_matcher
{
#ifndef ASBIND20_CONFIG_NO_COMPILE_TIME_CHECKS
    template <typename Fn, std::size_t ScriptArgCount, std::size_t Offset = 0>
    static consteval bool do_match()
    {
        using traits = function_traits<Fn>;

        if constexpr(ScriptArgCount + Offset > traits::arg_count_v)
            return false;
        else
        {
            using std::tuple_element_t, std::in_place_type_t, std::in_place_type;
            using validator_tuple = std::tuple<Args...>;

            return []<std::size_t... Is>(std::index_sequence<Is...>)
            {
                bool ret = Return{}(in_place_type<typename traits::return_type>);

                return ret &&
                       (tuple_element_t<Is, validator_tuple>{}(
                            in_place_type<typename traits::template arg_type<Is + Offset>>
                        ) &&
                        ...);
            }(std::make_index_sequence<ScriptArgCount>());
        }
    }

#endif

public:
    template <typename Fn>
    constexpr bool operator()(std::in_place_type_t<Fn>) const
    {
#ifndef ASBIND20_CONFIG_NO_COMPILE_TIME_CHECKS
        using traits = function_traits<Fn>;
        constexpr std::size_t arg_count = traits::arg_count_v;

        if constexpr(arg_count != sizeof...(Args))
            return false;
        else
            return do_match<Fn, arg_count>();

#else
        return true;
#endif
    }

    template <
        typename Fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    constexpr bool operator()(
        std::in_place_type_t<Fn>,
        call_conv_t<CallConv>
    ) const
    {
#ifndef ASBIND20_CONFIG_NO_COMPILE_TIME_CHECKS
        using traits = function_traits<Fn>;
        constexpr std::size_t arg_count = traits::arg_count_v;

        if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
                     CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST)
        {
            if constexpr(arg_count - 1 != sizeof...(Args))
                return false;
            else
                return do_match<Fn, arg_count - 1, (arg_count == 1 ? 0 : 1)>();
        }
        else if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST ||
                          CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST)
        {
            if constexpr(arg_count - 1 != sizeof...(Args))
                return false;
            else
                return do_match<Fn, arg_count - 1>();
        }
        else
        {
            return (*this)(std::in_place_type<Fn>);
        }

#else
        return true;
#endif
    }
};
} // namespace asbind20::detail

#endif
