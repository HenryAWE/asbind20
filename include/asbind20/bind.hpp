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
auto get_generic_object(asIScriptGeneric* gen)
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
T get_generic_arg(asIScriptGeneric* gen, asUINT idx)
{
    constexpr bool is_customized = requires() {
        { type_traits<T>::get_arg(gen, idx) } -> std::convertible_to<T>;
    };

    if constexpr(is_customized)
    {
        return type_traits<T>::get_arg(gen, idx);
    }
    else if constexpr(
        std::is_same_v<std::remove_cv_t<T>, asITypeInfo*> ||
        std::is_same_v<std::remove_cv_t<T>, const asITypeInfo*>
    )
    {
        return *(asITypeInfo**)gen->GetAddressOfArg(idx);
    }
    else if constexpr(
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
        if constexpr(std::is_class_v<value_t> || std::is_void_v<value_t>)
            ptr = gen->GetArgAddress(idx);
        else
            ptr = gen->GetAddressOfArg(idx);
        assert(ptr != nullptr);
        return (T)ptr;
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
void set_generic_return(asIScriptGeneric* gen, T&& ret)
{
    constexpr bool is_customized = requires() {
        { type_traits<T>::set_return(gen, std::forward<T>(ret)) } -> std::same_as<int>;
    };

    if constexpr(is_customized)
    {
        type_traits<T>::set_return(gen, std::forward<T>(ret));
    }
    else if constexpr(std::is_reference_v<T>)
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
        void* mem = gen->GetAddressOfReturnLocation();
        new(mem) T(std::forward<T>(ret));
    }
    else if constexpr(std::is_enum_v<T>)
    {
        set_generic_return<int>(gen, static_cast<int>(ret));
    }
    else if constexpr(std::integral<T>)
    {
        if constexpr(sizeof(T) == sizeof(asBYTE))
            gen->SetReturnByte(static_cast<asBYTE>(ret));
        else if constexpr(sizeof(T) == sizeof(asWORD))
            gen->SetReturnWord(static_cast<asWORD>(ret));
        else if constexpr(sizeof(T) == sizeof(asDWORD))
            gen->SetReturnDWord(static_cast<asDWORD>(ret));
        else if constexpr(sizeof(T) == sizeof(asQWORD))
            gen->SetReturnQWord(static_cast<asQWORD>(ret));
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

namespace detail
{
    template <typename T>
    concept is_native_function_helper = std::is_function_v<T> ||
                                        std::is_function_v<std::remove_pointer_t<T>> ||
                                        std::is_member_function_pointer_v<T>;
}

template <typename T>
concept native_function =
    !std::is_convertible_v<T, asGENFUNC_t> &&
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

    static constexpr asGENFUNC_t generate() noexcept
    {
        return &wrapper_impl;
    }

    constexpr explicit operator asGENFUNC_t() const noexcept
    {
        return generate();
    }

    constexpr asGENFUNC_t operator()() const noexcept
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
class global final : public register_helper_base<ForceGeneric>
{
    using my_base = register_helper_base<ForceGeneric>;

    using my_base::m_engine;

public:
    global() = delete;
    global(const global&) = default;

    global(asIScriptEngine* engine)
        : my_base(engine) {}

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

    template <
        native_function auto Function,
        asECallConvTypes CallConv = asCALL_CDECL>
    global& function(use_generic_t, const char* decl)
    {
        function(
            decl,
            generic_wrapper<Function, CallConv>()
        );

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = asCALL_CDECL>
    requires(!std::is_member_function_pointer_v<std::decay_t<decltype(Function)>>)
    global& function(const char* decl)
    {
        if constexpr(ForceGeneric)
        {
            function<Function, CallConv>(use_generic, decl);
        }
        else
        {
            int r = m_engine->RegisterGlobalFunction(
                decl,
                asFunctionPtr(Function),
                CallConv
            );
            assert(r >= 0);
        }

        return *this;
    }

    template <typename T>
    global& function(
        const char* decl,
        asGENFUNC_t gfn,
        T& instance
    )
    {
        int r = m_engine->RegisterGlobalFunction(
            decl,
            asFunctionPtr(gfn),
            asCALL_GENERIC,
            (void*)std::addressof(instance)
        );
        assert(r >= 0);

        return *this;
    }

    template <typename T, typename Return, typename Class, typename... Args>
    global& function(
        const char* decl,
        Return (Class::*fn)(Args...),
        T& instance
    ) requires(!ForceGeneric)
    {
        int r = m_engine->RegisterGlobalFunction(
            decl,
            asSMethodPtr<sizeof(fn)>::Convert(fn),
            asCALL_THISCALL_ASGLOBAL,
            (void*)std::addressof(instance)
        );
        assert(r >= 0);

        return *this;
    }

    template <
        native_function auto Function,
        typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<decltype(Function)>>)
    global& function(use_generic_t, const char* decl, T& instance)
    {
        function(
            decl,
            +[](asIScriptGeneric* gen) -> void
            {
                using traits = function_traits<std::decay_t<decltype(Function)>>;

                [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                {
                    using return_t = typename traits::return_type;
                    using class_t = typename traits::class_type;
                    using args_tuple = typename traits::args_tuple;

                    class_t* this_ = (class_t*)gen->GetAuxiliary();

                    if constexpr(std::is_void_v<return_t>)
                    {
                        std::invoke(
                            Function, this_, get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is)...
                        );
                    }
                    else
                    {
                        set_generic_return<return_t>(
                            gen,
                            std::invoke(
                                Function, this_, get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is)...
                            )
                        );
                    }
                }(std::make_index_sequence<traits::arg_count_v>());
            },
            instance
        );

        return *this;
    }

    template <
        native_function auto Function,
        typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<decltype(Function)>>)
    global& function(const char* decl, T& instance)
    {
        if constexpr(ForceGeneric)
            function<Function>(use_generic, decl, instance);
        else
            function(decl, Function, instance);

        return *this;
    }

    template <typename T>
    global& property(
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

    global& function(
        const char* decl,
        asGENFUNC_t gfn,
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
            sizeof(Enum) <= sizeof(int),
            "Enum size too large"
        );

        int r = m_engine->RegisterEnumValue(
            type,
            name,
            static_cast<int>(val)
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * Generic calling convention for message callback is not supported.
     */
    global& message_callback(asGENFUNC_t gfn, void* obj = nullptr) = delete;

    template <native_function Callback>
    requires(!std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& message_callback(Callback fn, void* obj = nullptr)
    {
        int r = m_engine->SetMessageCallback(
            asFunctionPtr(fn), obj, asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    template <native_function Callback, typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& message_callback(Callback fn, T& obj)
    {
        int r = m_engine->SetMessageCallback(
            asSMethodPtr<sizeof(fn)>::Convert(fn), (void*)std::addressof(obj), asCALL_THISCALL
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * Generic calling convention for message callback is not supported
     */
    template <native_function auto Callback>
    global& message_callback(use_generic_t, void* obj = nullptr) = delete;
    template <native_function auto Callback, typename T>
    global& message_callback(use_generic_t, T& obj) = delete;

    template <native_function auto Callback>
    requires(!std::is_member_function_pointer_v<std::decay_t<decltype(Callback)>>)
    global& message_callback(void* obj = nullptr)
    {
        message_callback(Callback, obj);

        return *this;
    }

    template <native_function auto Callback, typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<decltype(Callback)>>)
    global& message_callback(T& obj)
    {
        message_callback(Callback, obj);

        return *this;
    }

    /**
     * Generic calling convention for exception translator is not supported.
     */
    global& exception_translator(asGENFUNC_t gfn, void* obj = nullptr) = delete;

    template <native_function Callback>
    requires(!std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& exception_translator(Callback fn, void* obj = nullptr)
    {
        int r = m_engine->SetTranslateAppExceptionCallback(
            asFunctionPtr(fn), obj, asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    template <native_function Callback, typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& exception_translator(Callback fn, T& obj)
    {
        int r = m_engine->SetTranslateAppExceptionCallback(
            asSMethodPtr<sizeof(fn)>::Convert(fn), (void*)std::addressof(obj), asCALL_THISCALL
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * Generic calling convention for exception translator is not supported
     */
    template <native_function auto Callback>
    global& exception_translator(use_generic_t, void* obj = nullptr) = delete;
    template <native_function auto Callback, typename T>
    global& exception_translator(use_generic_t, T& obj) = delete;

    template <native_function auto Callback>
    requires(!std::is_member_function_pointer_v<std::decay_t<decltype(Callback)>>)
    global& exception_translator(void* obj = nullptr)
    {
        exception_translator(Callback, obj);

        return *this;
    }

    template <native_function auto Callback, typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<decltype(Callback)>>)
    global& exception_translator(T& obj)
    {
        exception_translator(Callback, obj);

        return *this;
    }
};

global(asIScriptEngine*) -> global<false>;

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
    requires(std::is_member_function_pointer_v<Fn>)
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

    void method_impl(const char* decl, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC>)
    {
        int r = m_engine->RegisterObjectMethod(
            m_name,
            decl,
            asFunctionPtr(gfn),
            asCALL_GENERIC
        );
        assert(r >= 0);
    }

    template <native_function auto Method, asECallConvTypes CallConv>
    void wrapped_method_impl(const char* decl)
    {
        if constexpr(ForceGeneric)
        {
            method_impl(
                decl,
                generic_wrapper<Method, CallConv>(),
                call_conv<asCALL_GENERIC>
            );
        }
        else
        {
            method_impl(
                decl,
                Method,
                call_conv<CallConv>
            );
        }
    }

    void behaviour_impl(asEBehaviours beh, const char* decl, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC>)
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

    void property_impl(const char* decl, std::size_t off)
    {
        int r = m_engine->RegisterObjectProperty(
            m_name,
            decl,
            static_cast<int>(off)
        );
        assert(r >= 0);
    }

    template <typename T, typename Class>
    void property_impl(const char* decl, T Class::*mp)
    {
        property_impl(decl, member_offset(mp));
    }

#define ASBIND20_CLASS_UNARY_PREFIX_OP(as_op_sig, cpp_op, decl_arg_list, return_type, const_)      \
    std::string as_op_sig##_decl() const                                                           \
    {                                                                                              \
        return string_concat decl_arg_list;                                                        \
    }                                                                                              \
    template <typename Class>                                                                      \
    void as_op_sig##_impl_generic()                                                                \
    {                                                                                              \
        method_impl(                                                                               \
            as_op_sig##_decl().c_str(),                                                            \
            +[](asIScriptGeneric* gen) -> void                                                     \
            {                                                                                      \
                using this_arg_t = std::conditional_t<(#const_[0] != '\0'), const Class&, Class&>; \
                set_generic_return<return_type>(                                                   \
                    cpp_op get_generic_object<this_arg_t>(gen)                                     \
                );                                                                                 \
            },                                                                                     \
            call_conv<asCALL_GENERIC>                                                              \
        );                                                                                         \
    }                                                                                              \
    template <typename Class>                                                                      \
    void as_op_sig##_impl_native()                                                                 \
    {                                                                                              \
        method_impl(                                                                               \
            as_op_sig##_decl().c_str(),                                                            \
            static_cast<return_type (Class::*)() const_>(&Class::operator cpp_op),                 \
            call_conv<asCALL_THISCALL>                                                             \
        );                                                                                         \
    }

    ASBIND20_CLASS_UNARY_PREFIX_OP(
        opNeg, -, (m_name, " opNeg() const"), Class, const
    );

    ASBIND20_CLASS_UNARY_PREFIX_OP(
        opPreInc, ++, (m_name, "& opPreInc()"), Class&,
    );
    ASBIND20_CLASS_UNARY_PREFIX_OP(
        opPreDec, --, (m_name, "& opPreDec()"), Class&,
    );

#undef ASBIND20_CLASS_REGISTER_UNARY_PREFIX_OP

#define ASBIND20_CLASS_BINARY_OP_GENERIC(as_decl, cpp_op, return_type, const_, rhs_type)           \
    do {                                                                                           \
        this->method_impl(                                                                         \
            as_decl,                                                                               \
            +[](asIScriptGeneric* gen) -> void                                                     \
            {                                                                                      \
                using this_arg_t = std::conditional_t<(#const_[0] != '\0'), const Class&, Class&>; \
                set_generic_return<return_type>(                                                   \
                    gen,                                                                           \
                    get_generic_object<this_arg_t>(gen) cpp_op get_generic_arg<rhs_type>(gen, 0)   \
                );                                                                                 \
            },                                                                                     \
            call_conv<asCALL_GENERIC>                                                              \
        );                                                                                         \
    } while(0)

#define ASBIND20_CLASS_BINARY_OP_NATIVE(as_decl, cpp_op, return_type, const_, rhs_type)        \
    do {                                                                                       \
        static constexpr bool has_member_func = requires() {                                   \
            static_cast<return_type (Class::*)(rhs_type) const_>(&Class::operator cpp_op);     \
        };                                                                                     \
        if constexpr(has_member_func)                                                          \
        {                                                                                      \
            this->method_impl(                                                                 \
                as_decl,                                                                       \
                static_cast<return_type (Class::*)(rhs_type) const_>(&Class::operator cpp_op), \
                call_conv<asCALL_THISCALL>                                                     \
            );                                                                                 \
        }                                                                                      \
        else                                                                                   \
        {                                                                                      \
            using this_arg_t = std::conditional_t<(#const_[0] != '\0'), const Class&, Class&>; \
            this->method_impl(                                                                 \
                as_decl,                                                                       \
                +[](this_arg_t lhs, rhs_type rhs) -> return_type                               \
                {                                                                              \
                    return lhs cpp_op rhs;                                                     \
                },                                                                             \
                call_conv<asCALL_CDECL_OBJFIRST>                                               \
            );                                                                                 \
        }                                                                                      \
    } while(0)

#define ASBIND20_CLASS_BINARY_OP_IMPL(as_op_sig, cpp_op, decl_arg_list, return_type, const_, rhs_type) \
    std::string as_op_sig##_decl() const                                                               \
    {                                                                                                  \
        return string_concat decl_arg_list;                                                            \
    }                                                                                                  \
    template <typename Class>                                                                          \
    void as_op_sig##_impl_generic()                                                                    \
    {                                                                                                  \
        ASBIND20_CLASS_BINARY_OP_GENERIC(                                                              \
            as_op_sig##_decl().c_str(), cpp_op, return_type, const_, rhs_type                          \
        );                                                                                             \
    }                                                                                                  \
    template <typename Class>                                                                          \
    void as_op_sig##_impl_native()                                                                     \
    {                                                                                                  \
        ASBIND20_CLASS_BINARY_OP_NATIVE(                                                               \
            as_op_sig##_decl().c_str(), cpp_op, return_type, const_, rhs_type                          \
        );                                                                                             \
    }

    // Predefined method names:
    // https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_class_ops.html

    // Assignment operators

    ASBIND20_CLASS_BINARY_OP_IMPL(
        opAssign, =, (m_name, "& opAssign(const ", m_name, " &in)"), Class&, , const Class&
    );
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opAddAssign, +=, (m_name, "& opAddAssign(const ", m_name, " &in)"), Class&, , const Class&
    );
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opSubAssign, -=, (m_name, "& opSubAssign(const ", m_name, " &in)"), Class&, , const Class&
    );
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opMulAssign, *=, (m_name, "& opMulAssign(const ", m_name, " &in)"), Class&, , const Class&
    );
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opDivAssign, /=, (m_name, "& opDivAssign(const ", m_name, " &in)"), Class&, , const Class&
    );

    // Comparison operators

    ASBIND20_CLASS_BINARY_OP_IMPL(
        opEquals, ==, ("bool opEquals(const ", m_name, " &in) const"), bool, const, const Class&
    );

    // opCmp needs special logic to translate the result of operator<=> from C++

    std::string opCmp_decl() const
    {
        return string_concat("int opCmp(const ", m_name, "&in) const");
    }

    template <typename Class>
    void opCmp_impl_generic()
    {
        method_impl(
            opCmp_decl().c_str(),
            +[](asIScriptGeneric* gen) -> void
            {
                set_generic_return<int>(
                    gen,
                    translate_three_way(
                        get_generic_object<const Class&>(gen) <=> get_generic_arg<const Class&>(gen, 0)
                    )
                );
            },
            call_conv<asCALL_GENERIC>
        );
    }

    template <typename Class>
    void opCmp_impl_native()
    {
        method_impl(
            opCmp_decl().c_str(),
            +[](const Class& lhs, const Class& rhs) -> int
            {
                return translate_three_way(lhs <=> rhs);
            },
            call_conv<asCALL_CDECL_OBJFIRST>
        );
    }

    ASBIND20_CLASS_BINARY_OP_IMPL(
        opAdd, +, (m_name, " opAdd(const ", m_name, " &in) const"), Class, const, const Class&
    );
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opSub, -, (m_name, " opSub(const ", m_name, " &in) const"), Class, const, const Class&
    );
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opMul, *, (m_name, " opMul(const ", m_name, " &in) const"), Class, const, const Class&
    );
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opDiv, /, (m_name, " opDiv(const ", m_name, " &in) const"), Class, const, const Class&
    );

#undef ASBIND20_CLASS_BINARY_OP_GENERIC
#undef ASBIND20_CLASS_BINARY_OP_NATIVE
#undef ASBIND20_CLASS_BINARY_OP_IMPL

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

    value_class& common_behaviours(use_generic_t)
    {
        if constexpr(std::is_default_constructible_v<Class>)
        {
            if(m_flags & asOBJ_APP_CLASS_CONSTRUCTOR)
                default_constructor(use_generic);
        }

        if constexpr(std::is_copy_constructible_v<Class>)
        {
            if(m_flags & asOBJ_APP_CLASS_COPY_CONSTRUCTOR)
                copy_constructor(use_generic);
        }

        if constexpr(std::is_copy_assignable_v<Class>)
        {
            if(m_flags & asOBJ_APP_CLASS_ASSIGNMENT)
                opAssign(use_generic);
        }

        if(m_flags & asOBJ_APP_CLASS_DESTRUCTOR)
            destructor(use_generic);

        return *this;
    }

    value_class& constructor_function(const char* decl, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC>)
    {
        this->behaviour_impl(
            asBEHAVE_CONSTRUCT,
            decl,
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <typename R, typename... Args, asECallConvTypes CallConv>
    requires(CallConv != asCALL_GENERIC)
    value_class& constructor_function(const char* decl, R (*fn)(Args...), call_conv_t<CallConv>) requires(!ForceGeneric)
    {
        behaviour(
            asBEHAVE_CONSTRUCT,
            decl,
            fn,
            call_conv<CallConv>
        );

        return *this;
    }

    template <native_function auto Function, asECallConvTypes CallConv>
    requires(CallConv == asCALL_CDECL_OBJFIRST || CallConv == asCALL_CDECL_OBJLAST)
    value_class& constructor_function(use_generic_t, const char* decl)
    {
        if constexpr(CallConv == asCALL_CDECL_OBJFIRST)
        {
            constructor_function(
                decl,
                +[](asIScriptGeneric* gen) -> void
                {
                    using traits = function_traits<decltype(Function)>;
                    using args_tuple = typename traits::args_tuple;
                    [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                    {
                        Class* mem = (Class*)gen->GetObject();
                        std::invoke(
                            Function,
                            mem,
                            get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(gen, (asUINT)Is + 1)...
                        );
                    }(std::make_index_sequence<traits::arg_count_v - 1>());
                },
                call_conv<asCALL_GENERIC>
            );
        }
        else // asCALL_CDECL_OBJLAST
        {
            constructor_function(
                decl,
                +[](asIScriptGeneric* gen) -> void
                {
                    using traits = function_traits<decltype(Function)>;
                    using args_tuple = typename traits::args_tuple;
                    [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                    {
                        Class* mem = (Class*)gen->GetObject();
                        std::invoke(
                            Function,
                            get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is)...,
                            mem
                        );
                    }(std::make_index_sequence<traits::arg_count_v - 1>());
                },
                call_conv<asCALL_GENERIC>
            );
        }

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Function, Class>()>
    value_class& constructor_function(const char* decl)
    {
        if constexpr(ForceGeneric)
        {
            constructor_function<Function, CallConv>(use_generic, decl);
        }
        else
        {
            constructor_function(decl, Function, call_conv<CallConv>);
        }

        return *this;
    }

private:
    template <typename... Args>
    void constructor_impl_generic(const char* decl)
    {
        constructor_function(
            decl,
            +[](asIScriptGeneric* gen) -> void
            {
                using args_tuple = std::tuple<Args...>;
                [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                {
                    void* mem = gen->GetObject();
                    new(mem) Class(get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is)...);
                }(std::make_index_sequence<sizeof...(Args)>());
            },
            call_conv<asCALL_GENERIC>
        );
    }

    template <typename... Args>
    void constructor_impl_native(const char* decl)
    {
        constructor_function(
            decl,
            +[](Args... args, void* mem)
            {
                new(mem) Class(std::forward<Args>(args)...);
            },
            call_conv<asCALL_CDECL_OBJLAST>
        );
    }

public:
    template <typename... Args>
    value_class& constructor(use_generic_t, const char* decl) requires(std::is_constructible_v<Class, Args...>)
    {
        if constexpr(sizeof...(Args) != 0)
        {
            using first_arg_t = std::tuple_element_t<0, std::tuple<Args...>>;
            if constexpr(!(std::is_same_v<first_arg_t, const Class&> || std::is_same_v<first_arg_t, Class&>))
            {
                assert(m_flags & asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
            }
        }

        constructor_impl_generic<Args...>(decl);

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

        if constexpr(ForceGeneric)
            constructor<Args...>(use_generic, decl);
        else
            constructor_impl_native<Args...>(decl);

        return *this;
    }

    value_class& default_constructor(use_generic_t)
    {
        if(m_flags & asOBJ_POD)
            return *this;

        constructor<>(use_generic, "void f()");

        return *this;
    }

    value_class& default_constructor()
    {
        if(m_flags & asOBJ_POD)
            return *this;

        constructor<>("void f()");

        return *this;
    }

    value_class& copy_constructor(use_generic_t)
    {
        if(m_flags & asOBJ_POD)
            return *this;

        if constexpr(std::is_constructible_v<Class, const Class&>)
        {
            constructor<const Class&>(
                use_generic,
                string_concat("void f(const ", m_name, " &in)").c_str()
            );
        }
        else
        {
            constructor<Class&>(
                use_generic,
                string_concat("void f(", m_name, " &in)").c_str()
            );
        }

        return *this;
    }

    value_class& copy_constructor()
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

    value_class& destructor(use_generic_t)
    {
        if(m_flags & asOBJ_POD)
            return *this;

        behaviour(
            asBEHAVE_DESTRUCT,
            "void f()",
            +[](asIScriptGeneric* gen) -> void
            {
                reinterpret_cast<Class*>(gen->GetObject())->~Class();
            },
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    value_class& destructor()
    {
        if constexpr(ForceGeneric)
            destructor(use_generic);
        else
        {
            if(m_flags & asOBJ_POD)
                return *this;

            behaviour(
                asBEHAVE_DESTRUCT,
                "void f()",
                +[](Class* ptr) -> void
                {
                    ptr->~Class();
                },
                call_conv<asCALL_CDECL_OBJLAST>
            );
        }

        return *this;
    }

#define ASBIND20_VALUE_CLASS_OP(op_name)                   \
    value_class& op_name(use_generic_t)                    \
    {                                                      \
        this->template op_name##_impl_generic<Class>();    \
        return *this;                                      \
    }                                                      \
    value_class& op_name()                                 \
    {                                                      \
        if constexpr(ForceGeneric)                         \
            this->op_name(use_generic);                    \
        else                                               \
            this->template op_name##_impl_native<Class>(); \
        return *this;                                      \
    }

    ASBIND20_VALUE_CLASS_OP(opNeg);

    ASBIND20_VALUE_CLASS_OP(opPreInc);
    ASBIND20_VALUE_CLASS_OP(opPreDec);

    // Returning type by value in native calling convention
    // is NOT supported on all common platforms, e.g. x64 Linux.
    // Use generic calling convention as fallback.

    value_class& opPostInc(use_generic_t)
    {
        this->method_impl(
            string_concat(m_name, " opPostInc()").c_str(),
            +[](asIScriptGeneric* gen) -> void
            {
                set_generic_return<Class>(
                    gen,
                    get_generic_object<Class&>(gen)++
                );
            },
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    value_class& opPostInc()
    {
        opPostInc(use_generic);
        return *this;
    }

    value_class& opPostDec(use_generic_t)
    {
        this->method_impl(
            string_concat(m_name, " opPostDec()").c_str(),
            +[](asIScriptGeneric* gen) -> void
            {
                set_generic_return<Class>(
                    gen,
                    get_generic_object<Class&>(gen)--
                );
            },
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    value_class& opPostDec()
    {
        opPostDec(use_generic);

        return *this;
    }

    value_class& opAssign(use_generic_t) requires(std::is_copy_assignable_v<Class>)
    {
        if(m_flags & asOBJ_POD)
            return *this;

        this->template opAssign_impl_generic<Class>();

        return *this;
    }

    value_class& opAssign() requires(std::is_copy_assignable_v<Class>)
    {
        if constexpr(ForceGeneric)
            opAssign(use_generic);
        else
        {
            if(m_flags & asOBJ_POD)
                return *this;

            this->template opAssign_impl_native<Class>();
        }
        return *this;
    }

    ASBIND20_VALUE_CLASS_OP(opAddAssign);
    ASBIND20_VALUE_CLASS_OP(opSubAssign);
    ASBIND20_VALUE_CLASS_OP(opMulAssign);
    ASBIND20_VALUE_CLASS_OP(opDivAssign);

    ASBIND20_VALUE_CLASS_OP(opEquals);
    ASBIND20_VALUE_CLASS_OP(opCmp);

    ASBIND20_VALUE_CLASS_OP(opAdd);
    ASBIND20_VALUE_CLASS_OP(opSub);
    ASBIND20_VALUE_CLASS_OP(opMul);
    ASBIND20_VALUE_CLASS_OP(opDiv);

#undef ASBIND20_VALUE_CLASS_OP

    template <typename Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    value_class& behaviour(asEBehaviours beh, const char* decl, Fn&& fn, call_conv_t<asCALL_THISCALL>) requires(!ForceGeneric)
    {
        behaviour_impl(beh, decl, std::forward<Fn>(fn), call_conv<asCALL_THISCALL>);

        return *this;
    }

    template <typename R, typename... Args, asECallConvTypes CallConv>
    requires(CallConv != asCALL_GENERIC)
    value_class& behaviour(asEBehaviours beh, const char* decl, R (*fn)(Args...), call_conv_t<CallConv>) requires(!ForceGeneric)
    {
        this->behaviour_impl(beh, decl, fn, call_conv<CallConv>);

        return *this;
    }

    value_class& behaviour(asEBehaviours beh, const char* decl, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC>)
    {
        this->behaviour_impl(beh, decl, gfn, call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <native_function Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    value_class& method(const char* decl, Fn&& fn) requires(!ForceGeneric)
    {
        this->method_impl(decl, std::forward<Fn>(fn), call_conv<asCALL_THISCALL>);

        return *this;
    }

    template <native_function Fn, asECallConvTypes CallConv>
    requires(CallConv != asCALL_GENERIC)
    value_class& method(const char* decl, Fn&& fn, call_conv_t<CallConv>) requires(!ForceGeneric)
    {
        this->method_impl(decl, std::forward<Fn>(fn), call_conv<CallConv>);

        return *this;
    }

    value_class& method(const char* decl, asGENFUNC_t gfn)
    {
        this->method_impl(decl, gfn, call_conv<asCALL_GENERIC>);

        return *this;
    }

    value_class& method(const char* decl, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC>)
    {
        this->method_impl(decl, gfn, call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <
        native_function auto Method,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Method, Class>()>
    value_class& method(use_generic_t, const char* decl)
    {
        method(decl, generic_wrapper<Method, CallConv>(), call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <
        native_function auto Method,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Method, Class>()>
    value_class& method(const char* decl)
    {
        if constexpr(ForceGeneric)
        {
            method<Method, CallConv>(use_generic, decl);
        }
        else
        {
            method(decl, Method, call_conv<CallConv>);
        }

        return *this;
    }

    template <typename R, typename... Args>
    value_class& method(const char* decl, R (*fn)(Args...)) requires(!ForceGeneric)
    {
        this->template method_auto_callconv<Class>(decl, fn);

        return *this;
    }

    value_class& property(const char* decl, std::size_t off)
    {
        this->property_impl(decl, off);

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
class reference_class : public class_register_helper_base<ForceGeneric>
{
    using my_base = class_register_helper_base<ForceGeneric>;

    using my_base::m_engine;
    using my_base::m_name;

public:
    using class_type = Class;

    reference_class(asIScriptEngine* engine, const char* name, asQWORD flags = 0)
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

    reference_class& behaviour(asEBehaviours beh, const char* decl, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC>)
    {
        this->behaviour_impl(beh, decl, gfn, call_conv<asCALL_GENERIC>);

        return *this;
    }

    reference_class& behaviour(asEBehaviours beh, const char* decl, asGENFUNC_t gfn)
    {
        behaviour(beh, decl, gfn, call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <native_function Fn, asECallConvTypes CallConv>
    requires(CallConv != asCALL_GENERIC)
    reference_class& behaviour(asEBehaviours beh, const char* decl, Fn&& fn, call_conv_t<CallConv>) requires(!ForceGeneric)
    {
        this->behaviour_impl(beh, decl, std::forward<Fn>(fn), call_conv<CallConv>);

        return *this;
    }

    static constexpr char decl_template_callback[] = "bool f(int&in,bool&out)";

    reference_class& template_callback(asGENFUNC_t gfn) requires(Template)
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
    requires(!std::is_member_function_pointer_v<std::decay_t<Fn>>)
    reference_class& template_callback(Fn&& fn) requires(Template && !ForceGeneric)
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
    reference_class& template_callback(use_generic_t) requires(Template)
    {
        template_callback(
            +[](asIScriptGeneric* gen) -> void
            {
                set_generic_return<bool>(
                    gen,
                    Callback(get_generic_arg<asITypeInfo*>(gen, 0), get_generic_arg<bool&>(gen, 1))
                );
            }
        );

        return *this;
    }

    template <native_function auto Callback>
    reference_class& template_callback() requires(Template)
    {
        if constexpr(ForceGeneric)
            template_callback<Callback>(use_generic);
        else
            template_callback(Callback);

        return *this;
    }

    template <native_function Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    reference_class& method(const char* decl, Fn&& fn)
    {
        this->method_impl(decl, std::forward<Fn>(fn), call_conv<asCALL_THISCALL>);

        return *this;
    }

    template <typename Fn, asECallConvTypes CallConv>
    reference_class& method(const char* decl, Fn&& fn, call_conv_t<CallConv>)
    {
        this->method_impl(decl, std::forward<Fn>(fn), call_conv<CallConv>);

        return *this;
    }

    template <typename R, typename... Args>
    reference_class& method(const char* decl, R (*fn)(Args...))
    {
        this->template method_auto_callconv<Class>(decl, fn);

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Function, Class>()>
    reference_class& method(use_generic_t, const char* decl)
    {
        method(decl, generic_wrapper<Function, CallConv>(), call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Function, Class>()>
    reference_class& method(const char* decl)
    {
        if constexpr(ForceGeneric)
            this->template method<Function, CallConv>(use_generic, decl);
        else
            this->method_impl(decl, Function, call_conv<CallConv>);

        return *this;
    }

    template <typename... Args>
    reference_class& factory_function(const char* decl, Class* (*fn)(Args...)) requires(!ForceGeneric)
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

    reference_class& factory_function(const char* decl, asGENFUNC_t gfn)
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
    reference_class& factory_function(use_generic_t, const char* decl)
    {
        behaviour(
            asBEHAVE_FACTORY,
            decl,
            generic_wrapper<Function, asCALL_CDECL>()
        );

        return *this;
    }

    template <native_function auto Function>
    reference_class& factory_function(const char* decl)
    {
        if constexpr(ForceGeneric)
            factory_function<Function>(use_generic, decl);
        else
            factory_function(decl, Function);

        return *this;
    }

    reference_class& default_factory(use_generic_t)
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

    reference_class& default_factory()
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
    reference_class& factory(use_generic_t, const char* decl)
    {
        if constexpr(Template)
        {
            static_assert(sizeof...(Args) >= 1);

            factory_function(
                decl,
                +[](asIScriptGeneric* gen) -> void
                {
                    using args_tuple = std::tuple<Args...>;
                    [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                    {
                        Class* ptr = new Class(
                            get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is)...
                        );
                        gen->SetReturnAddress(ptr);
                    }(std::make_index_sequence<sizeof...(Args)>());
                }
            );
        }
        else
        {
            factory_function(
                decl,
                +[](asIScriptGeneric* gen) -> void
                {
                    using args_tuple = std::tuple<Args...>;

                    [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                    {
                        Class* ptr = new Class(get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is)...);
                        gen->SetReturnAddress(ptr);
                    }(std::make_index_sequence<sizeof...(Args)>());
                }
            );
        }

        return *this;
    }

    template <typename... Args>
    reference_class& factory(const char* decl)
    {
        if constexpr(ForceGeneric)
        {
            factory<Args...>(use_generic, decl);
        }
        else
        {
            factory_function(
                decl,
                +[](Args... args) -> Class*
                {
                    return new Class(args...);
                }
            );
        }

        return *this;
    }

    template <typename... Args>
    reference_class& list_factory_function(const char* decl, Class* (*fn)(Args...))
    {
        behaviour(
            asBEHAVE_LIST_FACTORY,
            decl,
            fn,
            call_conv<asCALL_CDECL>
        );

        return *this;
    }

    reference_class& list_factory_function(const char* decl, asGENFUNC_t gfn)
    {
        behaviour(
            asBEHAVE_LIST_FACTORY,
            decl,
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    reference_class& list_factory(std::string_view repeated_type_name) requires(Template)
    {
        std::string decl = string_concat(m_name, "@ f(int&in,int&in) {repeat ", repeated_type_name, "}");
        if constexpr(ForceGeneric)
        {
            list_factory_function(
                decl.c_str(),
                +[](asIScriptGeneric* gen) -> void
                {
                    Class* ptr = new Class(
                        get_generic_arg<asITypeInfo*>(gen, 0),
                        gen->GetArgAddress(1)
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

#define ASBIND20_REFERENCE_CLASS_OP(op_name)               \
    reference_class& op_name(use_generic_t)                \
    {                                                      \
        this->template op_name##_impl_generic<Class>();    \
        return *this;                                      \
    }                                                      \
    reference_class& op_name()                             \
    {                                                      \
        if constexpr(ForceGeneric)                         \
            this->op_name(use_generic);                    \
        else                                               \
            this->template op_name##_impl_native<Class>(); \
        return *this;                                      \
    }

    ASBIND20_REFERENCE_CLASS_OP(opAssign);

    ASBIND20_REFERENCE_CLASS_OP(opEquals);
    ASBIND20_REFERENCE_CLASS_OP(opCmp);

    ASBIND20_REFERENCE_CLASS_OP(opPreInc);
    ASBIND20_REFERENCE_CLASS_OP(opPreDec);

#undef ASBIND20_REFERENCE_CLASS_OP

    using addref_t = void (Class::*)();

    reference_class& addref(addref_t fn) requires(!ForceGeneric)
    {
        behaviour(
            asBEHAVE_ADDREF,
            "void f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    reference_class& addref(asGENFUNC_t gfn)
    {
        behaviour(
            asBEHAVE_ADDREF,
            "void f()",
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <addref_t AddRef>
    reference_class& addref(use_generic_t)
    {
        addref(generic_wrapper<AddRef, asCALL_THISCALL>());

        return *this;
    }

    template <native_function auto AddRef>
    reference_class& addref()
    {
        if constexpr(ForceGeneric)
            addref<AddRef>(use_generic);
        else
            addref(AddRef);

        return *this;
    }

    using release_t = void (Class::*)();

    reference_class& release(release_t fn) requires(!ForceGeneric)
    {
        behaviour(
            asBEHAVE_RELEASE,
            "void f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    reference_class& release(asGENFUNC_t gfn)
    {
        behaviour(
            asBEHAVE_RELEASE,
            "void f()",
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <release_t Release>
    reference_class& release(use_generic_t)
    {
        release(generic_wrapper<Release, asCALL_THISCALL>());

        return *this;
    }

    template <native_function auto Release>
    reference_class& release()
    {
        if constexpr(ForceGeneric)
            release<Release>(use_generic);
        else
            release(Release);

        return *this;
    }

    using get_refcount_t = int (Class::*)() const;

    reference_class& get_refcount(get_refcount_t fn) requires(!ForceGeneric)
    {
        behaviour(
            asBEHAVE_GETREFCOUNT,
            "int f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    reference_class& get_refcount(asGENFUNC_t gfn)
    {
        behaviour(
            asBEHAVE_GETREFCOUNT,
            "int f()",
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <get_refcount_t GetRefCount>
    reference_class& get_refcount(use_generic_t)
    {
        get_refcount(generic_wrapper<GetRefCount, asCALL_THISCALL>());

        return *this;
    }

    template <get_refcount_t GetRefCount>
    reference_class& get_refcount()
    {
        if constexpr(ForceGeneric)
            get_refcount<GetRefCount>(use_generic);
        else
            get_refcount(GetRefCount);

        return *this;
    }

    using set_gc_flag_t = void (Class::*)();

    reference_class& set_gc_flag(set_gc_flag_t fn) requires(!ForceGeneric)
    {
        behaviour(
            asBEHAVE_SETGCFLAG,
            "void f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    reference_class& set_gc_flag(asGENFUNC_t gfn)
    {
        behaviour(
            asBEHAVE_SETGCFLAG,
            "void f()",
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <set_gc_flag_t SetGCFlag>
    reference_class& set_gc_flag(use_generic_t)
    {
        set_gc_flag(generic_wrapper<SetGCFlag, asCALL_THISCALL>());

        return *this;
    }

    template <set_gc_flag_t SetGCFlag>
    reference_class& set_gc_flag()
    {
        if constexpr(ForceGeneric)
            set_gc_flag<SetGCFlag>(use_generic);
        else
            set_gc_flag(SetGCFlag);

        return *this;
    }

    using get_gc_flag_t = bool (Class::*)() const;

    reference_class& get_gc_flag(get_gc_flag_t fn) requires(!ForceGeneric)
    {
        behaviour(
            asBEHAVE_GETGCFLAG,
            "bool f()",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    reference_class& get_gc_flag(asGENFUNC_t gfn)
    {
        behaviour(
            asBEHAVE_GETGCFLAG,
            "bool f()",
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <get_gc_flag_t GetGCFlag>
    reference_class& get_gc_flag(use_generic_t)
    {
        get_gc_flag(generic_wrapper<GetGCFlag, asCALL_THISCALL>());

        return *this;
    }

    template <get_gc_flag_t GetGCFlag>
    reference_class& get_gc_flag()
    {
        if constexpr(ForceGeneric)
            get_gc_flag<GetGCFlag>(use_generic);
        else
            get_gc_flag(GetGCFlag);

        return *this;
    }

    using enum_refs_t = void (Class::*)(asIScriptEngine*);

    reference_class& enum_refs(enum_refs_t fn) requires(!ForceGeneric)
    {
        behaviour(
            asBEHAVE_ENUMREFS,
            "void f(int&in)",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    reference_class& enum_refs(asGENFUNC_t gfn)
    {
        behaviour(
            asBEHAVE_ENUMREFS,
            "void f(int&in)",
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <enum_refs_t EnumRefs>
    reference_class& enum_refs(use_generic_t)
    {
        enum_refs(
            +[](asIScriptGeneric* gen) -> void
            {
                Class* this_ = (Class*)gen->GetObject();
                asIScriptEngine* engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);

                std::invoke(EnumRefs, this_, engine);
            }
        );

        return *this;
    }

    template <enum_refs_t EnumRefs>
    reference_class& enum_refs()
    {
        if constexpr(ForceGeneric)
            enum_refs<EnumRefs>(use_generic);
        else
            enum_refs(EnumRefs);

        return *this;
    }

    using release_refs_t = void (Class::*)(asIScriptEngine*);

    reference_class& release_refs(release_refs_t fn) requires(!ForceGeneric)
    {
        behaviour(
            asBEHAVE_RELEASEREFS,
            "void f(int&in)",
            fn,
            call_conv<asCALL_THISCALL>
        );

        return *this;
    }

    reference_class& release_refs(asGENFUNC_t gfn)
    {
        behaviour(
            asBEHAVE_RELEASEREFS,
            "void f(int&in)",
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <release_refs_t ReleaseRefs>
    reference_class& release_refs(use_generic_t)
    {
        release_refs(
            +[](asIScriptGeneric* gen) -> void
            {
                Class* this_ = (Class*)gen->GetObject();
                asIScriptEngine* engine = *(asIScriptEngine**)gen->GetAddressOfArg(0);

                std::invoke(ReleaseRefs, this_, engine);
            }
        );

        return *this;
    }

    template <release_refs_t ReleaseRefs>
    reference_class& release_refs()
    {
        if constexpr(ForceGeneric)
            release_refs<ReleaseRefs>(use_generic);
        else
            release_refs(ReleaseRefs);

        return *this;
    }

    reference_class& property(const char* decl, std::size_t off)
    {
        this->property_impl(decl, off);

        return *this;
    }

    template <typename T>
    reference_class& property(const char* decl, T Class::*mp)
    {
        this->template property_impl<T, Class>(decl, mp);

        return *this;
    }

    reference_class& funcdef(std::string_view decl)
    {
        this->member_funcdef_impl(decl);

        return *this;
    }

private:
    asQWORD m_flags;
};

template <typename Class, bool UseGeneric = false>
using ref_class = reference_class<Class, false, UseGeneric>;

template <typename Class, bool ForceGeneric = false>
using template_class = reference_class<Class, true, ForceGeneric>;

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
