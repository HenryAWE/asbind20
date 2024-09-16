#ifndef ASBIND20_BIND_HPP
#define ASBIND20_BIND_HPP

#pragma once

#include <cassert>
#include <concepts>
#include <type_traits>
#include <string>
#include <tuple>
#include <algorithm>
#include <functional>
#include <angelscript.h>
#include "utility.hpp"
#include "function_traits.hpp"

namespace asbind20
{
template <asECallConvTypes CallConv>
struct call_conv_t
{};

template <asECallConvTypes CallConv>
constexpr inline call_conv_t<CallConv> call_conv;

/**
 * @brief Check if `asGetLibraryOptions()` returns "AS_MAX_PORTABILITY"
 */
[[nodiscard]]
bool has_max_portability();

template <typename T>
T get_generic_arg(asIScriptGeneric* gen, asUINT idx)
{
    if constexpr(
        std::is_same_v<std::remove_cv_t<T>, asIScriptObject*> ||
        std::is_same_v<std::remove_cv_t<T>, const asIScriptObject*>
    )
    {
        return static_cast<asIScriptObject*>(gen->GetArgObject(idx));
    }
    else if constexpr(std::is_pointer_v<T>)
    {
        using value_t = std::remove_pointer_t<T>;

        void* ptr = nullptr;
        if constexpr(std::is_class_v<value_t>)
            ptr = gen->GetArgAddress(idx);
        else
            ptr = gen->GetAddressOfArg(idx);
        assert(ptr != nullptr);
        return static_cast<T*>(ptr);
    }
    else if constexpr(std::is_reference_v<T>)
    {
        using pointer_t = std::remove_reference_t<T>*;
        return *get_generic_arg<pointer_t>(gen, idx);
    }
    else if constexpr(std::is_class_v<T>)
    {
        return get_generic_arg<T&>(gen, idx);
    }
    else if constexpr(std::is_same_v<std::remove_cv_t<T>, std::byte>)
    {
        return static_cast<std::byte>(gen->GetArgByte(idx));
    }
    else if constexpr(std::is_enum_v<T>)
    {
        return static_cast<T>(get_generic_arg<int>(gen, idx));
    }
    else if constexpr(std::integral<T>)
    {
        if constexpr(sizeof(T) == sizeof(asBYTE))
            return static_cast<T>(gen->GetArgByte(idx));
        else if constexpr(sizeof(T) == sizeof(asWORD))
            return static_cast<T>(gen->GetArgWord(idx));
        else if constexpr(sizeof(T) == sizeof(asDWORD))
            return static_cast<T>(gen->GetArgDWord(idx));
        else if constexpr(sizeof(T) == sizeof(asQWORD))
            return static_cast<T>(gen->GetArgQWord(idx));
        else
            static_assert(!sizeof(T), "Integer size too large");
    }
    else if constexpr(std::floating_point<T>)
    {
        if constexpr(std::is_same_v<std::remove_cv_t<T>, float>)
            return gen->GetArgFloat(idx);
        else if constexpr(std::is_same_v<std::remove_cv_t<T>, double>)
            return gen->GetArgDouble(idx);
        else
            static_assert(!sizeof(T), "Unsupported floating point type");
    }
    else
    {
        static_assert(!sizeof(T), "Unsupported type");
    }
}

template <typename T>
void set_generic_return(asIScriptGeneric* gen, std::type_identity_t<T>&& ret)
{
    if constexpr(std::is_reference_v<T>)
    {
        using pointer_t = std::remove_reference_t<T>*;
        set_generic_return<pointer_t>(gen, std::addressof(ret));
    }
    else if constexpr(std::is_pointer_v<T>)
    {
        void* ptr = (void*)ret;

        if constexpr(
            std::is_same_v<std::remove_cv_t<T>, asIScriptObject*> ||
            std::is_same_v<std::remove_cv_t<T>, const asIScriptObject*>
        )
            gen->SetReturnObject(ptr);
        else
        {
            using value_t = std::remove_pointer_t<T>;

            if(std::is_class_v<T>)
                gen->SetReturnObject(ptr);
            else
                gen->SetReturnAddress(ptr);
        }
    }
    else if constexpr(std::is_class_v<T>)
    {
        using pointer_t = T*;
        set_generic_return<pointer_t>(gen, std::addressof(ret));
    }
    else if constexpr(std::is_same_v<std::remove_cv_t<T>, std::byte>)
    {
        gen->SetReturnByte(static_cast<asBYTE>(ret));
    }
    else if constexpr(std::is_enum_v<T>)
    {
        set_generic_return<int>(gen, static_cast<int>(ret));
    }
    else if constexpr(std::integral<T>)
    {
        if constexpr(sizeof(T) == sizeof(asBYTE))
            gen->SetReturnByte(ret);
        else if constexpr(sizeof(T) == sizeof(asWORD))
            gen->SetReturnWord(ret);
        else if constexpr(sizeof(T) == sizeof(asDWORD))
            gen->SetReturnDWord(ret);
        else if constexpr(sizeof(T) == sizeof(asQWORD))
            gen->SetReturnQWord(ret);
        else
            static_assert(!sizeof(T), "Integer size too large");
    }
    else if constexpr(std::floating_point<T>)
    {
        if constexpr(std::is_same_v<std::remove_cv_t<T>, float>)
            gen->SetReturnFloat(ret);
        else if constexpr(std::is_same_v<std::remove_cv_t<T>, double>)
            gen->SetReturnDouble(ret);
        else
            static_assert(!sizeof(T), "Unsupported floating point type");
    }
    else
    {
        static_assert(!sizeof(T), "Unsupported type");
    }
}

class register_helper_base
{
public:
    register_helper_base() = delete;
    register_helper_base(const register_helper_base&) noexcept = default;

