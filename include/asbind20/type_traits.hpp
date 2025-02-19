/**
 * @file type_traits.hpp
 * @author HenryAWE
 * @brief Type traits for special conversion rules
 */

#ifndef ASBIND20_TYPE_TRAITS_HPP
#define ASBIND20_TYPE_TRAITS_HPP

#pragma once

#include "detail/include_as.hpp"
#include "utility.hpp"

namespace asbind20
{
template <typename T>
struct type_traits
{};

template <>
struct type_traits<std::byte>
{
    static int set_arg(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
        AS_NAMESPACE_QUALIFIER asUINT arg,
        std::byte val
    )
    {
        return ctx->SetArgByte(
            arg, static_cast<AS_NAMESPACE_QUALIFIER asBYTE>(val)
        );
    }

    static std::byte get_arg(
        AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen,
        AS_NAMESPACE_QUALIFIER asUINT arg
    )
    {
        return static_cast<std::byte>(gen->GetArgByte(arg));
    }

    static int set_return(
        AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen, std::byte val
    )
    {
        return gen->SetReturnByte(
            static_cast<AS_NAMESPACE_QUALIFIER asBYTE>(val)
        );
    }

    static std::byte get_return(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx
    )
    {
        return static_cast<std::byte>(ctx->GetReturnByte());
    }
};

template <>
struct type_traits<script_object>
{
    static int set_arg(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
        AS_NAMESPACE_QUALIFIER asUINT arg,
        const script_object& val
    )
    {
        return ctx->SetArgObject(arg, val.get());
    }

    static script_object get_arg(
        AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen,
        AS_NAMESPACE_QUALIFIER asUINT arg
    )
    {
        return script_object(
            static_cast<AS_NAMESPACE_QUALIFIER asIScriptObject*>(gen->GetArgObject(arg))
        );
    }

    static int set_return(
        AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen, const script_object& val
    )
    {
        return gen->SetReturnObject(val.get());
    }

    static script_object get_return(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx
    )
    {
        return script_object(
            static_cast<AS_NAMESPACE_QUALIFIER asIScriptObject*>(ctx->GetReturnObject())
        );
    }
};
} // namespace asbind20

#endif
