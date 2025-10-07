/**
 * @file generic.hpp
 * @author HenryAWE
 * @brief Helpers for generic calling convention
 */

#ifndef ASBIND20_GENERIC_HPP
#define ASBIND20_GENERIC_HPP

#pragma once

#include <algorithm>
#include <functional>
#include "detail/include_as.hpp"
#include "meta.hpp"
#include "type_traits.hpp"

namespace asbind20
{
#ifdef _MSC_VER
#    pragma warning(push)
// When binding functions that may directly throw an exception,
// MSVC will report warning of unreachable code.
#    pragma warning(disable : 4702)
#endif

/**
 * @brief Get pointer/reference to the object
 *
 * @tparam T Object type, can be a pointer, otherwise the return type will a reference
 */
template <typename T>
auto get_generic_object(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
    -> std::conditional_t<std::is_pointer_v<T>, T, std::add_lvalue_reference_t<T>>
{
    void* obj = gen->GetObject();
    if constexpr(std::is_pointer_v<T>)
    {
        return (T)obj;
    }
    else
    {
        using pointer_t = std::add_pointer_t<std::remove_reference_t<T>>;
        return *(pointer_t)obj;
    }
}

template <typename T>
auto get_generic_auxiliary(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
    -> std::conditional_t<std::is_pointer_v<T>, T, std::add_lvalue_reference_t<T>>
{
    void* obj = gen->GetAuxiliary();
    if constexpr(std::is_pointer_v<T>)
    {
        return (T)obj;
    }
    else
    {
        using pointer_t = std::add_pointer_t<std::remove_reference_t<T>>;
        return *(pointer_t)obj;
    }
}

template <typename T>
T get_generic_arg(
    AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen,
    AS_NAMESPACE_QUALIFIER asUINT idx
)
{
    constexpr bool is_customized = requires() {
        { type_traits<std::remove_cv_t<T>>::get_arg(gen, idx) } -> std::convertible_to<T>;
    };

    if constexpr(is_customized)
    {
        return type_traits<std::remove_cv_t<T>>::get_arg(gen, idx);
    }
    else if constexpr(std::is_pointer_v<T>)
    {
        using value_t = std::remove_cv_t<std::remove_pointer_t<T>>;

        if constexpr(std::same_as<value_t, AS_NAMESPACE_QUALIFIER asIScriptObject>)
        {
            void* ptr = gen->GetArgObject(idx);
            return static_cast<AS_NAMESPACE_QUALIFIER asIScriptObject*>(ptr);
        }
        else if constexpr(std::same_as<value_t, AS_NAMESPACE_QUALIFIER asITypeInfo>)
        {
            return *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(idx);
        }
        else if constexpr(std::same_as<value_t, AS_NAMESPACE_QUALIFIER asIScriptEngine>)
        {
            return *(AS_NAMESPACE_QUALIFIER asIScriptEngine**)gen->GetAddressOfArg(idx);
        }
        else
        {
            void* ptr = gen->GetArgAddress(idx);
            return (T)ptr;
        }
    }
    else if constexpr(std::is_reference_v<T>)
    {
        using pointer_t = std::remove_reference_t<T>*;
        return *get_generic_arg<pointer_t>(gen, idx);
    }
    else if constexpr(std::is_class_v<T>)
    {
        using pointer_t = std::remove_reference_t<T>*;
        return std::move(*(pointer_t)gen->GetArgObject(idx));
    }
    else if constexpr(std::is_enum_v<T>)
    {
        return static_cast<T>(get_generic_arg<int>(gen, idx));
    }
    else if constexpr(std::integral<T>)
    {
        if constexpr(sizeof(T) == sizeof(AS_NAMESPACE_QUALIFIER asBYTE))
            return static_cast<T>(gen->GetArgByte(idx));
        else if constexpr(sizeof(T) == sizeof(AS_NAMESPACE_QUALIFIER asWORD))
            return static_cast<T>(gen->GetArgWord(idx));
        else if constexpr(sizeof(T) == sizeof(AS_NAMESPACE_QUALIFIER asDWORD))
            return static_cast<T>(gen->GetArgDWord(idx));
        else if constexpr(sizeof(T) == sizeof(AS_NAMESPACE_QUALIFIER asQWORD))
            return static_cast<T>(gen->GetArgQWord(idx));
        else
            static_assert(!sizeof(T), "Integer size too large");
    }
    else if constexpr(std::floating_point<T>)
    {
        if constexpr(std::same_as<std::remove_cv_t<T>, float>)
            return gen->GetArgFloat(idx);
        else if constexpr(std::same_as<std::remove_cv_t<T>, double>)
            return gen->GetArgDouble(idx);
        else
            static_assert(!sizeof(T), "Unsupported floating point type");
    }
    else
    {
        static_assert(!sizeof(T), "Unsupported type");
    }
}

template <typename Return>
void set_generic_return(
    AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen,
    std::type_identity_t<Return>&& ret
)
{
    constexpr bool is_customized = requires() {
        { type_traits<std::remove_cv_t<Return>>::set_return(gen, std::forward<Return>(ret)) } -> std::same_as<int>;
    };

    if constexpr(is_customized)
    {
        type_traits<std::remove_cv_t<Return>>::set_return(gen, std::forward<Return>(ret));
    }
    else if constexpr(std::is_reference_v<Return>)
    {
        using pointer_t = std::remove_reference_t<Return>*;
        set_generic_return<pointer_t>(gen, std::addressof(ret));
    }
    else if constexpr(std::is_pointer_v<Return>)
    {
        void* ptr = (void*)ret;

        if constexpr(
            std::same_as<std::remove_cv_t<Return>, AS_NAMESPACE_QUALIFIER asIScriptObject*> ||
            std::same_as<std::remove_cv_t<Return>, const AS_NAMESPACE_QUALIFIER asIScriptObject*>
        )
            gen->SetReturnObject(ptr);
        else
        {
            gen->SetReturnAddress(ptr);
        }
    }
    else if constexpr(std::is_class_v<Return>)
    {
        if constexpr(meta::is_constructible_at_v<Return, Return&&>)
        {
            void* mem = gen->GetAddressOfReturnLocation();
            new(mem) Return(std::forward<Return>(ret));
        }
        else
        {
            gen->SetReturnObject((void*)std::addressof(ret));
        }
    }
    else if constexpr(std::is_enum_v<Return>)
    {
        set_generic_return<int>(gen, static_cast<int>(ret));
    }
    else if constexpr(std::integral<Return>)
    {
        if constexpr(sizeof(Return) == sizeof(AS_NAMESPACE_QUALIFIER asBYTE))
            gen->SetReturnByte(static_cast<AS_NAMESPACE_QUALIFIER asBYTE>(ret));
        else if constexpr(sizeof(Return) == sizeof(AS_NAMESPACE_QUALIFIER asWORD))
            gen->SetReturnWord(static_cast<AS_NAMESPACE_QUALIFIER asWORD>(ret));
        else if constexpr(sizeof(Return) == sizeof(AS_NAMESPACE_QUALIFIER asDWORD))
            gen->SetReturnDWord(static_cast<AS_NAMESPACE_QUALIFIER asDWORD>(ret));
        else if constexpr(sizeof(Return) == sizeof(AS_NAMESPACE_QUALIFIER asQWORD))
            gen->SetReturnQWord(static_cast<AS_NAMESPACE_QUALIFIER asQWORD>(ret));
        else // Compiler extension like __int128
            new(gen->GetAddressOfReturnLocation()) Return(ret);
    }
    else if constexpr(std::floating_point<Return>)
    {
        if constexpr(std::same_as<std::remove_cv_t<Return>, float>)
            gen->SetReturnFloat(ret);
        else if constexpr(std::same_as<std::remove_cv_t<Return>, double>)
            gen->SetReturnDouble(ret);
        else
            static_assert(!sizeof(Return), "Unsupported floating point type");
    }
    else
    {
        static_assert(!sizeof(Return), "Unsupported type");
    }
}

/**
 * @brief Set generic return value by invocation result
 *
 * @note Use this function instead of directly using the `set_generic_return` for non-moveable types.
 *       This function will try to use NRVO for non-moveable types.
 *
 * @tparam Return Return type
 *
 * @param gen The interface for the generic calling convention
 * @param fn Function to invoke
 * @param args Arguments for the function
 */
template <typename Return, typename Fn, typename... Args>
void set_generic_return_by(
    AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen,
    Fn&& fn,
    Args&&... args
)
{
    constexpr bool is_customized = requires() {
        { type_traits<std::remove_cv_t<Return>>::set_return(gen, std::declval<Return>()) } -> std::same_as<int>;
    };
    constexpr bool non_moveable =
        !(std::is_reference_v<Return> || std::is_pointer_v<Return>) &&
        std::is_class_v<Return> &&
        (!is_customized ||
         std::is_move_constructible_v<Return>);

    if constexpr(std::is_void_v<Return>)
    {
        std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
    else if constexpr(non_moveable) // Try NRVO for non-moveable types
    {
        void* mem = gen->GetAddressOfReturnLocation();
        new(mem) Return(std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...));
    }
    else
    {
        set_generic_return<Return>(
            gen,
            std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...)
        );
    }
}

/**
 * @brief Get the `this` pointer based on calling convention of wrapped function
 *
 * @tparam FunctionType The function signature of wrapped function
 * @tparam CallConv Calling convention of original function, must support `this` pointer
 *
 * @note For `asCALL_THISCALL_OBJFIRST/LAST`, this function will return `this` for the `OBJFIRST/LAST`.
 *       Please use the `get_generic_auxiliary` for the auxiliary pointer. @sa get_generic_auxiliary
 */
template <
    typename FunctionType,
    AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
decltype(auto) get_generic_this(
    AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen
)
{
    using traits = function_traits<FunctionType>;

    constexpr bool from_auxiliary =
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL;

    void* ptr = nullptr;
    if constexpr(from_auxiliary)
        ptr = gen->GetAuxiliary();
    else
        ptr = gen->GetObject();

    if constexpr(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL
    )
    {
        using pointer_t = typename traits::class_type*;
        return static_cast<pointer_t>(ptr);
    }
    else if constexpr(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST
    )
    {
        using this_arg_t = typename traits::first_arg_type;
        if constexpr(std::is_pointer_v<this_arg_t>)
            return static_cast<this_arg_t>(ptr);
        else
            return *static_cast<std::remove_reference_t<this_arg_t>*>(ptr);
    }
    else if constexpr(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST
    )
    {
        using this_arg_t = typename traits::last_arg_type;
        if constexpr(std::is_pointer_v<this_arg_t>)
            return static_cast<this_arg_t>(ptr);
        else
            return *static_cast<std::remove_reference_t<this_arg_t>*>(ptr);
    }
    else
        static_assert(!CallConv && false, "This calling convention doesn't have a this pointer");
}

#ifdef _MSC_VER
#    pragma warning(pop)
#endif
} // namespace asbind20

#endif