    register_helper_base(asIScriptEngine* engine)
        : m_engine(engine)
    {
        assert(engine != nullptr);
    }

    asIScriptEngine* get_engine() const noexcept
    {
        return m_engine;
    }

protected:
    asIScriptEngine* const m_engine;
};

namespace detail
{
    template <typename T, typename Class>
    static constexpr bool is_this_arg_v =
        std::is_same_v<T, Class*> ||
        std::is_same_v<T, const Class*> ||
        std::is_same_v<T, Class&> ||
        std::is_same_v<T, const Class&>;

    template <typename Class, typename... Args>
    static consteval asECallConvTypes deduce_method_callconv() noexcept
    {
        using args_t = std::tuple<Args...>;
        constexpr std::size_t arg_count = sizeof...(Args);
        using first_arg_t = std::tuple_element_t<0, args_t>;
        using last_arg_t = std::tuple_element_t<sizeof...(Args) - 1, args_t>;

        if constexpr(arg_count == 1 && std::is_same_v<first_arg_t, asIScriptGeneric*>)
            return asCALL_GENERIC;
        else
        {
            constexpr bool obj_first = is_this_arg_v<std::remove_cv_t<first_arg_t>, Class>;
            constexpr bool obj_last = is_this_arg_v<std::remove_cv_t<last_arg_t>, Class> && arg_count != 1;

            static_assert(obj_last || obj_first, "Missing object parameter");

            if(obj_first)
                return arg_count == 1 ? asCALL_CDECL_OBJLAST : asCALL_CDECL_OBJFIRST;
            else
                return asCALL_CDECL_OBJLAST;
        }
    }
} // namespace detail

using generic_function_t = void(asIScriptGeneric* gen);

template <typename T>
concept native_function =
    !std::is_convertible_v<T, generic_function_t*> &&
    (std::is_function_v<T> ||
     std::is_function_v<std::remove_pointer_t<T>> ||
     std::is_member_function_pointer_v<T>);

template <
    native_function auto Function,
    asECallConvTypes OriginalConv>
requires(OriginalConv != asCALL_GENERIC)
class generic_wrapper_t
{
public:
    using function_type = decltype(Function);

    static_assert(
        !std::is_member_function_pointer_v<function_type> || OriginalConv == asCALL_THISCALL,
        "Invalid calling convention"
    );

