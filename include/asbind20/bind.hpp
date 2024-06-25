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
        std::is_same_v<T, const Class*> ||
        std::is_same_v<T, Class&> ||
        std::is_same_v<T, const Class&>;
} // namespace detail

template <asECallConvTypes CallConv>
struct call_conv_t
{};

template <asECallConvTypes CallConv>
constexpr inline call_conv_t<CallConv> call_conv;

namespace detail
{
    class class_register_base
    {
    public:
        class_register_base() = delete;
        class_register_base(const class_register_base&) = default;

        class_register_base(asIScriptEngine* engine, const char* name);

    protected:
        asIScriptEngine* m_engine;
        const char* m_name;

        bool force_generic() const noexcept
        {
            return m_force_generic;
        }

        template <typename Fn>
        void method_impl(const char* decl, Fn&& fn, call_conv_t<asCALL_THISCALL>)
        {
            int r = m_engine->RegisterObjectMethod(
                m_name,
                decl,
                asSMethodPtr<sizeof(Fn)>::Convert(fn),
                asCALL_THISCALL
            );
            assert(r >= 0);
        }

        template <typename Fn>
        void method_impl(const char* decl, Fn&& fn, call_conv_t<asCALL_CDECL_OBJFIRST>)
        {
            int r = m_engine->RegisterObjectMethod(
                m_name,
                decl,
                asFunctionPtr(fn),
                asCALL_CDECL_OBJFIRST
            );
            assert(r >= 0);
        }

        template <typename Fn>
        void method_impl(const char* decl, Fn&& fn, call_conv_t<asCALL_CDECL_OBJLAST>)
        {
            int r = m_engine->RegisterObjectMethod(
                m_name,
                decl,
                asFunctionPtr(fn),
                asCALL_CDECL_OBJLAST
            );
            assert(r >= 0);
        }

        template <typename Fn>
        void behaviour_impl(asEBehaviours beh, const char* decl, Fn&& fn, call_conv_t<asCALL_THISCALL>)
        {
            int r = m_engine->RegisterObjectBehaviour(
                m_name,
                beh,
                decl,
                asSMethodPtr<sizeof(Fn)>::Convert(fn),
                asCALL_THISCALL
            );
            assert(r >= 0);
        }

        template <typename Fn>
        void behaviour_impl(asEBehaviours beh, const char* decl, Fn&& fn, call_conv_t<asCALL_CDECL_OBJFIRST>)
        {
            int r = m_engine->RegisterObjectBehaviour(
                m_name,
                beh,
                decl,
                asFunctionPtr(fn),
                asCALL_CDECL_OBJFIRST
            );
            assert(r >= 0);
        }

        template <typename Fn>
        void behaviour_impl(asEBehaviours beh, const char* decl, Fn&& fn, call_conv_t<asCALL_CDECL_OBJLAST>)
        {
            int r = m_engine->RegisterObjectBehaviour(
                m_name,
                beh,
                decl,
                asFunctionPtr(fn),
                asCALL_CDECL_OBJLAST
            );
            assert(r >= 0);
        }

        template <typename Class, typename... Args>
        static consteval asECallConvTypes call_conv_from_args() noexcept
        {
            using args_t = std::tuple<Args...>;
            constexpr std::size_t arg_count = sizeof...(Args);
            using first_arg_t = std::tuple_element_t<0, args_t>;
            using last_arg_t = std::tuple_element_t<sizeof...(Args) - 1, args_t>;

            constexpr bool obj_first = is_this_arg<std::remove_cv_t<first_arg_t>, Class>;
            constexpr bool obj_last = is_this_arg<std::remove_cv_t<last_arg_t>, Class> && arg_count != 1;

            static_assert(obj_last || obj_first, "Missing object parameter");

            if(obj_first)
                return asCALL_CDECL_OBJFIRST;
            else
                return asCALL_CDECL_OBJLAST;
        }

