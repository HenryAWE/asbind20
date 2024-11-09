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
#include "detail/include_as.hpp"
#include "utility.hpp"
#include "function_traits.hpp"

namespace asbind20
{
class [[nodiscard]] namespace_
{
public:
    namespace_() = delete;
    namespace_(const namespace_&) = delete;

    namespace_& operator=(const namespace_&) = delete;

    namespace_(asIScriptEngine* engine)
        : m_engine(engine), m_prev(engine->GetDefaultNamespace())
    {
        m_engine->SetDefaultNamespace("");
    }

    namespace_(asIScriptEngine* engine, const char* ns, bool nested = true)
        : m_engine(engine), m_prev(engine->GetDefaultNamespace())
    {
        if(nested)
        {
            if(ns[0] != '\0') [[likely]]
            {
                if(m_prev.empty())
                    m_engine->SetDefaultNamespace(ns);
                else
                    m_engine->SetDefaultNamespace(string_concat(m_prev, "::", ns).c_str());
            }
        }
        else
        {
            m_engine->SetDefaultNamespace(ns);
        }
    }

    ~namespace_()
    {
        m_engine->SetDefaultNamespace(m_prev.c_str());
    }

private:
    asIScriptEngine* m_engine;
    std::string m_prev;
};

template <asECallConvTypes CallConv>
struct call_conv_t
{};

template <asECallConvTypes CallConv>
constexpr inline call_conv_t<CallConv> call_conv;

struct use_generic_t
{};

constexpr inline use_generic_t use_generic{};

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

template <bool ForceGeneric>
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

    static constexpr bool force_generic() noexcept
    {
        return ForceGeneric;
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

    template <auto Function, typename Class>
    consteval asECallConvTypes deduce_method_callconv()
    {
        if constexpr(std::is_member_function_pointer_v<std::decay_t<decltype(Function)>>)
        {
            return asCALL_THISCALL;
        }
        else
        {
            using traits = function_traits<std::decay_t<decltype(Function)>>;

            return []<std::size_t... Is>(std::index_sequence<Is...>)
            {
                return deduce_method_callconv<Class, typename traits::template arg_type<Is>...>();
            }(std::make_index_sequence<traits::arg_count_v>());
        }
    }
} // namespace detail

using generic_function_t = void(asIScriptGeneric* gen);

namespace detail
{
    template <typename T>
    concept is_native_function_helper = std::is_function_v<T> ||
                                        std::is_function_v<std::remove_pointer_t<T>> ||
                                        std::is_member_function_pointer_v<T>;
}

template <typename T>
concept native_function =
    !std::is_convertible_v<T, generic_function_t*> &&
    detail::is_native_function_helper<std::decay_t<T>>;

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

    static constexpr generic_function_t* generate() noexcept
    {
        return &wrapper_impl;
    }

    constexpr explicit operator generic_function_t*() const noexcept
    {
        return generate();
    }

    constexpr generic_function_t* operator()() const noexcept
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

template <bool ForceGeneric>
class global_t final : public register_helper_base<ForceGeneric>
{
    using my_base = register_helper_base<ForceGeneric>;

    using my_base::m_engine;

public:
    global_t() = delete;
    global_t(const global_t&) = default;

    global_t(asIScriptEngine* engine)
        : my_base(engine) {}