    constexpr generic_wrapper_t() noexcept = default;

    constexpr generic_wrapper_t(const generic_wrapper_t&) noexcept = default;

    static constexpr function_type underlying_function() noexcept
    {
        return Function;
    }

    static constexpr asECallConvTypes underlying_convention() noexcept
    {
        return OriginalConv;
    }

    static constexpr generic_function_t* generate()
    {
        return &wrapper_impl;
    }

    constexpr operator generic_function_t*() const
    {
        return generate();
    }

private:
    static decltype(auto) get_this_arg(asIScriptGeneric* gen)
    {
        using traits = function_traits<function_type>;

        void* ptr = gen->GetObject();

        if constexpr(OriginalConv == asCALL_THISCALL)
        {
            using pointer_t = typename traits::class_type*;
            return static_cast<pointer_t>(ptr);
        }
        else if constexpr(OriginalConv == asCALL_CDECL_OBJFIRST)
        {
            using this_arg_t = typename traits::first_arg_type;
            if constexpr(std::is_pointer_v<this_arg_t>)
                return static_cast<this_arg_t>(ptr);
            else
                return *static_cast<std::remove_reference_t<this_arg_t>*>(ptr);
        }
        else if constexpr(OriginalConv == asCALL_CDECL_OBJLAST)
        {
            using this_arg_t = typename traits::last_arg_type;
            if constexpr(std::is_pointer_v<this_arg_t>)
                return static_cast<this_arg_t>(ptr);
            else
                return *static_cast<std::remove_reference_t<this_arg_t>*>(ptr);
        }
        else
            static_assert(!OriginalConv, "Invalid type");
    }

    static void wrapper_impl(asIScriptGeneric* gen)
    {
        using traits = function_traits<function_type>;

        if constexpr(OriginalConv == asCALL_THISCALL)
        {
            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                if constexpr(std::is_void_v<typename traits::return_type>)
                {
                    std::invoke(
                        Function,
                        get_this_arg(gen),
                        get_generic_arg<typename traits::template arg_type<Is>>(
                            gen, static_cast<asUINT>(Is)
                        )...
                    );
                }
                else
                {
                    set_generic_return<typename traits::return_type>(
                        gen,
                        std::invoke(
                            Function,
                            get_this_arg(gen),
                            get_generic_arg<typename traits::template arg_type<Is>>(
                                gen, static_cast<asUINT>(Is)
                            )...
                        )
                    );
                }
            }(std::make_index_sequence<traits::arg_count::value>());
        }
        else if constexpr(OriginalConv == asCALL_CDECL_OBJFIRST)
        {
            static_assert(traits::arg_count::value >= 1);

            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                if constexpr(std::is_void_v<typename traits::return_type>)
                {
                    std::invoke(
                        Function,
                        get_this_arg(gen),
                        get_generic_arg<typename traits::template arg_type<Is + 1>>(
                            gen, static_cast<asUINT>(Is)
                        )...
                    );
                }
                else
                {
                    set_generic_return<typename traits::return_type>(
                        gen,
                        std::invoke(
                            Function,
                            get_this_arg(gen),
                            get_generic_arg<typename traits::template arg_type<Is + 1>>(
                                gen, static_cast<asUINT>(Is)
                            )...
                        )
                    );
                }
            }(std::make_index_sequence<traits::arg_count::value - 1>());
        }
        else if constexpr(OriginalConv == asCALL_CDECL_OBJLAST)
        {
            static_assert(traits::arg_count::value >= 1);

            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                if constexpr(std::is_void_v<typename traits::return_type>)
                {
                    std::invoke(
                        Function,
                        get_generic_arg<typename traits::template arg_type<Is>>(
                            gen, static_cast<asUINT>(Is)
                        )...,
                        get_this_arg(gen)
                    );
                }
                else
                {
                    set_generic_return<typename traits::return_type>(
                        gen,
                        std::invoke(
                            Function,
                            get_generic_arg<typename traits::template arg_type<Is>>(
                                gen, static_cast<asUINT>(Is)
                            )...,
                            get_this_arg(gen)
                        )
                    );
                }
            }(std::make_index_sequence<traits::arg_count::value - 1>());
        }
        else
        {
            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                if constexpr(std::is_void_v<typename traits::return_type>)
                {
                    std::invoke(
                        Function,
                        get_generic_arg<typename traits::template arg_type<Is>>(
                            gen, static_cast<asUINT>(Is)
                        )...
                    );
                }
                else
                {
                    set_generic_return<typename traits::return_type>(
                        gen,
                        std::invoke(
                            Function,
                            get_generic_arg<typename traits::template arg_type<Is>>(
                                gen, static_cast<asUINT>(Is)
                            )...
                        )
                    );
                }
            }(std::make_index_sequence<traits::arg_count::value>());
        }
    }
};