        template <typename Class, typename R, typename... Args>
        void method_wrapper_impl(const char* decl, R (*fn)(Args...))
        {
            method_impl(
                decl, fn, call_conv<call_conv_from_args<Class, Args...>()>
            );
        }

        template <typename Class, typename R, typename... Args>
        void behaviour_wrapper_impl(asEBehaviours beh, const char* decl, R (*fn)(Args...))
        {
            behaviour_impl(
                beh, decl, fn, call_conv<call_conv_from_args<Class, Args...>()>
            );
        }

        template <typename T, typename Class>
        void property_impl(const char* decl, T Class::*mp)
        {
            int r = m_engine->RegisterObjectProperty(
                m_name,
                decl,
                static_cast<int>(member_offset(mp))
            );
            assert(r >= 0);
        }

    private:
        bool m_force_generic;
    };
} // namespace detail

template <typename T>
class value_class : private detail::class_register_base
{
    static constexpr bool use_pod_v = std::is_trivial_v<T> && std::is_standard_layout_v<T>;

    using my_base = detail::class_register_base;

public:
    value_class(asIScriptEngine* engine, const char* name, asDWORD flags = asGetTypeTraits<T>())
        : my_base(engine, name), m_flags(asOBJ_VALUE | flags)
    {
        assert(!(m_flags & asOBJ_REF));

        int r = m_engine->RegisterObjectType(
            m_name,
            sizeof(T),
            m_flags
        );
        assert(r >= 0);
    }

    /**
     * @brief Registering common behaviours based on flags.
     *
     * Registering default constructor, copy constructor,
     * copy assignment operator (opAssign/operator=), and destructor based on flags of the type.
     */
    value_class& common_behaviours()
    {
        if constexpr(std::is_default_constructible_v<T>)
        {
            if(m_flags & asOBJ_APP_CLASS_CONSTRUCTOR)
                default_constructor();
        }

        if constexpr(std::is_copy_constructible_v<T>)
        {
            if(m_flags & asOBJ_APP_CLASS_COPY_CONSTRUCTOR)
                copy_constructor();
        }

        if constexpr(std::is_copy_assignable_v<T>)
        {
            if(m_flags & asOBJ_APP_CLASS_ASSIGNMENT)
                op_assign();
        }

        if(m_flags & asOBJ_APP_CLASS_DESTRUCTOR)
            destructor();

        return *this;
    }

    template <typename... Args>
    value_class& constructor(const char* decl) requires(std::is_constructible_v<T, Args...>)
    {
        behaviour(
            asBEHAVE_CONSTRUCT,
            decl,
            +[](void* mem, Args... args)
            {
                new(mem) T(std::forward<Args>(args)...);
            },
            call_conv<asCALL_CDECL_OBJFIRST>
        );

        return *this;
    }

    value_class& default_constructor() requires(std::is_default_constructible_v<T>)
    {
        if constexpr(use_pod_v)
            return *this;
        if(m_flags & asOBJ_POD)
            return *this;

        constructor<>("void f()");

        return *this;
    }

    value_class& copy_constructor() requires(std::is_copy_constructible_v<T>)
    {
        if constexpr(std::is_constructible_v<T, const T&>)
        {
            constructor<const T&>(
                detail::concat("void f(const ", m_name, " &in)").c_str()
            );
        }
        else
        {
            constructor<T&>(
                detail::concat("void f(", m_name, " &in)").c_str()
            );
        }

        return *this;
    }

    value_class& destructor()
    {
        if constexpr(use_pod_v)
            return *this;
        if(m_flags & asOBJ_POD)
            return *this;

        behaviour(
            asBEHAVE_DESTRUCT,
            "void f()",
            +[](void* ptr)
            {
                reinterpret_cast<T*>(ptr)->~T();
            },
            call_conv<asCALL_CDECL_OBJLAST>
        );

        return *this;
    }