    template <typename Return, typename... Args>
    global_t& function(
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

    template <
        native_function auto Function,
        asECallConvTypes CallConv = asCALL_CDECL>
    global_t& function(const char* decl)
    {
        if(has_max_portability())
        {
            function(
                decl,
                generic_wrapper<Function, CallConv>()
            );
        }
        else
        {
            function(
                decl,
                Function
            );
        }

        return *this;
    }

    template <typename T, typename Return, typename Class, typename... Args>
    global_t& function(
        const char* decl,
        Return (Class::*fn)(Args...),
        T& instance
    )
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

    template <typename T, typename Return, typename Class, typename... Args>
    global_t& function(
        const char* decl,
        Return (Class::*fn)(Args...),
        const T& instance
    )
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
    global_t& property(
        const char* decl,
        T& val
    )
    {
        int r = m_engine->RegisterGlobalProperty(
            decl, (void*)std::addressof(val)
        );
        assert(r >= 0);

        return *this;
    }

    template <typename T>
    global_t& property(
        const char* decl,
        const T& val
    )
    {
        int r = m_engine->RegisterGlobalProperty(
            decl, (void*)std::addressof(val)
        );
        assert(r >= 0);

        return *this;
    }

    global_t& function(
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

    global_t& funcdef(
        const char* decl
    )
    {
        int r = m_engine->RegisterFuncdef(decl);
        assert(r >= 0);

        return *this;
    }

    global_t& typedef_(
        const char* type_decl,
        const char* new_name
    )
    {
        int r = m_engine->RegisterTypedef(new_name, type_decl);
        assert(r >= 0);

        return *this;
    }

    // For those who feel more comfortable with the C++11 style `using alias = type`
    global_t& using_(
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

    global_t& enum_type(
        const char* type
    )
    {
        int r = m_engine->RegisterEnum(type);
        assert(r >= 0);

        return *this;
    }

    template <typename Enum>
    requires std::is_enum_v<Enum>
    global_t& enum_value(
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

using global = global_t<false>;

template <bool ForceGeneric>
class class_register_helper_base : public register_helper_base<ForceGeneric>
{
    using my_base = register_helper_base<ForceGeneric>;

protected:
    using my_base::m_engine;
    const char* m_name;

    class_register_helper_base() = delete;
    class_register_helper_base(const class_register_helper_base&) = default;

    class_register_helper_base(asIScriptEngine* engine, const char* name)
        : my_base(engine), m_name(name) {}

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

    void behaviour_impl(asEBehaviours beh, const char* decl, generic_function_t* gfn, call_conv_t<asCALL_GENERIC>)
    {
        int r = m_engine->RegisterObjectBehaviour(
            m_name,
            beh,
            decl,
            asFunctionPtr(gfn),
            asCALL_GENERIC
        );
        assert(r >= 0);
    }

    template <typename Fn, asECallConvTypes CallConv>
    requires(CallConv != asCALL_GENERIC)
    void behaviour_impl(asEBehaviours beh, const char* decl, Fn&& fn, call_conv_t<CallConv>) requires(!ForceGeneric)
    {
        [[maybe_unused]]
        int r = 0;
        if constexpr(std::is_member_function_pointer_v<std::decay_t<Fn>>)
        {
            r = m_engine->RegisterObjectBehaviour(
                m_name,
                beh,
                decl,
                asSMethodPtr<sizeof(Fn)>::Convert(fn),
                CallConv
            );
        }
        else
        {
            r = m_engine->RegisterObjectBehaviour(
                m_name,
                beh,
                decl,
                asFunctionPtr(fn),
                CallConv
            );
        }
        assert(r >= 0);
    }

    template <native_function auto Behaviour, asECallConvTypes CallConv>
    void wrapped_behaviour_impl(asEBehaviours beh, const char* decl)
    {
        int r = 0;
        if constexpr(ForceGeneric)
        {
            r = m_engine->RegisterObjectBehaviour(
                m_name,
                beh,
                decl,
                asFunctionPtr(generic_wrapper<Behaviour, CallConv>()),
                asCALL_GENERIC
            );
        }
        else
        {
            r = m_engine->RegisterObjectBehaviour(
                m_name,
                beh,
                decl,
                asFunctionPtr(Behaviour),
                CallConv
            );
        }
        assert(r >= 0);
    }

    template <typename Class, typename R, typename... Args>
    void method_auto_callconv(const char* decl, R (*fn)(Args...))
    {
        method_impl(
            decl, fn, call_conv<detail::deduce_method_callconv<Class, Args...>()>
        );
    }

    template <typename Class, typename R, typename... Args>
    void behaviour_wrapper_impl(asEBehaviours beh, const char* decl, R (*fn)(Args...))
    {
        behaviour_impl(
            beh, decl, fn, call_conv<detail::deduce_method_callconv<Class, Args...>()>
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

template <typename Class, bool ForceGeneric = false>
class value_class final : public class_register_helper_base<ForceGeneric>
{
    static constexpr bool use_pod_v = std::is_trivial_v<Class> && std::is_standard_layout_v<Class>;

    using my_base = class_register_helper_base<ForceGeneric>;

    using my_base::m_engine;
    using my_base::m_name;

    template <typename... Args>
    void constructor_impl(const char* decl)
    {
        this->behaviour_impl(
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

    value_class(asIScriptEngine* engine, const char* name, asQWORD flags = AS_NAMESPACE_QUALIFIER asGetTypeTraits<Class>())
        : my_base(engine, name), m_flags(asOBJ_VALUE | flags)
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

    template <typename R, typename... Args, asECallConvTypes CallConv>
    value_class& constructor(const char* decl, R (*fn)(Args...), call_conv_t<CallConv>)
    {
        behaviour(
            asBEHAVE_CONSTRUCT,
            decl,
            fn,
            call_conv<CallConv>
        );

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
        this->template opPreInc_impl<Class>();

        return *this;
    }

    value_class& opPreDec()
    {
        this->template opPreDec_impl<Class>();

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
        this->method_impl(
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
        this->method_impl(
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
        this->template opEquals_impl<Class>();

        return *this;
    }

    value_class& opCmp()
    {
        this->template opCmp_impl<Class>();

        return *this;
    }

    value_class& opAdd()
    {
        this->template opAdd_impl<Class>();

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
        this->behaviour_impl(beh, decl, fn, call_conv<CallConv>);

        return *this;
    }

    template <native_function Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    value_class& method(const char* decl, Fn&& fn)
    {
        this->method_impl(decl, std::forward<Fn>(fn), call_conv<asCALL_THISCALL>);

        return *this;
    }

    template <native_function Fn, asECallConvTypes CallConv>
    requires(CallConv != asCALL_GENERIC)
    value_class& method(const char* decl, Fn&& fn, call_conv_t<CallConv>)
    {
        this->method_impl(decl, std::forward<Fn>(fn), call_conv<CallConv>);

        return *this;
    }

    value_class& method(const char* decl, generic_function_t* gfn)
    {
        this->method_impl(decl, gfn, call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Function, Class>()>
    value_class& method(use_generic_t, const char* decl)
    {
        method(decl, generic_wrapper<Function, CallConv>(), call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Function, Class>()>
    value_class& method(const char* decl)
    {
        if constexpr(ForceGeneric)
        {
            method<Function, CallConv>(use_generic, decl);
        }
        else
        {
            method(decl, Function, call_conv<CallConv>);
        }

        return *this;
    }

    template <typename R, typename... Args>
    value_class& method(const char* decl, R (*fn)(Args...))
    {
        this->template method_auto_callconv<Class>(decl, fn);

        return *this;
    }

    template <typename T>
    value_class& property(const char* decl, T Class::*mp)
    {
        this->template property_impl<T, Class>(decl, mp);

        return *this;
    }

    value_class& funcdef(std::string_view decl)
    {
        this->member_funcdef_impl(decl);

        return *this;
    }

private:
    asQWORD m_flags;
};

template <typename Class, bool Template = false, bool ForceGeneric = false>
class ref_class_t : public class_register_helper_base<ForceGeneric>
{
    using my_base = class_register_helper_base<ForceGeneric>;

    using my_base::m_engine;
    using my_base::m_name;

public:
    using class_type = Class;

    ref_class_t(asIScriptEngine* engine, const char* name, asQWORD flags = 0)
        : my_base(engine, name), m_flags(asOBJ_REF | flags)
    {
        assert(!(m_flags & asOBJ_VALUE));

        if constexpr(!Template)
        {
            assert(!(m_flags & asOBJ_TEMPLATE));
        }
        else
        {
            m_flags |= asOBJ_TEMPLATE;
        }

        int r = m_engine->RegisterObjectType(
            m_name,
            sizeof(Class),
            m_flags
        );
        assert(r >= 0);
    }

    ref_class_t& behaviour(asEBehaviours beh, const char* decl, generic_function_t* gfn, call_conv_t<asCALL_GENERIC>)
    {
        this->behaviour_impl(beh, decl, gfn, call_conv<asCALL_GENERIC>);

        return *this;
    }

    ref_class_t& behaviour(asEBehaviours beh, const char* decl, generic_function_t* gfn)
    {
        behaviour(beh, decl, gfn, call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <native_function Fn, asECallConvTypes CallConv>
    requires(CallConv != asCALL_GENERIC)
    ref_class_t& behaviour(asEBehaviours beh, const char* decl, Fn&& fn, call_conv_t<CallConv>) requires(!ForceGeneric)
    {
        this->behaviour_impl(beh, decl, std::forward<Fn>(fn), call_conv<CallConv>);

        return *this;
    }

    static constexpr char decl_template_callback[] = "bool f(int&in,bool&out)";

    ref_class_t& template_callback(generic_function_t* gfn) requires(Template)
    {
        this->behaviour_impl(
            asBEHAVE_TEMPLATE_CALLBACK,
            decl_template_callback,
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <native_function Fn>
    ref_class_t& template_callback(Fn&& fn) requires(Template && !ForceGeneric)
    {
        this->behaviour_impl(
            asBEHAVE_TEMPLATE_CALLBACK,
            decl_template_callback,
            fn,
            call_conv<asCALL_CDECL>
        );

        return *this;
    }

    template <native_function auto Callback>
    ref_class_t& template_callback(use_generic_t) requires(Template)
    {
        generic_function_t* gfn = +[](asIScriptGeneric* gen)
        {
            asITypeInfo* ti = *(asITypeInfo**)gen->GetAddressOfArg(0);
            bool* no_gc = *(bool**)gen->GetAddressOfArg(1);

            bool result = std::invoke(Callback, ti, std::ref(*no_gc));
            gen->SetReturnByte(result);
        };
        template_callback(gfn);

        return *this;
    }

    template <native_function auto Callback>
    ref_class_t& template_callback() requires(Template)
    {
        int r = 0;
        if constexpr(ForceGeneric)
        {
            template_callback<Callback>(use_generic);
        }
        else
        {
            template_callback(Callback);
        }
        assert(r >= 0);

        return *this;
    }

    template <native_function Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    ref_class_t& method(const char* decl, Fn&& fn)
    {
        this->template method_impl(decl, std::forward<Fn>(fn), call_conv<asCALL_THISCALL>);

        return *this;
    }

    template <typename Fn, asECallConvTypes CallConv>
    ref_class_t& method(const char* decl, Fn&& fn, call_conv_t<CallConv>)
    {
        this->method_impl(decl, std::forward<Fn>(fn), call_conv<CallConv>);

        return *this;
    }

    template <typename R, typename... Args>
    ref_class_t& method(const char* decl, R (*fn)(Args...))
    {
        this->template method_auto_callconv<Class>(decl, fn);

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Function, Class>()>
    ref_class_t& method(use_generic_t, const char* decl)
    {
        method(decl, generic_wrapper<Function, CallConv>(), call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Function, Class>()>
    ref_class_t& method(const char* decl)
    {
        if constexpr(ForceGeneric)
        {
            method<Function, CallConv>(use_generic, decl);
        }
        else
        {
            method(decl, Function, call_conv<CallConv>);
        }

        return *this;
    }

    template <typename... Args>
    ref_class_t& factory_function(const char* decl, Class* (*fn)(Args...)) requires(!ForceGeneric)
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

    ref_class_t& factory_function(const char* decl, generic_function_t* gfn)
    {
        int r = m_engine->RegisterObjectBehaviour(
            m_name,
            asBEHAVE_FACTORY,
            decl,
            asFunctionPtr(gfn),
            asCALL_GENERIC
        );

        return *this;
    }

    template <native_function auto Function>
    ref_class_t& factory_function(use_generic_t, const char* decl)
    {
        behaviour(
            asBEHAVE_FACTORY,
            decl,
            generic_wrapper<Function, asCALL_CDECL>()
        );

        return *this;
    }

    template <native_function auto Function>
    ref_class_t& factory_function(const char* decl)
    {
        if constexpr(ForceGeneric)
            factory_function<Function>(use_generic, decl);
        else
            factory_function(decl, Function);

        return *this;
    }

    ref_class_t& default_factory(use_generic_t)
    {
        if constexpr(Template)
        {
            factory_function(
                string_concat(m_name, "@ f(int&in)").c_str(),
                +[](asIScriptGeneric* gen) -> void
                {
                    Class* ptr = new Class(*(asITypeInfo**)gen->GetAddressOfArg(0));
                    gen->SetReturnAddress(ptr);
                }
            );
        }
        else
        {
            factory_function(
                string_concat(m_name, "@ f()").c_str(),
                +[](asIScriptGeneric* gen) -> void
                {
                    Class* ptr = new Class();
                    gen->SetReturnAddress(ptr);
                }
            );
        }

        return *this;
    }

    ref_class_t& default_factory()
    {
        if constexpr(ForceGeneric)
        {
            default_factory(use_generic);
        }
        else
        {
            if constexpr(Template)
            {
                factory_function(
                    string_concat(m_name, "@ f(int&in)").c_str(),
                    +[](asITypeInfo* ti) -> Class*
                    {
                        return new Class(ti);
                    }
                );
            }
            else
            {
                factory_function(
                    string_concat(m_name, "@ f()").c_str(),
                    +[]() -> Class*
                    {
                        return new Class();
                    }
                );
            }
        }

        return *this;
    }

    template <typename... Args>
    ref_class_t& factory(const char* decl)
        requires newable<Class, Args...>
    {
        factory_function(
            decl,
            +[](Args... args) -> Class*
            {
                return new Class(args...);
            }
        );

        return *this;
    }

    template <typename... Args>
    ref_class_t& list_factory_function(const char* decl, Class* (*fn)(Args...))
    {
        behaviour(
            asBEHAVE_LIST_FACTORY,
            decl,
            fn,
            call_conv<asCALL_CDECL>
        );

        return *this;
    }

    ref_class_t& list_factory_function(const char* decl, generic_function_t* gfn)
    {
        behaviour(
            asBEHAVE_LIST_FACTORY,
            decl,
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    ref_class_t& list_factory(std::string_view repeated_type_name) requires(Template)
    {
        std::string decl = string_concat(m_name, "@ f(int&in,int&in) {repeat ", repeated_type_name, "}");
        if constexpr(ForceGeneric)
        {
            list_factory_function(
                decl.c_str(),
                +[](asIScriptGeneric* gen) -> void
                {
                    Class* ptr = new Class(
                        /* ti = */ *(asITypeInfo**)gen->GetAddressOfArg(0),
                        /* list_buf = */ gen->GetArgAddress(1)
                    );
                    gen->SetReturnAddress(ptr);
                }
            );
        }
        else
        {
            list_factory_function(
                decl.c_str(),
                +[](asITypeInfo* ti, void* list_buf) -> Class*
                {
                    return new Class(ti, list_buf);
                }
            );
        }

        return *this;
    }

    ref_class_t& opPreInc()
    {
        this->template opPreInc_impl<Class>();

        return *this;
    }

    ref_class_t& opPreDec()
    {
        this->template opPreDec_impl<Class>();

        return *this;
    }

    ref_class_t& opEquals()
    {
        this->template opEquals_impl<Class>();

        return *this;
    }

    ref_class_t& opAssign() requires std::is_copy_assignable_v<Class>
    {
        method(
            string_concat(m_name, "& opAssign(const ", m_name, " &in)").c_str(),
            static_cast<Class& (Class::*)(const Class&)>(&Class::operator=)
        );

        return *this;
    }

    ref_class_t& addref(void (Class::*fn)())
    {
        behaviour(
            asBEHAVE_ADDREF,
            "void f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class_t& release(void (Class::*fn)())
    {
        behaviour(
            asBEHAVE_RELEASE,
            "void f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class_t& get_refcount(int (Class::*fn)() const)
    {
        behaviour(
            asBEHAVE_GETREFCOUNT,
            "int f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class_t& set_flag(void (Class::*fn)())
    {
        behaviour(
            asBEHAVE_SETGCFLAG,
            "void f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class_t& get_flag(bool (Class::*fn)() const)
    {
        behaviour(
            asBEHAVE_GETGCFLAG,
            "bool f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class_t& enum_refs(void (Class::*fn)(asIScriptEngine*))
    {
        behaviour(
            asBEHAVE_ENUMREFS,
            "void f(int&in)",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    ref_class_t& release_refs(void (Class::*fn)(asIScriptEngine*))
    {
        behaviour(
            asBEHAVE_RELEASEREFS,
            "void f(int&in)",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    template <typename T>
    ref_class_t& property(const char* decl, T Class::*mp)
    {
        this->template property_impl<T, Class>(decl, mp);

        return *this;
    }

    ref_class_t& funcdef(std::string_view decl)
    {
        this->member_funcdef_impl(decl);

        return *this;
    }

private:
    asQWORD m_flags;
};

template <typename Class>
using ref_class = ref_class_t<Class, false, false>;

template <typename Class, bool ForceGeneric = false>
using template_class = ref_class_t<Class, true, ForceGeneric>;

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
