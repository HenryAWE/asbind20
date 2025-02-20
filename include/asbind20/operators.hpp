/**
 * @file operators.hpp
 * @author HenryAWE
 * @brief Tools for registering operators
 *
 * For the predefined operator name in AngelScript, see:
 * https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_class_ops.html
 */

#ifndef ASBIND20_OPERATORS_HPP
#define ASBIND20_OPERATORS_HPP

#pragma once

#include "utility.hpp"

namespace asbind20
{
struct self_t
{};

constexpr inline self_t self{};

template <typename T>
struct param_t
{};

template <typename T>
constexpr inline param_t<T> param{};

namespace operators
{
    template <typename Lhs, typename Rhs>
    struct binary_operator_tag
    {
        using operator_proxy_tag = void;

        static constexpr std::size_t operand_count = 2;

        using lhs_type = Lhs;
        using rhs_type = Rhs;
    };

    template <typename Lhs, typename Rhs>
    struct opAdd;

    template <>
    struct opAdd<self_t, self_t> : binary_operator_tag<self_t, self_t>
    {
        static constexpr auto name = meta::fixed_string("opAdd");

        template <typename Self>
        using result_type = decltype(std::declval<const Self>() + std::declval<const Self>());

        template <typename Self>
        static result_type<Self> call(const Self& lhs, const Self& rhs)
        {
            return lhs + rhs;
        }

        static std::string declaration(std::string_view self_name)
        {
            return string_concat(
                self_name, " opAdd(", self_name, "&in,", self_name, "&in)"
            );
        }
    };

    template <has_static_name Rhs>
    struct opAdd<self_t, Rhs> : binary_operator_tag<self_t, Rhs>
    {
        static constexpr auto name = meta::fixed_string("opAdd");

        template <typename Self>
        using result_type = decltype(std::declval<Self>() + std::declval<Rhs>());

        template <typename Self>
        static result_type<Self> call(Self& lhs, Rhs& rhs)
        {
            return lhs + rhs;
        }
    };

    template <has_static_name Lhs>
    struct opAdd<Lhs, self_t> : binary_operator_tag<Lhs, self_t>
    {
        static constexpr auto name = meta::fixed_string("opAdd_r");

        template <typename Self>
        using result_type = decltype(std::declval<Lhs>() + std::declval<Self>());

        template <typename Self>
        static result_type<Self> call(Lhs& lhs, Self& rhs)
        {
            return lhs + rhs;
        }
    };

    template <typename Self, typename Proxy>
    concept has_fixed_declaration =
        requires() { {Proxy::operand_count} -> std::convertible_to<std::size_t>; } &&
        has_static_name<typename Proxy::template result_type<Self>>;

    template <typename Self, typename Proxy>
    requires(has_fixed_declaration<Self, Proxy>)
    constexpr auto fixed_declaration()
    {
        using result_type = typename Proxy::template result_type<Self>;
        using lhs_type = std::conditional_t<
            std::same_as<typename Proxy::lhs_type, self_t>,
            Self,
            typename Proxy::lhs_type>;
        using rhs_type = std::conditional_t<
            std::same_as<typename Proxy::rhs_type, self_t>,
            Self,
            typename Proxy::rhs_type>;

        if constexpr(Proxy::operand_count == 2)
        {
            constexpr auto lhs_mod =
                std::is_const_v<std::remove_reference_t<lhs_type>> ?
                    AS_NAMESPACE_QUALIFIER asTM_INREF :
                    AS_NAMESPACE_QUALIFIER asTM_INOUTREF;
            constexpr auto rhs_mod =
                std::is_const_v<std::remove_reference_t<rhs_type>> ?
                    AS_NAMESPACE_QUALIFIER asTM_INREF :
                    AS_NAMESPACE_QUALIFIER asTM_INOUTREF;

            return meta::full_fixed_name_of<result_type, AS_NAMESPACE_QUALIFIER asTM_NONE>() +
                   meta::fixed_string(' ') +
                   Proxy::name +
                   meta::fixed_string('(') +
                   meta::full_fixed_name_of<lhs_type, lhs_mod>() +
                   meta::fixed_string(',') +
                   meta::full_fixed_name_of<rhs_type, rhs_mod>() +
                   meta::fixed_string(')');
        }
    }

    template <typename T>
    concept proxy = requires() {
        typename T::operator_proxy_tag;
    };
} // namespace operators

consteval auto operator+(self_t, self_t) -> operators::opAdd<self_t, self_t>
{
    return {};
}

template <has_static_name Rhs>
constexpr auto operator+(self_t, param_t<Rhs>) -> operators::opAdd<self_t, Rhs>
{
    return {};
}
} // namespace asbind20

#endif
