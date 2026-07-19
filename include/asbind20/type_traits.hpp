/**
 * @file type_traits.hpp
 * @author HenryAWE
 * @brief Type traits for special conversion rules
 */

#ifndef ASBIND20_TYPE_TRAITS_HPP
#define ASBIND20_TYPE_TRAITS_HPP

#pragma once

#include "fwd.hpp"
#include "memory.hpp"

namespace asbind20
{
template <typename T>
struct type_traits
{};

// Forward declarations
// The following template functions are implemented in generic.hpp and invoke.hpp

template <typename T>
T get_generic_arg(
    generic_pointer gen,
    arg_index_type idx
);

template <typename Return>
void set_generic_return(
    generic_pointer gen,
    std::type_identity_t<Return>&& ret
);

template <typename T>
requires(!std::is_const_v<T>)
decltype(auto) get_script_return(context_pointer ctx);

template <std::integral T>
int set_script_arg(
    context_pointer ctx,
    arg_index_type idx,
    T val
);

/**
 * @brief Utility for quickly implementing type traits for enum with custom underlying type
 */
template <typename Enum>
requires std::is_enum_v<Enum>
struct underlying_enum_traits
{
    using underlying_type = std::underlying_type_t<Enum>;

    static int set_arg(
        context_pointer ctx,
        arg_index_type arg,
        Enum val
    )
    {
        return set_script_arg(ctx, arg, static_cast<underlying_type>(val));
    }

    static Enum get_arg(
        generic_pointer gen,
        arg_index_type arg
    )
    {
        return static_cast<Enum>(get_generic_arg<underlying_type>(gen, arg));
    }

    static int set_return(
        generic_pointer gen, Enum val
    )
    {
        return set_generic_return<underlying_type>(gen, static_cast<underlying_type>(val));
    }

    static Enum get_return(
        context_pointer ctx
    )
    {
        return static_cast<Enum>(get_script_return<underlying_type>(ctx));
    }
};

template <>
struct type_traits<std::byte> : public underlying_enum_traits<std::byte>
{};

template <>
struct type_traits<script_object>
{
    static int set_arg(
        context_pointer ctx,
        arg_index_type arg,
        const script_object& val
    )
    {
        return ctx->SetArgObject(arg, val.get());
    }

    static script_object get_arg(
        generic_pointer gen,
        arg_index_type arg
    )
    {
        return script_object(
            static_cast<object_pointer>(gen->GetArgObject(arg))
        );
    }

    static int set_return(
        generic_pointer gen, const script_object& val
    )
    {
        return gen->SetReturnObject(val.get());
    }

    static script_object get_return(
        context_pointer ctx
    )
    {
        return script_object(
            static_cast<object_pointer>(ctx->GetReturnObject())
        );
    }
};
} // namespace asbind20

#endif
