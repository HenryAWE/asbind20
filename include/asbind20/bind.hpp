#ifndef ASBIND20_BIND_HPP
#define ASBIND20_BIND_HPP

#pragma once

#include <cassert>
#include <type_traits>
#include <string>
#include <angelscript.h>

namespace asbind20
{
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
            return;

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
            ("void f(const "s + m_name + " &in)").c_str(),
            asFUNCTION(wrapper),
            asCALL_CDECL_OBJLAST
        );
        assert(r >= 0);

        return *this;
    }

    value_class& register_dtor()
    {
        if constexpr(use_pod_v)
            return;

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
            (m_name + "& opAssign(const "s + m_name + " &in)").c_str(),
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

private:
    asIScriptEngine* m_engine;
    const char* m_name;
};
} // namespace asbind20

#endif