template <
    native_function auto Function,
    asECallConvTypes OriginalConv>
constexpr inline generic_wrapper_t<Function, OriginalConv> generic_wrapper{};

namespace detail
{
    class class_register_helper_base : public register_helper_base
    {
    protected:
        const char* m_name;

        class_register_helper_base() = delete;
        class_register_helper_base(const class_register_helper_base&) = default;

        class_register_helper_base(asIScriptEngine* engine, const char* name);

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

        void method_impl(const char* decl, void (*fn)(asIScriptGeneric*), call_conv_t<asCALL_GENERIC>)
        {
            int r = m_engine->RegisterObjectMethod(
                m_name,
                decl,
                asFunctionPtr(fn),
                asCALL_GENERIC
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

        template <typename Class, typename R, typename... Args>
        void method_wrapper_impl(const char* decl, R (*fn)(Args...))
        {
            method_impl(
                decl, fn, call_conv<deduce_method_callconv<Class, Args...>()>
            );
        }

        template <typename Class, typename R, typename... Args>
        void behaviour_wrapper_impl(asEBehaviours beh, const char* decl, R (*fn)(Args...))
        {
            behaviour_impl(
                beh, decl, fn, call_conv<deduce_method_callconv<Class, Args...>()>
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

        template <typename Class>
        void opPreInc_impl()
        {
            method_impl(
                string_concat(m_name, "& opPreInc()").c_str(),
                static_cast<Class& (Class::*)()>(&Class::operator++),
                call_conv<asCALL_THISCALL>
            );
        }

        template <typename Class>
        void opPreDec_impl()
        {
            method_impl(
                string_concat(m_name, "& opPreDec()").c_str(),
                static_cast<Class& (Class::*)()>(&Class::operator--),
                call_conv<asCALL_THISCALL>
            );
        }

        std::string opEquals_decl() const
        {
            return string_concat("bool opEquals(const ", m_name, " &in) const");
        }

        template <typename Class>
        requires std::equality_comparable<Class>
        void opEquals_impl()
        {
            constexpr bool has_member_op_eq = requires() {
                static_cast<bool (Class::*)(const Class&) const>(&Class::operator==);
            };

            if constexpr(has_member_op_eq)
            {
                method_impl(
                    opEquals_decl().c_str(),
                    static_cast<bool (Class::*)(const Class&) const>(&Class::operator==),
                    call_conv<asCALL_THISCALL>
                );
            }
            else
            {
                // Reverse parameter order for asCALL_CDECL_OBJLAST
                auto wrapper_obj_last = +[](const Class& rhs, const Class& lhs)
                    -> bool
                {
                    return lhs == rhs;
                };
                method_impl(
                    opEquals_decl().c_str(),
                    wrapper_obj_last,
                    call_conv<asCALL_CDECL_OBJLAST>
                );
            }
        }

        std::string opCmp_decl() const
        {
            return string_concat("int opCmp(const ", m_name, " &in) const");
        }

        template <typename Class>
        requires std::three_way_comparable<Class, std::weak_ordering>
        void opCmp_impl()
        {
            // Reverse parameter order for asCALL_CDECL_OBJLAST
            auto wrapper_obj_last = +[](const Class& rhs, const Class& lhs)
                -> int
            {
                return translate_three_way(lhs <=> rhs);
            };
            method_impl(
                opCmp_decl().c_str(),
                wrapper_obj_last,
                call_conv<asCALL_CDECL_OBJLAST>
            );
        }

        std::string opAdd_decl() const
        {
            return string_concat(m_name, " opAdd(const ", m_name, " &in) const");
        }

        template <typename Class>
        void opAdd_impl()
        {
            constexpr bool has_member_op_add = requires() {
                static_cast<Class (Class::*)(const Class&) const>(&Class::operator+);
            };

            if constexpr(has_member_op_add)
            {
                method_impl(
                    opAdd_decl().c_str(),
                    static_cast<Class (Class::*)(const Class&) const>(&Class::operator+),
                    call_conv<asCALL_THISCALL>
                );
            }
            else
            {
                // Reverse parameter order for asCALL_CDECL_OBJLAST
                auto wrapper_obj_last = +[](const Class& rhs, const Class& lhs)
                    -> Class
                {
                    return lhs + rhs;
                };
                method_impl(
                    opAdd_decl().c_str(),
                    wrapper_obj_last,
                    call_conv<asCALL_CDECL_OBJLAST>
                );
            }
        }

        void member_funcdef_impl(std::string_view decl)
        {
            // Firstly, find the begin of parameters
            auto param_being = std::find(decl.rbegin(), decl.rend(), '(');
            if(param_being != decl.rend())
            {
                ++param_being;
                param_being = std::find_if_not(
                    param_being,
                    decl.rend(),
                    [](char ch)
                    { return ch != ' '; }
                );
            }
            auto name_begin = std::find_if_not(
                param_being,
                decl.rend(),
                [](char ch)
                {
                    return '0' <= ch && ch <= '9' ||
                           'a' <= ch && ch <= 'z' ||
                           'A' <= ch && ch <= 'Z' ||
                           ch == '_' ||
                           ch > 127;
                }
            );

            std::string full_decl = string_concat(
                std::string_view(decl.begin(), name_begin.base()),
                ' ',
                m_name,
                "::",
                std::string_view(name_begin.base(), decl.end())
            );
            full_funcdef(full_decl.c_str());
        }

    private:
        void full_funcdef(const char* decl)
        {
            int r = m_engine->RegisterFuncdef(decl);
            assert(r >= 0);
        }
    };
} // namespace detail

class global : public register_helper_base
{
public:
    global() = delete;
    global(const global&) = default;

    global(asIScriptEngine* engine);

    template <typename Return, typename... Args>
    global& function(
        const char* decl,
        Return (*fn)(Args...)
    )
    {
        int r = m_engine->RegisterGlobalFunction(
            decl,
            asFunctionPtr(fn),
            asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    template <typename T, typename Return, typename Class, typename... Args>
    global& function(
        const char* decl,
        Return (Class::*fn)(Args...),
        T&& instance
    ) requires(std::is_convertible_v<T, Class>)
    {
        int r = m_engine->RegisterGlobalFunction(
            decl,
            asSMethodPtr<sizeof(fn)>::Convert(fn),
            asCALL_THISCALL_ASGLOBAL,
            std::addressof(instance)
        );
        assert(r >= 0);

        return *this;
    }

    template <typename T>
    global& property(
        const char* decl,
        T&& val
    )
    {
        int r = m_engine->RegisterGlobalProperty(
            decl, (void*)std::addressof(val)
        );
        assert(r >= 0);

        return *this;
    }

    global& function(
        const char* decl,
        generic_function_t gfn,
        void* auxiliary = nullptr
    )
    {
        int r = m_engine->RegisterGlobalFunction(
            decl,
            asFunctionPtr(gfn),
            asCALL_GENERIC,
            auxiliary
        );
        assert(r >= 0);

        return *this;
    }

    global& funcdef(
        const char* decl
    )
    {
        int r = m_engine->RegisterFuncdef(decl);
        assert(r >= 0);

        return *this;
    }

    global& typedef_(
        const char* type_decl,
        const char* new_name
    )
    {
        int r = m_engine->RegisterTypedef(new_name, type_decl);
        assert(r >= 0);

        return *this;
    }

    // For those who feel more comfortable with the C++11 style `using alias = type`
    global& using_(
        const char* new_name,
        const char* type_decl
    )
    {
        typedef_(type_decl, new_name);

        return *this;
    }

    template <typename Enum>
    requires std::is_enum_v<Enum>
    using enum_value_name_pair = std::pair<Enum, const char*>;

    global& enum_type(
        const char* type
    )
    {
        int r = m_engine->RegisterEnum(type);
        assert(r >= 0);

        return *this;
    }

    template <typename Enum>
    requires std::is_enum_v<Enum>
    global& enum_value(
        const char* type,
        Enum val,
        const char* name
    )
    {
        static_assert(
            sizeof(std::underlying_type_t<Enum>) <= sizeof(int),
            "Enum value too big"
        );

        int r = m_engine->RegisterEnumValue(
            type,
            name,
            static_cast<int>(val)
        );
        assert(r >= 0);

        return *this;
    }
};

template <typename Class>
class value_class : private detail::class_register_helper_base
{
    static constexpr bool use_pod_v = std::is_trivial_v<Class> && std::is_standard_layout_v<Class>;

    using my_base = detail::class_register_helper_base;

    template <typename... Args>
    void constructor_impl(const char* decl)
    {
        behaviour_impl(
            asBEHAVE_CONSTRUCT,
            decl,
            +[](Args... args, void* mem)
            {
                new(mem) Class(std::forward<Args>(args)...);
            },
            call_conv<asCALL_CDECL_OBJLAST>
        );
    }

public:
    using class_type = Class;

    value_class(asIScriptEngine* engine, const char* name, asQWORD flags = 0)
        : my_base(engine, name), m_flags(asOBJ_VALUE | flags | asGetTypeTraits<Class>())
    {
        assert(!(m_flags & asOBJ_REF));

        int r = m_engine->RegisterObjectType(
            m_name,
            sizeof(Class),
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
        if constexpr(std::is_default_constructible_v<Class>)
        {
            if(m_flags & asOBJ_APP_CLASS_CONSTRUCTOR)
                default_constructor();
        }

        if constexpr(std::is_copy_constructible_v<Class>)
        {
            if(m_flags & asOBJ_APP_CLASS_COPY_CONSTRUCTOR)
                copy_constructor();
        }

        if constexpr(std::is_copy_assignable_v<Class>)
        {
            if(m_flags & asOBJ_APP_CLASS_ASSIGNMENT)
                opAssign();
        }

        if(m_flags & asOBJ_APP_CLASS_DESTRUCTOR)
            destructor();

        return *this;
    }

    template <typename... Args>
    value_class& constructor(const char* decl) requires(std::is_constructible_v<Class, Args...>)
    {
        if constexpr(sizeof...(Args) != 0)
        {
            using first_arg_t = std::tuple_element_t<0, std::tuple<Args...>>;
            if constexpr(!(std::is_same_v<first_arg_t, const Class&> || std::is_same_v<first_arg_t, Class&>))
            {
                assert(m_flags & asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
            }
        }

        constructor_impl<Args...>(decl);

        return *this;
    }

    value_class& default_constructor() requires(std::is_default_constructible_v<Class>)
    {
        if(m_flags & asOBJ_POD)
            return *this;

        constructor<>("void f()");

        return *this;
    }

    value_class& copy_constructor() requires(std::is_copy_constructible_v<Class>)
    {
        if(m_flags & asOBJ_POD)
            return *this;

        if constexpr(std::is_constructible_v<Class, const Class&>)
        {
            constructor<const Class&>(
                string_concat("void f(const ", m_name, " &in)").c_str()
            );
        }
        else
        {
            constructor<Class&>(
                string_concat("void f(", m_name, " &in)").c_str()
            );
        }

        return *this;
    }

    value_class& destructor()
    {
        if(m_flags & asOBJ_POD)
            return *this;

        behaviour(
            asBEHAVE_DESTRUCT,
            "void f()",
            +[](void* ptr)
            {
                reinterpret_cast<Class*>(ptr)->~Class();
            },
            call_conv<asCALL_CDECL_OBJLAST>
        );

        return *this;
    }

    value_class& opPreInc()
    {
        opPreInc_impl<Class>();

        return *this;
    }

    value_class& opPreDec()
    {
        opPreDec_impl<Class>();

        return *this;
    }

    // Returning type by value in native calling convention
    // is NOT supported on all common platforms, e.g. x64 Linux.
    // Use generic calling convention as fallback.

    value_class& opPostInc()
    {
        auto wrapper = +[](asIScriptGeneric* gen) -> void
        {
            Class* this_ = static_cast<Class*>(gen->GetObject());
            set_generic_return<Class>(gen, (*this_)++);
        };
        method_impl(
            string_concat(m_name, " opPostInc()").c_str(),
            wrapper,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    value_class& opPostDec()
    {
        auto wrapper = +[](asIScriptGeneric* gen) -> void
        {
            Class* this_ = static_cast<Class*>(gen->GetObject());
            set_generic_return<Class>(gen, (*this_)--);
        };
        method_impl(
            string_concat(m_name, " opPostDec()").c_str(),
            wrapper,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    value_class& opAssign() requires(std::is_copy_assignable_v<Class>)
    {
        if(use_pod_v)
            return *this;
        if(m_flags & asOBJ_POD)
            return *this;

        method(
            string_concat(m_name, "& opAssign(const ", m_name, " &in)").c_str(),
            static_cast<Class& (Class::*)(const Class&)>(&Class::operator=)
        );

        return *this;
    }

    value_class& opAddAssign()
    {
        method(
            string_concat(m_name, "& opAddAssign(const ", m_name, " &in)").c_str(),
            static_cast<Class& (Class::*)(const Class&)>(&Class::operator+=)
        );

        return *this;
    }

    value_class& opEquals()
    {
        opEquals_impl<Class>();

        return *this;
    }

    value_class& opCmp()
    {
        opCmp_impl<Class>();

        return *this;
    }

    value_class& opAdd()
    {
        opAdd_impl<Class>();

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

    template <native_function Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    value_class& method(const char* decl, Fn&& fn)
    {
        method_impl(decl, std::forward<Fn>(fn), call_conv<asCALL_THISCALL>);

        return *this;
    }

    template <native_function Fn, asECallConvTypes CallConv>
    requires(CallConv != asCALL_GENERIC)
    value_class& method(const char* decl, Fn&& fn, call_conv_t<CallConv>)
    {
        method_impl(decl, std::forward<Fn>(fn), call_conv<CallConv>);

        return *this;
    }

    value_class& method(const char* decl, generic_function_t* gfn)
    {
        method_impl(decl, gfn, call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <typename R, typename... Args>
    value_class& method(const char* decl, R (*fn)(Args...))
    {
        method_wrapper_impl<Class>(decl, fn);

        return *this;
    }

    template <typename U>
    value_class& property(const char* decl, U Class::*mp)
    {
        property_impl<U, Class>(decl, mp);

        return *this;
    }

private:
    asQWORD m_flags;
};

template <typename Class>
class ref_class : private detail::class_register_helper_base
{
    using my_base = detail::class_register_helper_base;

public:
    using class_type = Class;

    ref_class(asIScriptEngine* engine, const char* name, asQWORD flags = 0)
        : my_base(engine, name), m_flags(asOBJ_REF | flags)
    {
        assert(!(m_flags & asOBJ_VALUE));

        int r = m_engine->RegisterObjectType(
            m_name,
            sizeof(Class),
            m_flags
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
    ref_class& template_callback(Fn&& fn)
    {
        int r = m_engine->RegisterObjectBehaviour(
            m_name,
            asBEHAVE_TEMPLATE_CALLBACK,
            "bool f(int&in,bool&out)",
            asFunctionPtr(fn),
            asCALL_CDECL
        );
        assert(r >= 0);

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
        method_wrapper_impl<Class>(decl, fn);

        return *this;
    }

    ref_class& default_factory()
        requires std::is_default_constructible_v<Class>
    {
        assert(!(m_flags & asOBJ_TEMPLATE));
        factory(
            string_concat(m_name, "@ f()").c_str(),
            +[]()
            {
                return new Class();
            }
        );

        return *this;
    }

    ref_class& template_default_factory()
        requires std::is_constructible_v<Class, asITypeInfo*>
    {
        assert(m_flags & asOBJ_TEMPLATE);
        factory(
            string_concat(m_name, "@ f(int&in)").c_str(),
            +[](asITypeInfo* ti)
            {
                return new Class(ti);
            }
        );

        return *this;
    }

    template <typename... Args>
    ref_class& factory(const char* decl, Class* (*fn)(Args...))
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

    template <typename... Args>
    requires std::is_constructible_v<Class, Args...>
    ref_class& factory(const char* decl)
    {
        factory(
            decl,
            +[](Args... args) -> Class*
            {
                return new Class(args...);
            }
        );

        return *this;
    }

    template <typename... Args>
    ref_class& list_factory(const char* decl, Class* (*fn)(Args...))
    {
        int r = m_engine->RegisterObjectBehaviour(
            m_name,
            asBEHAVE_LIST_FACTORY,
            decl,
            asFunctionPtr(fn),
            asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    ref_class& template_list_factory(std::string_view repeated_type_name)
        requires std::is_constructible_v<Class, asITypeInfo*, void*>
    {
        list_factory(
            string_concat(m_name, "@ f(int&in,int&in) {repeat ", repeated_type_name, "}").c_str(),
            +[](asITypeInfo* ti, void* list_buf)
            {
                return new Class(ti, list_buf);
            }
        );

        return *this;
    }

    ref_class& opPreInc()
    {
        opPreInc_impl<Class>();

        return *this;
    }

    ref_class& opPreDec()
    {
        opPreDec_impl<Class>();

        return *this;
    }

    ref_class& opEquals()
    {
        opEquals_impl<Class>();

        return *this;
    }

    ref_class& opAssign() requires std::is_copy_assignable_v<Class>
    {
        method(
            string_concat(m_name, "& opAssign(const ", m_name, " &in)").c_str(),
            static_cast<Class& (Class::*)(const Class&)>(&Class::operator=)
        );

        return *this;
    }

    ref_class& addref(void (Class::*fn)())
    {
        behaviour(
            asBEHAVE_ADDREF,
            "void f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class& release(void (Class::*fn)())
    {
        behaviour(
            asBEHAVE_RELEASE,
            "void f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class& get_refcount(int (Class::*fn)() const)
    {
        behaviour(
            asBEHAVE_GETREFCOUNT,
            "int f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class& set_flag(void (Class::*fn)())
    {
        behaviour(
            asBEHAVE_SETGCFLAG,
            "void f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class& get_flag(bool (Class::*fn)() const)
    {
        behaviour(
            asBEHAVE_GETGCFLAG,
            "bool f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class& enum_refs(void (Class::*fn)(asIScriptEngine*))
    {
        behaviour(
            asBEHAVE_ENUMREFS,
            "void f(int&in)",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class& release_refs(void (Class::*fn)(asIScriptEngine*))
    {
        behaviour(
            asBEHAVE_RELEASEREFS,
            "void f(int&in)",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    template <typename U>
    ref_class& property(const char* decl, U Class::*mp)
    {
        property_impl<U, Class>(decl, mp);

        return *this;
    }

    ref_class& funcdef(std::string_view decl)
    {
        member_funcdef_impl(decl);

        return *this;
    }

private:
    asQWORD m_flags;
};

class interface
{
public:
    interface() = delete;
    interface(const interface&) = default;

    interface(asIScriptEngine* engine, const char* name)
        : m_engine(engine), m_name(name)
    {
        int r = m_engine->RegisterInterface(m_name);
        assert(r >= 0);
    }

    interface& method(const char* decl)
    {
        int r = m_engine->RegisterInterfaceMethod(
            m_name,
            decl
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