    value_class& op_assign() requires(std::is_copy_assignable_v<T>)
    {
        if(use_pod_v)
            return *this;
        if(m_flags & asOBJ_POD)
            return *this;

        method(
            detail::concat(m_name, "& opAssign(const ", m_name, " &in)").c_str(),
            static_cast<T& (T::*)(const T&)>(&T::operator=)
        );

        return *this;
    }

    template <typename Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    value_class& behaviour(asEBehaviours beh, const char* decl, Fn&& fn, call_conv_t<asCALL_THISCALL>)
    {
        behaviour_impl(beh, decl, std::forward<Fn>(fn), call_conv<asCALL_THISCALL>);

        return *this;
    }

    template <typename R, typename... Args, asECallConvTypes CallConv>
    value_class& behaviour(asEBehaviours beh, const char* decl, R (*fn)(Args...), call_conv_t<CallConv>)
    {
        behaviour_impl(beh, decl, fn, call_conv<CallConv>);

        return *this;
    }

    template <typename Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    value_class& method(const char* decl, Fn&& fn)
    {
        method_impl(decl, std::forward<Fn>(fn), call_conv<asCALL_THISCALL>);

        return *this;
    }

    template <typename Fn, asECallConvTypes CallConv>
    value_class& method(const char* decl, Fn&& fn, call_conv_t<CallConv>)
    {
        method_impl(decl, std::forward<Fn>(fn), call_conv<CallConv>);

        return *this;
    }

    template <typename R, typename... Args>
    value_class& method(const char* decl, R (*fn)(Args...))
    {
        method_wrapper_impl<T>(decl, fn);

        return *this;
    }

    template <typename U>
    value_class& property(const char* decl, U T::*mp)
    {
        property_impl<U, T>(decl, mp);

        return *this;
    }

private:
    asDWORD m_flags;
};

template <typename T>
class ref_class : private detail::class_register_base
{
    using my_base = detail::class_register_base;

public:
    ref_class(asIScriptEngine* engine, const char* name, asDWORD flags = 0)
        : my_base(engine, name)
    {
        assert(!(flags & asOBJ_VALUE));

        int r = m_engine->RegisterObjectType(
            m_name,
            sizeof(T),
            asOBJ_REF | flags
        );
        assert(r >= 0);
    }

    template <typename Fn>
    ref_class& behaviour(asEBehaviours beh, const char* decl, Fn&& fn, call_conv_t<asCALL_THISCALL>)
    {
        behaviour_impl(beh, decl, std::forward<Fn>(fn), call_conv<asCALL_THISCALL>);

        return *this;
    }

    template <typename Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    ref_class& method(const char* decl, Fn&& fn)
    {
        method_impl(decl, std::forward<Fn>(fn), call_conv<asCALL_THISCALL>);

        return *this;
    }

    template <typename Fn, asECallConvTypes CallConv>
    ref_class& method(const char* decl, Fn&& fn, call_conv_t<CallConv>)
    {
        method_impl(decl, std::forward<Fn>(fn), call_conv<CallConv>);

        return *this;
    }

    template <typename R, typename... Args>
    ref_class& method(const char* decl, R (*fn)(Args...))
    {
        method_wrapper_impl<T>(decl, fn);

        return *this;
    }

    ref_class& default_factory()
    {
        factory(
            detail::concat(m_name, "@ f()").c_str(),
            +[]()
            {
                return new T();
            }
        );

        return *this;
    }

    template <typename... Args>
    ref_class& factory(const char* decl, T* (*fn)(Args...))
    {
        int r = m_engine->RegisterObjectBehaviour(
            m_name,
            asBEHAVE_FACTORY,
            decl,
            asFunctionPtr(fn),
            asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    ref_class& addref(void (T::*fn)())
    {
        behaviour(
            asBEHAVE_ADDREF,
            "void f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class& release(void (T::*fn)())
    {
        behaviour(
            asBEHAVE_RELEASE,
            "void f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    template <typename U>
    ref_class& property(const char* decl, U T::*mp)
    {
        property_impl<U, T>(decl, mp);

        return *this;
    }
};
} // namespace asbind20

#endif
