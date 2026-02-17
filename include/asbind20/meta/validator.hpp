/**
 * @file validator.hpp
 * @author HenryAWE
 * @brief Validators for checking function signature at compile-time
 */

#ifndef ASBIND20_META_VALIDATOR_HPP
#define ASBIND20_META_VALIDATOR_HPP

#pragma once

#include <concepts>
#include <type_traits>
#include "../meta.hpp"

namespace asbind20::meta
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
public:
    template <typename Fn>
    constexpr bool operator()(std::in_place_type_t<Fn>) const
    {
        using std::tuple_element_t, std::in_place_type_t, std::in_place_type;
        using traits = function_traits<Fn>;
        using validator_tuple = std::tuple<Args...>;

        if constexpr(sizeof...(Args) != traits::arg_count_v)
            return false;
        else
        {
            return []<std::size_t... Is>(std::index_sequence<Is...>)
            {
                bool ret = Return{}(in_place_type<typename traits::return_type>);

                return ret &&
                       (tuple_element_t<Is, validator_tuple>{}(
                            in_place_type<typename traits::template arg_type<Is>>
                        ) &&
                        ...);
            }(std::make_index_sequence<sizeof...(Args)>());
        }
    }
};
} // namespace asbind20::meta

#endif
