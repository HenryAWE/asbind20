#ifndef ASBIND20_BIND_HPP
#define ASBIND20_BIND_HPP

#pragma once

#include <cassert>
#include <type_traits>
#include <string>
#include <tuple>
#include <angelscript.h>
#include "utility.hpp"

namespace asbind20
{
namespace detail
{
    template <typename T, typename Class>
    concept is_this_arg =
        std::is_same_v<T, Class*> ||
        std::is_same_v<T, const Class*>;
} // namespace detail

template <asECallConvTypes CallConv>
struct call_conv_t
{};

template <asECallConvTypes CallConv>
constexpr inline call_conv_t<CallConv> call_conv;

template <typename T>
class value_class
{
    static constexpr bool use_pod_v = std::is_trivial_v<T> && std::is_standard_layout_v<T>;

public:
    value_class(asIScriptEngine* engine, const char* name)
        : m_engine(engine), m_name(name)
    {
        asWORD flags = asOBJ_VALUE | asGetTypeTraits<T>();

        int r = m_engine->RegisterObjectType(
            m_name,
            sizeof(T),
            flags
        );
        assert(r >= 0);
    }

    value_class& register_basic_methods()
    {
        if constexpr(std::is_default_constructible_v<T>)
            register_default_ctor();

        if constexpr(std::is_copy_constructible_v<T>)
            register_copy_ctor();

        if constexpr(std::is_copy_assignable_v<T>)
            register_op_assign();

        register_dtor();

        return *this;
    }

    value_class& register_default_ctor() requires(std::is_default_constructible_v<T>)
    {
        if constexpr(use_pod_v)
            return *this;

        static auto* wrapper = +[](void* mem)
        {
            new(mem) T();
        };

        int r = m_engine->RegisterObjectBehaviour(
            m_name,
            asBEHAVE_CONSTRUCT,
            "void f()",
            asFUNCTION(wrapper),
            asCALL_CDECL_OBJLAST
        );
        assert(r >= 0);

        return *this;
    }

    value_class& register_copy_ctor()
    {
        using namespace std::literals;

        static auto* wrapper = +[](const T& other, void* mem)
        {
            new(mem) T(other);
        };

        int r = m_engine->RegisterObjectBehaviour(
            m_name,
            asBEHAVE_CONSTRUCT,
            detail::concat("void f(const "s, m_name, " &in)").c_str(),
            asFUNCTION(wrapper),
            asCALL_CDECL_OBJLAST
        );
        assert(r >= 0);

        return *this;
    }

    value_class& register_dtor()
    {
        if constexpr(use_pod_v)
            return *this;

        static auto* wrapper = +[](void* ptr)
        {
            reinterpret_cast<T*>(ptr)->~T();
        };

        int r = m_engine->RegisterObjectBehaviour(
            m_name,
            asBEHAVE_DESTRUCT,
            "void f()",
            asFUNCTION(wrapper),
            asCALL_CDECL_OBJLAST
        );
        assert(r >= 0);

        return *this;
    }

    value_class& register_op_assign()
    {
        if(use_pod_v)
            return *this;

        using namespace std::literals;

        int r = m_engine->RegisterObjectMethod(
            m_name,
            detail::concat(m_name, "& opAssign(const "s, m_name, " &in)").c_str(),
            asMETHODPR(T, operator=, (const T&), T&),
            asCALL_THISCALL
        );
        assert(r >= 0);

        return *this;
    }

    template <typename Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    value_class& method(const char* decl, Fn&& fn)
    {
        int r = m_engine->RegisterObjectMethod(
            m_name,
            decl,
            asSMethodPtr<sizeof(Fn)>::Convert(fn),
            asCALL_THISCALL
        );
        assert(r >= 0);

        return *this;
    }

    template <typename Fn>
    value_class& method(const char* decl, Fn&& fn, call_conv_t<asCALL_CDECL_OBJFIRST>)
    {
        int r = m_engine->RegisterObjectMethod(
            m_name,
            decl,
            asFunctionPtr(fn),
            asCALL_CDECL_OBJFIRST
        );
        assert(r >= 0);

        return *this;
    }

    template <typename Fn>
    value_class& method(const char* decl, Fn&& fn, call_conv_t<asCALL_CDECL_OBJLAST>)
    {
        int r = m_engine->RegisterObjectMethod(
            m_name,
            decl,
            asFunctionPtr(fn),
            asCALL_CDECL_OBJLAST
        );
        assert(r >= 0);

        return *this;
    }

    template <typename R, typename... Args>
    value_class& method(const char* decl, R (*fn)(Args...))
    {
        static_assert(sizeof...(Args) != 0, "Empty parameters");

        method_wrapper_impl<R, Args...>(decl, fn);

        return *this;
    }

private:
    asIScriptEngine* m_engine;
    const char* m_name;

    template <typename R, typename... Args>
    void method_wrapper_impl(const char* decl, R (*fn)(Args...))
    {
        using args_t = std::tuple<Args...>;
        constexpr std::size_t arg_count = sizeof...(Args);
        using first_arg_t = std::tuple_element_t<0, args_t>;
        using last_arg_t = std::tuple_element_t<sizeof...(Args) - 1, args_t>;

        constexpr bool obj_last = detail::is_this_arg<std::remove_cvref_t<last_arg_t>, T>;
        constexpr bool obj_first = detail::is_this_arg<std::remove_cvref_t<first_arg_t>, T> && arg_count != 1;

        static_assert(!(obj_last && obj_first), "Ambiguous object parameter");
        static_assert(obj_last || obj_first, "Missing object parameter");

        if constexpr(obj_last)
            method(decl, fn, call_conv<asCALL_CDECL_OBJLAST>);
        else if constexpr(obj_first)
            method(decl, fn, call_conv<asCALL_CDECL_OBJFIRST>);
    }
};
} // namespace asbind20

#endif
