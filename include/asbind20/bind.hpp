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
#include "detail/include_as.hpp" // IWYU pragma: keep
#include "utility.hpp"
#include "generic.hpp"

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

    asIScriptEngine* get_engine() const noexcept
    {
        return m_engine;
    }

private:
    asIScriptEngine* m_engine;
    std::string m_prev;
};

struct use_generic_t
{};

constexpr inline use_generic_t use_generic{};

struct use_explicit_t
{};

constexpr inline use_explicit_t use_explicit{};

/**
 * @brief Wrappers for special functions like constructor
 *
 */
namespace wrappers
{
    template <typename Class, typename... Args>
    class constructor
    {
        template <asECallConvTypes CallConv>
        using native_function_type_helper = std::conditional<
            CallConv == asCALL_CDECL_OBJFIRST,
            void (*)(void*, Args...),
            void (*)(Args..., void*)>;

    public:
        static constexpr bool is_acceptable_native_call_conv(asECallConvTypes conv) noexcept
        {
            return conv == asCALL_CDECL_OBJFIRST || conv == asCALL_CDECL_OBJLAST;
        }

        static constexpr bool is_acceptable_call_conv(asECallConvTypes conv) noexcept
        {
            return conv == asCALL_GENERIC || is_acceptable_native_call_conv(conv);
        }

        template <asECallConvTypes CallConv>
        requires(is_acceptable_native_call_conv(CallConv))
        using native_function_type = typename native_function_type_helper<CallConv>::type;

        template <asECallConvTypes CallConv>
        requires(is_acceptable_native_call_conv(CallConv) || CallConv == asCALL_GENERIC)
        using wrapper_type = std::conditional_t<
            CallConv == asCALL_GENERIC,
            asGENFUNC_t,
            typename native_function_type_helper<CallConv>::type>;

        template <asECallConvTypes CallConv>
        static auto generate() noexcept -> wrapper_type<CallConv>
        {
            if constexpr(CallConv == asCALL_GENERIC)
            {
                return +[](asIScriptGeneric* gen) -> void
                {
                    using args_tuple = std::tuple<Args...>;
                    [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                    {
                        void* mem = gen->GetObject();
                        new(mem) Class(get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is)...);
                    }(std::index_sequence_for<Args...>());
                };
            }
            else if constexpr(CallConv == asCALL_CDECL_OBJFIRST)
            {
                return +[](void* mem, Args... args) -> void
                {
                    new(mem) Class(std::forward<Args>(args)...);
                };
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                return +[](Args... args, void* mem) -> void
                {
                    new(mem) Class(std::forward<Args>(args)...);
                };
            }
        }

        asGENFUNC_t operator()(use_generic_t) const
        {
            return this->template generate<asCALL_GENERIC>();
        }

        template <asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        auto operator()(call_conv_t<CallConv>) const -> wrapper_type<CallConv>
        {
            return this->template generate<CallConv>();
        }
    };

    template <
        native_function auto ConstructorFunc,
        typename Class,
        asECallConvTypes OriginalCallConv>
    requires(OriginalCallConv == asCALL_CDECL_OBJFIRST || OriginalCallConv == asCALL_CDECL_OBJLAST)
    class constructor_function
    {
    public:
        static constexpr bool is_acceptable_native_call_conv(asECallConvTypes conv) noexcept
        {
            return conv == OriginalCallConv;
        }

        static constexpr bool is_acceptable_call_conv(asECallConvTypes conv) noexcept
        {
            return conv == asCALL_GENERIC || is_acceptable_native_call_conv(conv);
        }

        using native_function_type = std::decay_t<decltype(ConstructorFunc)>;

        template <asECallConvTypes CallConv>
        using wrapper_type = std::conditional_t<
            CallConv == asCALL_GENERIC,
            asGENFUNC_t,
            native_function_type>;

        template <asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        static auto generate() noexcept
        {
            if constexpr(CallConv == asCALL_GENERIC)
            {
                using traits = function_traits<decltype(ConstructorFunc)>;
                using args_tuple = typename traits::args_tuple;

                if constexpr(OriginalCallConv == asCALL_CDECL_OBJFIRST)
                {
                    return +[](asIScriptGeneric* gen) -> void
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc,
                                mem,
                                get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(gen, (asUINT)Is + 1)...
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 1>());
                    };
                }
                else // OriginalCallConv == asCALL_CDECL_OBJLAST
                {
                    return +[](asIScriptGeneric* gen) -> void
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc,
                                get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is)...,
                                mem
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 1>());
                    };
                }
            }
            else // CallConv == OriginalCallConv
            {
                return ConstructorFunc;
            }
        }

        asGENFUNC_t operator()(use_generic_t) const
        {
            return this->template generate<asCALL_GENERIC>();
        }

        template <asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        auto operator()(call_conv_t<CallConv>) const -> wrapper_type<CallConv>
        {
            return this->template generate<CallConv>();
        }
    };

    template <typename Class, typename ListElementType = void>
    class list_constructor
    {
        template <asECallConvTypes CallConv>
        using native_function_type_helper = std::conditional<
            CallConv == asCALL_CDECL_OBJFIRST,
            void (*)(void*, ListElementType*),
            void (*)(ListElementType*, void*)>;

    public:
        static constexpr bool is_acceptable_native_call_conv(asECallConvTypes conv) noexcept
        {
            return conv == asCALL_CDECL_OBJFIRST || conv == asCALL_CDECL_OBJLAST;
        }

        static constexpr bool is_acceptable_call_conv(asECallConvTypes conv) noexcept
        {
            return conv == asCALL_GENERIC || is_acceptable_native_call_conv(conv);
        }

        template <asECallConvTypes CallConv>
        requires(is_acceptable_native_call_conv(CallConv))
        using native_function_type = typename native_function_type_helper<CallConv>::type;

        template <asECallConvTypes CallConv>
        requires(is_acceptable_native_call_conv(CallConv) || CallConv == asCALL_GENERIC)
        using wrapper_type = std::conditional_t<
            CallConv == asCALL_GENERIC,
            asGENFUNC_t,
            typename native_function_type_helper<CallConv>::type>;

        template <asECallConvTypes CallConv>
        static auto generate() noexcept -> wrapper_type<CallConv>
        {
            if constexpr(CallConv == asCALL_GENERIC)
            {
                return +[](asIScriptGeneric* gen) -> void
                {
                    void* mem = gen->GetObject();
                    new(mem) Class(*(ListElementType**)gen->GetAddressOfArg(0));
                };
            }
            else if constexpr(CallConv == asCALL_CDECL_OBJFIRST)
            {
                return +[](void* mem, ListElementType* list_buf) -> void
                {
                    new(mem) Class(list_buf);
                };
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                return +[](ListElementType* list_buf, void* mem) -> void
                {
                    new(mem) Class(list_buf);
                };
            }
        }

        asGENFUNC_t operator()(use_generic_t) const
        {
            return this->template generate<asCALL_GENERIC>();
        }

        template <asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        auto operator()(call_conv_t<CallConv>) const -> wrapper_type<CallConv>
        {
            return this->template generate<CallConv>();
        }
    };

    template <typename Class, bool Template, typename... Args>
    class factory
    {
    public:
        static constexpr bool is_acceptable_native_call_conv(asECallConvTypes conv) noexcept
        {
            return conv == asCALL_CDECL;
        }

        static constexpr bool is_acceptable_call_conv(asECallConvTypes conv) noexcept
        {
            return conv == asCALL_GENERIC || is_acceptable_native_call_conv(conv);
        }

        using native_function_type = std::conditional_t<
            Template,
            Class* (*)(asITypeInfo*, Args...),
            Class* (*)(Args...)>;

        template <asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        using wrapper_type = std::conditional_t<
            CallConv == asCALL_GENERIC,
            asGENFUNC_t,
            native_function_type>;

        template <asECallConvTypes CallConv>
        static auto generate() noexcept -> wrapper_type<CallConv>
        {
            if constexpr(CallConv == asCALL_GENERIC)
            {
                if constexpr(Template)
                {
                    return +[](asIScriptGeneric* gen) -> void
                    {
                        using args_tuple = std::tuple<Args...>;

                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* ptr = new Class(
                                *(asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is + 1)...
                            );
                            gen->SetReturnAddress(ptr);
                        }(std::index_sequence_for<Args...>());
                    };
                }
                else
                {
                    return +[](asIScriptGeneric* gen) -> void
                    {
                        using args_tuple = std::tuple<Args...>;

                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* ptr = new Class(get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is)...);
                            gen->SetReturnAddress(ptr);
                        }(std::index_sequence_for<Args...>());
                    };
                }
            }
            else // CallConv == asCALL_CDECL
            {
                if constexpr(Template)
                {
                    return +[](asITypeInfo* ti, Args... args) -> Class*
                    {
                        return new Class(ti, std::forward<Args>(args)...);
                    };
                }
                else
                {
                    return +[](Args... args) -> Class*
                    {
                        return new Class(std::forward<Args>(args)...);
                    };
                }
            }
        }

        asGENFUNC_t operator()(use_generic_t) const
        {
            return this->template generate<asCALL_GENERIC>();
        }

        template <asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        auto operator()(call_conv_t<CallConv>) const -> wrapper_type<CallConv>
        {
            return this->template generate<CallConv>();
        }
    };

    template <typename Class, bool Template, typename ListElementType = void>
    class list_factory
    {
    public:
        static constexpr bool is_acceptable_native_call_conv(asECallConvTypes conv) noexcept
        {
            return conv == asCALL_CDECL;
        }

        static constexpr bool is_acceptable_call_conv(asECallConvTypes conv) noexcept
        {
            return conv == asCALL_GENERIC || is_acceptable_native_call_conv(conv);
        }

        using native_function_type = std::conditional_t<
            Template,
            Class* (*)(asITypeInfo*, ListElementType*),
            Class* (*)(ListElementType*)>;

        template <asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        using wrapper_type = std::conditional_t<
            CallConv == asCALL_GENERIC,
            asGENFUNC_t,
            native_function_type>;

        template <asECallConvTypes CallConv>
        static auto generate() noexcept -> wrapper_type<CallConv>
        {
            if constexpr(CallConv == asCALL_GENERIC)
            {
                if constexpr(Template)
                {
                    return +[](asIScriptGeneric* gen) -> void
                    {
                        Class* ptr = new Class(
                            *(asITypeInfo**)gen->GetAddressOfArg(0),
                            *(ListElementType**)gen->GetAddressOfArg(1)
                        );
                        gen->SetReturnAddress(ptr);
                    };
                }
                else
                {
                    return +[](asIScriptGeneric* gen) -> void
                    {
                        Class* ptr = new Class(
                            *(ListElementType**)gen->GetAddressOfArg(0)
                        );
                        gen->SetReturnAddress(ptr);
                    };
                }
            }
            else // CallConv == asCALL_CDECL
            {
                if constexpr(Template)
                {
                    return +[](asITypeInfo* ti, ListElementType* list_buf) -> Class*
                    {
                        return new Class(ti, list_buf);
                    };
                }
                else
                {
                    return +[](ListElementType* list_buf) -> Class*
                    {
                        return new Class(list_buf);
                    };
                }
            }
        }

        asGENFUNC_t operator()(use_generic_t) const
        {
            return this->template generate<asCALL_GENERIC>();
        }

        template <asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        auto operator()(call_conv_t<CallConv>) const -> wrapper_type<CallConv>
        {
            return this->template generate<CallConv>();
        }
    };

    template <typename Class, typename To>
    class opConv
    {
    public:
        static constexpr bool is_acceptable_native_call_conv(asECallConvTypes conv) noexcept
        {
            return conv == asCALL_CDECL_OBJFIRST || conv == asCALL_CDECL_OBJLAST;
        }

        static constexpr bool is_acceptable_call_conv(asECallConvTypes conv) noexcept
        {
            return conv == asCALL_GENERIC || is_acceptable_native_call_conv(conv);
        }

        using native_function_type = To (*)(Class&);

        template <asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        using wrapper_type = std::conditional_t<
            CallConv == asCALL_GENERIC,
            asGENFUNC_t,
            native_function_type>;

        template <asECallConvTypes CallConv>
        static auto generate() noexcept -> wrapper_type<CallConv>
        {
            if constexpr(CallConv == asCALL_GENERIC)
            {
                return +[](asIScriptGeneric* gen) -> void
                {
                    set_generic_return<To>(
                        gen,
                        static_cast<To>(get_generic_object<Class&>(gen))
                    );
                };
            }
            else // CallConv == asCALL_CDECL_FISRT/LAST
            {
                return +[](Class& obj) -> To
                {
                    return static_cast<To>(obj);
                };
            }
        }

        asGENFUNC_t operator()(use_generic_t) const
        {
            return this->template generate<asCALL_GENERIC>();
        }

        template <asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        auto operator()(call_conv_t<CallConv>) const -> wrapper_type<CallConv>
        {
            return this->template generate<CallConv>();
        }
    };
} // namespace wrappers

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
        std::same_as<T, Class*> ||
        std::same_as<T, const Class*> ||
        std::same_as<T, Class&> ||
        std::same_as<T, const Class&>;

    template <typename Class, typename... Args>
    static consteval asECallConvTypes deduce_method_callconv() noexcept
    {
        using args_t = std::tuple<Args...>;
        constexpr std::size_t arg_count = sizeof...(Args);
        using first_arg_t = std::tuple_element_t<0, args_t>;
        using last_arg_t = std::tuple_element_t<sizeof...(Args) - 1, args_t>;

        if constexpr(arg_count == 1 && std::same_as<first_arg_t, asIScriptGeneric*>)
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

    template <noncapturing_lambda Lambda, typename Class>
    consteval asECallConvTypes deduce_lambda_callconv()
    {
        using function_type = decltype(+std::declval<const Lambda>());
        using traits = function_traits<function_type>;

        return []<std::size_t... Is>(std::index_sequence<Is...>)
        {
            return deduce_method_callconv<Class, typename traits::template arg_type<Is>...>();
        }(std::make_index_sequence<traits::arg_count_v>());
    }
} // namespace detail

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
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterGlobalFunction(
            decl,
            to_asSFuncPtr(fn),
            asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = asCALL_CDECL>
    [[deprecated("Use the version with fp<>")]]
    global& function(use_generic_t, const char* decl)
    {
        function(
            decl,
            to_asGENFUNC_t(fp<Function>, call_conv<CallConv>)
        );

        return *this;
    }

    template <
        auto Function,
        asECallConvTypes CallConv = asCALL_CDECL>
    requires(CallConv == asCALL_CDECL || CallConv == asCALL_STDCALL)
    global& function(
        use_generic_t,
        const char* decl,
        fp_wrapper_t<Function>,
        call_conv_t<CallConv> = {}
    )
    {
        this->function(
            decl,
            to_asGENFUNC_t(fp<Function>, call_conv<CallConv>)
        );

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = asCALL_CDECL>
    requires(!std::is_member_function_pointer_v<std::decay_t<decltype(Function)>>)
    [[deprecated("Use the version with fp<>")]]
    global& function(const char* decl)
    {
        if constexpr(ForceGeneric)
        {
            function<Function, CallConv>(use_generic, decl);
        }
        else
        {
            [[maybe_unused]]
            int r = 0;
            r = m_engine->RegisterGlobalFunction(
                decl,
                to_asSFuncPtr(Function),
                CallConv
            );
            assert(r >= 0);
        }

        return *this;
    }

    template <
        auto Function,
        asECallConvTypes CallConv = asCALL_CDECL>
    requires(CallConv == asCALL_CDECL || CallConv == asCALL_STDCALL)
    global& function(
        const char* decl,
        fp_wrapper_t<Function>,
        call_conv_t<CallConv> = {}
    )
    {
        if constexpr(ForceGeneric)
        {
            function(use_generic, decl, fp<Function>, call_conv<CallConv>);
        }
        else
        {
            [[maybe_unused]]
            int r = 0;
            r = m_engine->RegisterGlobalFunction(
                decl,
                to_asSFuncPtr(Function),
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
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterGlobalFunction(
            decl,
            to_asSFuncPtr(gfn),
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
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterGlobalFunction(
            decl,
            to_asSFuncPtr(fn),
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
    [[deprecated("Use the version with fp<>")]]
    global& function(use_generic_t, const char* decl, T& instance)
    {
        function(
            decl,
            to_asGENFUNC_t(fp<Function>, call_conv<asCALL_THISCALL_ASGLOBAL>),
            instance
        );

        return *this;
    }

    template <
        native_function auto Function,
        typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<decltype(Function)>>)
    [[deprecated("Use the version with fp<>")]]
    global& function(const char* decl, T& instance)
    {
        if constexpr(ForceGeneric)
            function<Function>(use_generic, decl, instance);
        else
            function(decl, Function, instance);

        return *this;
    }

    template <
        auto Function,
        typename T>
    global& function(
        use_generic_t,
        const char* decl,
        fp_wrapper_t<Function>,
        T& instance
    )
    {
        static_assert(
            std::is_member_function_pointer_v<std::decay_t<decltype(Function)>>,
            "Function for asCALL_THISCALL_ASGLOBAL must be a member function"
        );

        function(
            decl,
            to_asGENFUNC_t(fp<Function>, call_conv<asCALL_THISCALL_ASGLOBAL>),
            instance
        );

        return *this;
    }

    template <
        auto Function,
        typename T>
    global& function(
        const char* decl,
        fp_wrapper_t<Function>,
        T& instance
    )
    {
        static_assert(
            std::is_member_function_pointer_v<std::decay_t<decltype(Function)>>,
            "Function for asCALL_THISCALL_ASGLOBAL must be a member function"
        );

        if constexpr(ForceGeneric)
            function(use_generic, decl, fp<Function>, instance);
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
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterGlobalProperty(
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
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterGlobalFunction(
            decl,
            to_asSFuncPtr(gfn),
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
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterFuncdef(decl);
        assert(r >= 0);

        return *this;
    }

    global& typedef_(
        const char* type_decl,
        const char* new_name
    )
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterTypedef(new_name, type_decl);
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

    global& enum_type(
        const char* type
    )
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterEnum(type);
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

        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterEnumValue(
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
        [[maybe_unused]]
        int r = 0;
        r = m_engine->SetMessageCallback(
            to_asSFuncPtr(fn), obj, asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    template <native_function Callback, typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& message_callback(Callback fn, T& obj)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->SetMessageCallback(
            to_asSFuncPtr(fn), (void*)std::addressof(obj), asCALL_THISCALL
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
        [[maybe_unused]]
        int r = 0;
        r = m_engine->SetTranslateAppExceptionCallback(
            to_asSFuncPtr(fn), obj, asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    template <native_function Callback, typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& exception_translator(Callback fn, T& obj)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->SetTranslateAppExceptionCallback(
            to_asSFuncPtr(fn), (void*)std::addressof(obj), asCALL_THISCALL
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

global(const script_engine&) -> global<false>;

namespace detail
{
    inline std::string generate_member_funcdef(
        const char* type, std::string_view funcdef
    )
    {
        // Firstly, find the begin of parameters
        auto param_being = std::find(funcdef.rbegin(), funcdef.rend(), '(');
        if(param_being != funcdef.rend())
        {
            ++param_being; // Skip '('

            // Skip possible whitespaces between the function name and the parameters
            param_being = std::find_if_not(
                param_being,
                funcdef.rend(),
                [](char ch) -> bool
                { return ch == ' '; }
            );
        }
        auto name_begin = std::find_if_not(
            param_being,
            funcdef.rend(),
            [](char ch) -> bool
            {
                return ('0' <= ch && ch <= '9') ||
                       ('a' <= ch && ch <= 'z') ||
                       ('A' <= ch && ch <= 'Z') ||
                       ch == '_' ||
                       static_cast<std::uint8_t>(ch) > 127;
            }
        );

        using namespace std::literals;

        std::string_view return_type(funcdef.begin(), name_begin.base());
        if(return_type.back() == ' ')
            return_type.remove_suffix(1);
        return string_concat(
            return_type,
            ' ',
            type,
            "::"sv,
            std::string_view(name_begin.base(), funcdef.end())
        );
    }
} // namespace detail

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

    template <typename Class>
    void register_object_type(asQWORD flags)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterObjectType(
            m_name,
            static_cast<int>(sizeof(Class)),
            flags
        );
        assert(r >= 0);
    }

    template <typename Fn, asECallConvTypes CallConv>
    void method_impl(const char* decl, Fn&& fn, call_conv_t<CallConv>)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterObjectMethod(
            m_name,
            decl,
            to_asSFuncPtr(fn),
            CallConv
        );
        assert(r >= 0);
    }

    void method_impl(const char* decl, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC>)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterObjectMethod(
            m_name,
            decl,
            to_asSFuncPtr(gfn),
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
                to_asGENFUNC_t(fp<Method>, call_conv<CallConv>),
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
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterObjectBehaviour(
            m_name,
            beh,
            decl,
            to_asSFuncPtr(gfn),
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
        r = m_engine->RegisterObjectBehaviour(
            m_name,
            beh,
            decl,
            to_asSFuncPtr(fn),
            CallConv
        );
        assert(r >= 0);
    }

    template <native_function auto Behaviour, asECallConvTypes CallConv>
    void wrapped_behaviour_impl(asEBehaviours beh, const char* decl)
    {
        [[maybe_unused]]
        int r = 0;
        if constexpr(ForceGeneric)
        {
            r = m_engine->RegisterObjectBehaviour(
                m_name,
                beh,
                decl,
                to_asSFuncPtr(to_asGENFUNC_t(fp<Behaviour>, call_conv<CallConv>)),
                asCALL_GENERIC
            );
        }
        else
        {
            r = m_engine->RegisterObjectBehaviour(
                m_name,
                beh,
                decl,
                to_asSFuncPtr(Behaviour),
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
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterObjectProperty(
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
                    gen,                                                                           \
                    cpp_op get_generic_object<this_arg_t>(gen)                                     \
                );                                                                                 \
            },                                                                                     \
            call_conv<asCALL_GENERIC>                                                              \
        );                                                                                         \
    }                                                                                              \
    template <typename Class>                                                                      \
    void as_op_sig##_impl_native()                                                                 \
    {                                                                                              \
        static constexpr bool has_member_func = requires() {                                       \
            static_cast<return_type (Class::*)() const_>(&Class::operator cpp_op);                 \
        };                                                                                         \
        if constexpr(has_member_func)                                                              \
        {                                                                                          \
            method_impl(                                                                           \
                as_op_sig##_decl().c_str(),                                                        \
                static_cast<return_type (Class::*)() const_>(&Class::operator cpp_op),             \
                call_conv<asCALL_THISCALL>                                                         \
            );                                                                                     \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            using this_arg_t = std::conditional_t<(#const_[0] != '\0'), const Class&, Class&>;     \
            this->method_impl(                                                                     \
                as_op_sig##_decl().c_str(),                                                        \
                +[](this_arg_t this_) -> return_type                                               \
                {                                                                                  \
                    return cpp_op this_;                                                           \
                },                                                                                 \
                call_conv<asCALL_CDECL_OBJFIRST>                                                   \
            );                                                                                     \
        }                                                                                          \
    }

    ASBIND20_CLASS_UNARY_PREFIX_OP(
        opNeg, -, (m_name, " opNeg() const"), Class, const
    )

    ASBIND20_CLASS_UNARY_PREFIX_OP(
        opPreInc, ++, (m_name, "& opPreInc()"), Class&,
    )
    ASBIND20_CLASS_UNARY_PREFIX_OP(
        opPreDec, --, (m_name, "& opPreDec()"), Class&,
    )

#undef ASBIND20_CLASS_REGISTER_UNARY_PREFIX_OP

    // Only for operator++/--(int)
#define ASBIND20_CLASS_UNARY_SUFFIX_OP(as_op_sig, cpp_op)                   \
    std::string as_op_sig##_decl() const                                    \
    {                                                                       \
        return string_concat(m_name, " " #as_op_sig "()");                  \
    }                                                                       \
    template <typename Class>                                               \
    void as_op_sig##_impl_generic()                                         \
    {                                                                       \
        method_impl(                                                        \
            as_op_sig##_decl().c_str(),                                     \
            +[](asIScriptGeneric* gen) -> void                              \
            {                                                               \
                set_generic_return<Class>(                                  \
                    gen,                                                    \
                    get_generic_object<Class&>(gen) cpp_op                  \
                );                                                          \
            },                                                              \
            call_conv<asCALL_GENERIC>                                       \
        );                                                                  \
    }                                                                       \
    template <typename Class>                                               \
    void as_op_sig##_impl_native()                                          \
    {                                                                       \
        /* Use a wrapper to deal with the hidden int of postfix operator */ \
        method_impl(                                                        \
            as_op_sig##_decl().c_str(),                                     \
            +[](Class& this_) -> Class                                      \
            { return this_ cpp_op; },                                       \
            call_conv<asCALL_CDECL_OBJLAST>                                 \
        );                                                                  \
    }

    ASBIND20_CLASS_UNARY_SUFFIX_OP(opPostInc, ++)
    ASBIND20_CLASS_UNARY_SUFFIX_OP(opPostDec, --)

#undef ASBIND20_CLASS_UNARY_SUFFIX_OP

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
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opAddAssign, +=, (m_name, "& opAddAssign(const ", m_name, " &in)"), Class&, , const Class&
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opSubAssign, -=, (m_name, "& opSubAssign(const ", m_name, " &in)"), Class&, , const Class&
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opMulAssign, *=, (m_name, "& opMulAssign(const ", m_name, " &in)"), Class&, , const Class&
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opDivAssign, /=, (m_name, "& opDivAssign(const ", m_name, " &in)"), Class&, , const Class&
    )

    // Comparison operators

    ASBIND20_CLASS_BINARY_OP_IMPL(
        opEquals, ==, ("bool opEquals(const ", m_name, " &in) const"), bool, const, const Class&
    )

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
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opSub, -, (m_name, " opSub(const ", m_name, " &in) const"), Class, const, const Class&
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opMul, *, (m_name, " opMul(const ", m_name, " &in) const"), Class, const, const Class&
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opDiv, /, (m_name, " opDiv(const ", m_name, " &in) const"), Class, const, const Class&
    )

#undef ASBIND20_CLASS_BINARY_OP_GENERIC
#undef ASBIND20_CLASS_BINARY_OP_NATIVE
#undef ASBIND20_CLASS_BINARY_OP_IMPL

    void member_funcdef_impl(std::string_view decl)
    {
        full_funcdef(
            detail::generate_member_funcdef(m_name, decl).c_str()
        );
    }

    static std::string decl_opConv(std::string_view ret, bool implicit)
    {
        if(implicit)
            return string_concat(ret, " opImplConv() const");
        else
            return string_concat(ret, " opConv() const");
    }

    template <typename Class, typename To>
    void opConv_impl_native(std::string_view ret, bool implicit)
    {
        auto wrapper =
            wrappers::opConv<Class, To>{}(call_conv<asCALL_CDECL_OBJLAST>);

        method_impl(
            decl_opConv(ret, implicit).c_str(),
            wrapper,
            call_conv<asCALL_CDECL_OBJLAST>
        );
    }

    template <typename Class, typename To>
    void opConv_impl_generic(std::string_view ret, bool implicit)
    {
        auto wrapper =
            wrappers::opConv<Class, To>{}(use_generic);

        method_impl(
            decl_opConv(ret, implicit).c_str(),
            wrapper,
            call_conv<asCALL_GENERIC>
        );
    }

private:
    void full_funcdef(const char* decl)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterFuncdef(decl);
        assert(r >= 0);
    }
};

template <typename Class, bool ForceGeneric = false>
class value_class final : public class_register_helper_base<ForceGeneric>
{
    using my_base = class_register_helper_base<ForceGeneric>;

    using my_base::m_engine;
    using my_base::m_name;

public:
    using class_type = Class;

    value_class(asIScriptEngine* engine, const char* name, asQWORD flags = 0)
        : my_base(engine, name), m_flags(asOBJ_VALUE | flags | AS_NAMESPACE_QUALIFIER asGetTypeTraits<Class>())
    {
        assert(!(m_flags & asOBJ_REF));

        this->template register_object_type<Class>(m_flags);
    }

private:
    std::string decl_constructor_impl(std::string_view params, bool explicit_) const
    {
        if(explicit_)
        {
            return params.empty() ?
                       "void f()explicit" :
                       string_concat("void f(", params, ")explicit");
        }
        else
        {
            return params.empty() ?
                       "void f()" :
                       string_concat("void f(", params, ')');
        }
    }

    value_class& constructor_function_impl(std::string_view params, asGENFUNC_t gfn, bool explicit_, call_conv_t<asCALL_GENERIC> = {})
    {
        this->behaviour_impl(
            asBEHAVE_CONSTRUCT,
            decl_constructor_impl(params, explicit_).c_str(),
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

public:
    value_class& constructor_function(std::string_view params, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC> = {})
    {
        this->behaviour_impl(
            asBEHAVE_CONSTRUCT,
            decl_constructor_impl(params, false).c_str(),
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    value_class& constructor_function(std::string_view params, use_explicit_t, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC> = {})
    {
        this->behaviour_impl(
            asBEHAVE_CONSTRUCT,
            decl_constructor_impl(params, true).c_str(),
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <typename... Args, asECallConvTypes CallConv>
    requires(CallConv != asCALL_GENERIC)
    value_class& constructor_function(std::string_view params, void (*fn)(Args...), call_conv_t<CallConv>) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            asBEHAVE_CONSTRUCT,
            decl_constructor_impl(params, false).c_str(),
            fn,
            call_conv<CallConv>
        );

        return *this;
    }

    template <typename... Args, asECallConvTypes CallConv>
    requires(CallConv != asCALL_GENERIC)
    value_class& constructor_function(
        std::string_view params,
        use_explicit_t,
        void (*fn)(Args...),
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            asBEHAVE_CONSTRUCT,
            decl_constructor_impl(params, true).c_str(),
            fn,
            call_conv<CallConv>
        );

        return *this;
    }

private:
    template <native_function auto Function, asECallConvTypes CallConv>
    value_class& constructor_function_wrapper_impl(
        use_generic_t, std::string_view params, bool explicit_
    )
    {
        asGENFUNC_t wrapper =
            wrappers::constructor_function<Function, Class, CallConv>{}(use_generic);

        if(explicit_)
        {
            constructor_function(
                params,
                use_explicit,
                wrapper,
                call_conv<asCALL_GENERIC>
            );
        }
        else
        {
            constructor_function(
                params,
                wrapper,
                call_conv<asCALL_GENERIC>
            );
        }

        return *this;
    }

public:
    template <native_function auto Function, asECallConvTypes CallConv>
    requires(CallConv == asCALL_CDECL_OBJFIRST || CallConv == asCALL_CDECL_OBJLAST)
    value_class& constructor_function(use_generic_t, std::string_view params)
    {
        this->constructor_function_wrapper_impl<Function, CallConv>(use_generic, params, false);

        return *this;
    }

    template <native_function auto Function, asECallConvTypes CallConv>
    requires(CallConv == asCALL_CDECL_OBJFIRST || CallConv == asCALL_CDECL_OBJLAST)
    value_class& constructor_function(use_generic_t, std::string_view params, use_explicit_t)
    {
        this->constructor_function_wrapper_impl<Function, CallConv>(use_generic, params, true);

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Function, Class>()>
    requires(CallConv != asCALL_GENERIC)
    value_class& constructor_function(std::string_view params)
    {
        if constexpr(ForceGeneric)
        {
            constructor_function<Function, CallConv>(use_generic, params);
        }
        else
        {
            constructor_function(params, Function, call_conv<CallConv>);
        }

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Function, Class>()>
    requires(CallConv != asCALL_GENERIC)
    value_class& constructor_function(std::string_view params, use_explicit_t)
    {
        if constexpr(ForceGeneric)
        {
            constructor_function<Function, CallConv>(use_generic, params, use_explicit);
        }
        else
        {
            constructor_function(params, use_explicit, Function, call_conv<CallConv>);
        }

        return *this;
    }

private:
    template <typename... Args>
    void constructor_impl_generic(std::string_view params, bool explicit_)
    {
        wrappers::constructor<Class, Args...> wrapper;
        if(explicit_)
        {
            constructor_function(
                params,
                use_explicit,
                wrapper(use_generic),
                call_conv<asCALL_GENERIC>
            );
        }
        else
        {
            constructor_function(
                params,
                wrapper(use_generic),
                call_conv<asCALL_GENERIC>
            );
        }
    }

    template <typename... Args>
    void constructor_impl_native(std::string_view params, bool explicit_)
    {
        wrappers::constructor<Class, Args...> wrapper;
        if(explicit_)
        {
            constructor_function(
                params,
                use_explicit,
                wrapper(call_conv<asCALL_CDECL_OBJLAST>),
                call_conv<asCALL_CDECL_OBJLAST>
            );
        }
        else
        {
            constructor_function(
                params,
                wrapper(call_conv<asCALL_CDECL_OBJLAST>),
                call_conv<asCALL_CDECL_OBJLAST>
            );
        }
    }

public:
    template <typename... Args>
    value_class& constructor(
        use_generic_t,
        std::string_view params
    ) requires(is_only_constructible_v<Class, Args...>)
    {
        constructor_impl_generic<Args...>(params, false);

        return *this;
    }

    template <typename... Args>
    value_class& constructor(
        use_generic_t,
        std::string_view params,
        use_explicit_t
    ) requires(is_only_constructible_v<Class, Args...>)
    {
        constructor_impl_generic<Args...>(params, true);

        return *this;
    }

    template <typename... Args>
    value_class& constructor(
        std::string_view params
    ) requires(is_only_constructible_v<Class, Args...>)
    {
        if constexpr(ForceGeneric)
            constructor<Args...>(use_generic, params);
        else
            constructor_impl_native<Args...>(params, false);

        return *this;
    }

    template <typename... Args>
    value_class& constructor(
        std::string_view params,
        use_explicit_t
    ) requires(is_only_constructible_v<Class, Args...>)
    {
        if constexpr(ForceGeneric)
            constructor<Args...>(use_generic, params, use_explicit);
        else
            constructor_impl_native<Args...>(params, true);

        return *this;
    }

    value_class& default_constructor(use_generic_t)
    {
        constructor<>(use_generic, "");

        return *this;
    }

    value_class& default_constructor()
    {
        constructor<>("");

        return *this;
    }

    value_class& copy_constructor(use_generic_t)
    {
        constructor<const Class&>(
            use_generic,
            string_concat("const ", m_name, " &in")
        );

        return *this;
    }

    value_class& copy_constructor()
    {
        constructor<const Class&>(
            string_concat("const ", m_name, "&in")
        );

        return *this;
    }

    value_class& destructor(use_generic_t)
    {
        behaviour(
            asBEHAVE_DESTRUCT,
            "void f()",
            +[](asIScriptGeneric* gen) -> void
            {
                std::destroy_at(get_generic_object<Class*>(gen));
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
            behaviour(
                asBEHAVE_DESTRUCT,
                "void f()",
                +[](Class* ptr) -> void
                {
                    std::destroy_at(ptr);
                },
                call_conv<asCALL_CDECL_OBJLAST>
            );
        }

        return *this;
    }

    /**
     * @brief Automatically register functions based on traits using generic wrapper.
     *
     * @param traits Type traits
     */
    value_class& behaviours_by_traits(
        use_generic_t,
        asQWORD traits = AS_NAMESPACE_QUALIFIER asGetTypeTraits<Class>()
    )
    {
        if(traits & asOBJ_APP_CLASS_C)
        {
            if constexpr(is_only_constructible_v<Class>)
                default_constructor(use_generic);
            else
                assert(false && "missing default constructor");
        }
        if(traits & asOBJ_APP_CLASS_D)
        {
            if constexpr(std::is_destructible_v<Class>)
                destructor(use_generic);
            else
                assert(false && "missing destructor");
        }
        if(traits & asOBJ_APP_CLASS_A)
        {
            if constexpr(std::is_copy_assignable_v<Class>)
                opAssign(use_generic);
            else
                assert(false && "missing assignment operator");
        }
        if(traits & asOBJ_APP_CLASS_K)
        {
            if constexpr(is_only_constructible_v<Class, const Class&>)
                copy_constructor(use_generic);
            else
                assert(false && "missing copy constructor");
        }

        return *this;
    }

    /**
     * @brief Automatically register functions based on traits.
     *
     * @param traits Type traits
     */
    value_class& behaviours_by_traits(
        asQWORD traits = AS_NAMESPACE_QUALIFIER asGetTypeTraits<Class>()
    )
    {
        if(traits & asOBJ_APP_CLASS_C)
        {
            if constexpr(is_only_constructible_v<Class>)
                default_constructor();
            else
                assert(false && "missing default constructor");
        }
        if(traits & asOBJ_APP_CLASS_D)
        {
            if constexpr(std::is_destructible_v<Class>)
                destructor();
            else
                assert(false && "missing destructor");
        }
        if(traits & asOBJ_APP_CLASS_A)
        {
            if constexpr(std::is_copy_assignable_v<Class>)
                opAssign();
            else
                assert(false && "missing assignment operator");
        }
        if(traits & asOBJ_APP_CLASS_K)
        {
            if constexpr(is_only_constructible_v<Class, const Class&>)
                copy_constructor();
            else
                assert(false && "missing copy constructor");
        }

        return *this;
    }

    constexpr std::string decl_list_constructor(std::string_view pattern) const
    {
        return string_concat("void f(int&in){", pattern, "}");
    }

    value_class& list_constructor_function(std::string_view pattern, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC> = {})
    {
        this->behaviour_impl(
            asBEHAVE_LIST_CONSTRUCT,
            decl_list_constructor(pattern).c_str(),
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <asECallConvTypes CallConv, typename... Args>
    requires(CallConv == asCALL_CDECL_OBJFIRST || CallConv == asCALL_CDECL_OBJLAST)
    value_class& list_constructor_function(
        std::string_view pattern,
        void (*fn)(Args...),
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            asBEHAVE_LIST_CONSTRUCT,
            decl_list_constructor(pattern).c_str(),
            fn,
            call_conv<CallConv>
        );

        return *this;
    }

    template <native_function auto ListConstructor, asECallConvTypes CallConv>
    requires(CallConv == asCALL_CDECL_OBJFIRST || CallConv == asCALL_CDECL_OBJLAST)
    value_class& list_constructor_function(
        use_generic_t,
        std::string_view pattern
    )
    {
        using traits = function_traits<std::decay_t<decltype(ListConstructor)>>;
        static_assert(traits::arg_count_v == 2);
        static_assert(std::is_void_v<typename traits::return_type>);

        if constexpr(CallConv == asCALL_CDECL_OBJFIRST)
        {
            using list_buf_t = typename traits::last_arg_type;
            static_assert(std::is_pointer_v<list_buf_t>);
            list_constructor_function(
                pattern,
                +[](asIScriptGeneric* gen) -> void
                {
                    ListConstructor(
                        get_generic_object<Class*>(gen),
                        *(list_buf_t*)gen->GetAddressOfArg(0)
                    );
                },
                call_conv<asCALL_GENERIC>
            );
        }
        else // CallConv == asCALL_CDECL_OBJLAST
        {
            using list_buf_t = typename traits::first_arg_type;
            static_assert(std::is_pointer_v<list_buf_t>);
            list_constructor_function(
                pattern,
                +[](asIScriptGeneric* gen) -> void
                {
                    ListConstructor(
                        *(list_buf_t*)gen->GetAddressOfArg(0),
                        get_generic_object<Class*>(gen)
                    );
                },
                call_conv<asCALL_GENERIC>
            );
        }

        return *this;
    }

    template <native_function auto ListConstructor, asECallConvTypes CallConv>
    requires(CallConv == asCALL_CDECL_OBJFIRST || CallConv == asCALL_CDECL_OBJLAST)
    value_class& list_constructor_function(std::string_view pattern)
    {
        if constexpr(ForceGeneric)
        {
            list_constructor_function<ListConstructor, CallConv>(use_generic, pattern);
        }
        else
        {
            list_constructor_function(pattern, ListConstructor, call_conv<CallConv>);
        }

        return *this;
    }

    /**
     * @brief Register a list constructor with limited list size
     *
     * @tparam ListElementType Element type
     * @param pattern List pattern
     */
    template <typename ListElementType = void>
    value_class& list_constructor(
        use_generic_t, std::string_view pattern
    )
    {
        list_constructor_function(
            pattern,
            wrappers::list_constructor<Class, ListElementType>{}(use_generic),
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    /**
     * @brief Register a list constructor with limited list size
     *
     * @tparam ListElementType Element type
     * @param pattern List pattern
     */
    template <typename ListElementType = void>
    value_class& list_constructor(
        std::string_view pattern
    )
    {
        if constexpr(ForceGeneric)
            list_constructor<ListElementType>(use_generic, pattern);
        else
        {
            list_constructor_function(
                pattern,
                wrappers::list_constructor<Class, ListElementType>{}(call_conv<asCALL_CDECL_OBJLAST>),
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
    ASBIND20_VALUE_CLASS_OP(opPostInc);
    ASBIND20_VALUE_CLASS_OP(opPostDec);

    ASBIND20_VALUE_CLASS_OP(opAssign);
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

    template <typename To>
    value_class& opConv(use_generic_t, std::string_view to_decl)
    {
        this->template opConv_impl_generic<Class, To>(to_decl, false);

        return *this;
    }

    template <typename To>
    value_class& opConv(std::string_view to_decl)
    {
        if constexpr(ForceGeneric)
            opConv<To>(use_generic, to_decl);
        else
        {
            this->template opConv_impl_native<Class, To>(to_decl, false);
        }

        return *this;
    }

    template <typename To>
    value_class& opImplConv(use_generic_t, std::string_view to_decl)
    {
        this->template opConv_impl_generic<Class, To>(to_decl, true);

        return *this;
    }

    template <typename To>
    value_class& opImplConv(std::string_view to_decl)
    {
        if constexpr(ForceGeneric)
            opImplConv<To>(use_generic, to_decl);
        else
        {
            this->template opConv_impl_native<Class, To>(to_decl, true);
        }

        return *this;
    }

    template <has_static_name To>
    value_class& opConv(use_generic_t)
    {
        opConv<To>(use_generic, name_of<To>());

        return *this;
    }

    template <has_static_name To>
    value_class& opConv()
    {
        opConv<To>(name_of<To>());

        return *this;
    }

    template <has_static_name To>
    value_class& opImplConv(use_generic_t)
    {
        opImplConv<To>(use_generic, name_of<To>());

        return *this;
    }

    template <has_static_name To>
    value_class& opImplConv()
    {
        opImplConv<To>(name_of<To>());

        return *this;
    }

    template <typename Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    value_class& behaviour(asEBehaviours beh, const char* decl, Fn&& fn, call_conv_t<asCALL_THISCALL> = {}) requires(!ForceGeneric)
    {
        behaviour_impl(beh, decl, std::forward<Fn>(fn), call_conv<asCALL_THISCALL>);

        return *this;
    }

    template <typename R, typename... Args, asECallConvTypes CallConv>
    requires(native_function<R (*)(Args...)> && CallConv != asCALL_GENERIC)
    value_class& behaviour(asEBehaviours beh, const char* decl, R (*fn)(Args...), call_conv_t<CallConv>) requires(!ForceGeneric)
    {
        this->behaviour_impl(beh, decl, fn, call_conv<CallConv>);

        return *this;
    }

    value_class& behaviour(asEBehaviours beh, const char* decl, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC> = {})
    {
        this->behaviour_impl(beh, decl, gfn, call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <native_function Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    value_class& method(const char* decl, Fn&& fn, call_conv_t<asCALL_THISCALL> = {}) requires(!ForceGeneric)
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

    /**
     * @brief Register a generic function.
     */
    value_class& method(const char* decl, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC> = {})
    {
        this->method_impl(decl, gfn, call_conv<asCALL_GENERIC>);

        return *this;
    }

    /**
     * @brief Register a function with generated generic wrapper.
     */
    template <
        native_function auto Method,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Method, Class>()>
    [[deprecated("Use the version with fp<>")]]
    value_class& method(use_generic_t, const char* decl)
    {
        method(decl, to_asGENFUNC_t(fp<Method>, call_conv<CallConv>), call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <
        auto Method,
        asECallConvTypes CallConv>
    value_class& method(
        use_generic_t,
        const char* decl,
        fp_wrapper_t<Method>,
        call_conv_t<CallConv>
    )
    {
        this->method_impl(
            decl,
            to_asGENFUNC_t(fp<Method>, call_conv<CallConv>),
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <auto Method>
    value_class& method(
        use_generic_t,
        const char* decl,
        fp_wrapper_t<Method>
    )
    {
        constexpr asECallConvTypes conv = detail::deduce_method_callconv<Method, Class>();
        this->method_impl(
            decl,
            to_asGENFUNC_t(fp<Method>, call_conv<conv>),
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <
        native_function auto Method,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Method, Class>()>
    [[deprecated("Use the version with fp<>")]]
    value_class& method(const char* decl)
    {
        if constexpr(ForceGeneric)
            this->template method<Method, CallConv>(use_generic, decl);
        else
            this->method(decl, Method, call_conv<CallConv>);

        return *this;
    }

    template <
        auto Method,
        asECallConvTypes CallConv>
    value_class& method(
        const char* decl,
        fp_wrapper_t<Method>,
        call_conv_t<CallConv>
    )
    {
        if constexpr(ForceGeneric)
            this->method(use_generic, decl, fp<Method>, call_conv<CallConv>);
        else
            this->method(decl, Method, call_conv<CallConv>);

        return *this;
    }

    template <auto Method>
    value_class& method(
        const char* decl,
        fp_wrapper_t<Method>
    )
    {
        constexpr asECallConvTypes conv = detail::deduce_method_callconv<Method, Class>();
        if constexpr(ForceGeneric)
            this->method(use_generic, decl, fp<Method>, call_conv<conv>);
        else
            this->method(decl, Method, call_conv<conv>);

        return *this;
    }

    template <typename R, typename... Args>
    value_class& method(const char* decl, R (*fn)(Args...)) requires(!ForceGeneric)
    {
        this->template method_auto_callconv<Class>(decl, fn);

        return *this;
    }

    template <
        noncapturing_lambda Lambda,
        asECallConvTypes CallConv = detail::deduce_lambda_callconv<Lambda, Class>()>
    value_class& method(
        use_generic_t,
        const char* decl,
        const Lambda&,
        call_conv_t<CallConv> = {}
    )
    {
        this->method_impl(
            decl,
            to_asGENFUNC_t(Lambda{}, call_conv<CallConv>),
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <
        noncapturing_lambda Lambda,
        asECallConvTypes CallConv = detail::deduce_lambda_callconv<Lambda, Class>()>
    value_class& method(
        const char* decl,
        const Lambda&,
        call_conv_t<CallConv> = {}
    )
    {
        if constexpr(ForceGeneric)
            this->method(use_generic, decl, Lambda{}, call_conv<CallConv>);
        else
            this->method_impl(decl, +Lambda{}, call_conv<CallConv>);
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

        this->template register_object_type<Class>(m_flags);
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
                    std::invoke(
                        Callback,
                        get_generic_arg<asITypeInfo*>(gen, 0),
                        refptr_wrapper<bool>(get_generic_arg<bool*>(gen, 1))
                    )
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
    requires(std::is_member_function_pointer_v<std::decay_t<Fn>>)
    reference_class& method(const char* decl, Fn&& fn, call_conv_t<asCALL_THISCALL> = {}) requires(!ForceGeneric)
    {
        this->method_impl(decl, std::forward<Fn>(fn), call_conv<asCALL_THISCALL>);

        return *this;
    }

    template <native_function Fn, asECallConvTypes CallConv>
    requires(!std::is_member_function_pointer_v<std::decay_t<Fn>> && CallConv != asCALL_GENERIC)
    reference_class& method(const char* decl, Fn&& fn, call_conv_t<CallConv>) requires(!ForceGeneric)
    {
        this->method_impl(decl, std::forward<Fn>(fn), call_conv<CallConv>);

        return *this;
    }

    reference_class& method(const char* decl, asGENFUNC_t gfn, call_conv_t<asCALL_GENERIC> = {})
    {
        this->method_impl(decl, gfn, call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <typename R, typename... Args>
    requires(native_function<R (*)(Args...)>)
    reference_class& method(const char* decl, R (*fn)(Args...)) requires(!ForceGeneric)
    {
        this->template method_auto_callconv<Class>(decl, fn);

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Function, Class>()>
    [[deprecated("Use the version with fp<>")]]
    reference_class& method(use_generic_t, const char* decl)
    {
        method(decl, to_asGENFUNC_t(fp<Function>, call_conv<CallConv>), call_conv<asCALL_GENERIC>);

        return *this;
    }

    template <
        native_function auto Function,
        asECallConvTypes CallConv = detail::deduce_method_callconv<Function, Class>()>
    [[deprecated("Use the version with fp<>")]]
    reference_class& method(const char* decl)
    {
        if constexpr(ForceGeneric)
            this->template method<Function, CallConv>(use_generic, decl);
        else
            this->method_impl(decl, Function, call_conv<CallConv>);

        return *this;
    }

    template <
        auto Function,
        asECallConvTypes CallConv>
    reference_class& method(
        use_generic_t,
        const char* decl,
        fp_wrapper_t<Function>,
        call_conv_t<CallConv>
    )
    {
        this->method_impl(
            decl,
            to_asGENFUNC_t(fp<Function>, call_conv<CallConv>),
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <
        auto Function,
        asECallConvTypes CallConv>
    reference_class& method(
        const char* decl,
        fp_wrapper_t<Function>,
        call_conv_t<CallConv>
    )
    {
        if constexpr(ForceGeneric)
            this->method(use_generic, decl, fp<Function>, call_conv<CallConv>);
        else
            this->method_impl(decl, Function, call_conv<CallConv>);

        return *this;
    }

    template <auto Function>
    reference_class& method(
        use_generic_t,
        const char* decl,
        fp_wrapper_t<Function>
    )
    {
        constexpr asECallConvTypes conv = detail::deduce_method_callconv<Function, Class>();
        this->method_impl(
            decl,
            to_asGENFUNC_t(fp<Function>, call_conv<conv>),
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <auto Function>
    reference_class& method(
        const char* decl,
        fp_wrapper_t<Function>
    )
    {
        constexpr asECallConvTypes conv = detail::deduce_method_callconv<Function, Class>();
        if constexpr(ForceGeneric)
            this->method(decl, fp<Function>, call_conv<conv>);
        else
            this->method_impl(decl, Function, call_conv<conv>);

        return *this;
    }

    std::string decl_factory(std::string_view params) const
    {
        if constexpr(Template)
        {
            return params.empty() ?
                       string_concat(m_name, "@f(int&in)") :
                       string_concat(m_name, "@f(int&in,", params, ')');
        }
        else
        {
            return params.empty() ?
                       string_concat(m_name, "@f()") :
                       string_concat(m_name, "@f(", params, ')');
        }
    }

    std::string decl_factory(std::string_view params, use_explicit_t) const
    {
        if constexpr(Template)
        {
            return params.empty() ?
                       string_concat(m_name, "@f(int&in)explicit") :
                       string_concat(m_name, "@f(int&in,", params, ")explicit");
        }
        else
        {
            return params.empty() ?
                       string_concat(m_name, "@f()explicit") :
                       string_concat(m_name, "@f(", params, ")explicit");
        }
    }

private:
    template <typename... Args>
    static consteval bool check_factory_args()
    {
        if constexpr(Template)
        {
            using args_tuple = std::tuple<Args...>;
            if constexpr(std::tuple_size_v<args_tuple> >= 1)
            {
                return std::convertible_to<
                    asITypeInfo*,
                    std::tuple_element_t<0, args_tuple>>;
            }
            else
                return false;
        }
        else
            return true;
    }

public:
    template <typename... Args>
    requires(check_factory_args<Args...>())
    reference_class& factory_function(
        std::string_view params,
        Class* (*fn)(Args...),
        call_conv_t<asCALL_CDECL> = {}
    ) requires(!ForceGeneric)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterObjectBehaviour(
            m_name,
            asBEHAVE_FACTORY,
            decl_factory(params).c_str(),
            to_asSFuncPtr(fn),
            asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    template <typename... Args>
    requires(check_factory_args<Args...>())
    reference_class& factory_function(
        std::string_view params,
        use_explicit_t,
        Class* (*fn)(Args...),
        call_conv_t<asCALL_CDECL> = {}
    ) requires(!ForceGeneric)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterObjectBehaviour(
            m_name,
            asBEHAVE_FACTORY,
            decl_factory(params, use_explicit).c_str(),
            to_asSFuncPtr(fn),
            asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    reference_class& factory_function(
        std::string_view params,
        asGENFUNC_t gfn,
        call_conv_t<asCALL_GENERIC> = {}
    )
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterObjectBehaviour(
            m_name,
            asBEHAVE_FACTORY,
            decl_factory(params).c_str(),
            to_asSFuncPtr(gfn),
            asCALL_GENERIC
        );
        assert(r >= 0);

        return *this;
    }

    reference_class& factory_function(
        std::string_view params,
        use_explicit_t,
        asGENFUNC_t gfn,
        call_conv_t<asCALL_GENERIC> = {}
    )
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterObjectBehaviour(
            m_name,
            asBEHAVE_FACTORY,
            decl_factory(params, use_explicit).c_str(),
            to_asSFuncPtr(gfn),
            asCALL_GENERIC
        );
        assert(r >= 0);

        return *this;
    }

    template <native_function auto Function>
    reference_class& factory_function(
        use_generic_t,
        std::string_view params
    )
    {
        factory_function(
            params,
            to_asGENFUNC_t(fp<Function>, call_conv<asCALL_CDECL>),
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <native_function auto Function>
    reference_class& factory_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t
    )
    {
        factory_function(
            params,
            use_explicit,
            to_asGENFUNC_t(fp<Function>, call_conv<asCALL_CDECL>),
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <native_function auto Function, asECallConvTypes CallConv = asCALL_CDECL>
    reference_class& factory_function(
        std::string_view params
    )
    {
        if constexpr(ForceGeneric)
            factory_function<Function>(use_generic, params);
        else
            factory_function(params, Function, call_conv<CallConv>);

        return *this;
    }

    template <native_function auto Function, asECallConvTypes CallConv = asCALL_CDECL>
    reference_class& factory_function(
        std::string_view params,
        use_explicit_t
    )
    {
        if constexpr(ForceGeneric)
            factory_function<Function>(use_generic, params, use_explicit);
        else
            factory_function(params, use_explicit, Function, call_conv<CallConv>);

        return *this;
    }

    reference_class& default_factory(use_generic_t)
    {
        if constexpr(Template)
        {
            factory_function(
                "",
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
                "",
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
            auto wrapper =
                wrappers::factory<Class, Template>{}(call_conv<asCALL_CDECL>);

            factory_function(
                "",
                wrapper,
                call_conv<asCALL_CDECL>
            );
        }

        return *this;
    }

private:
    template <typename... Args>
    void factory_impl_generic(use_generic_t, std::string_view params, bool explicit_)
    {
        asGENFUNC_t wrapper =
            wrappers::factory<Class, Template, Args...>{}(use_generic);

        if(explicit_)
        {
            factory_function(
                params,
                use_explicit,
                wrapper,
                call_conv<asCALL_GENERIC>
            );
        }
        else
        {
            factory_function(
                params,
                wrapper,
                call_conv<asCALL_GENERIC>
            );
        }
    }

    template <typename... Args>
    void factory_impl_native(std::string_view params, bool explicit_) requires(!ForceGeneric)
    {
        auto wrapper =
            wrappers::factory<Class, Template, Args...>{}(call_conv<asCALL_CDECL>);

        if(explicit_)
        {
            factory_function(
                params,
                use_explicit,
                wrapper,
                call_conv<asCALL_CDECL>
            );
        }
        else
        {
            factory_function(
                params,
                wrapper,
                call_conv<asCALL_CDECL>
            );
        }
    }

public:
    template <typename... Args>
    reference_class& factory(use_generic_t, std::string_view params)
    {
        this->factory_impl_generic<Args...>(use_generic, params, false);

        return *this;
    }

    template <typename... Args>
    reference_class& factory(use_generic_t, std::string_view params, use_explicit_t)
    {
        this->factory_impl_generic<Args...>(use_generic, params, true);

        return *this;
    }

    template <typename... Args>
    reference_class& factory(std::string_view params)
    {
        if constexpr(ForceGeneric)
        {
            factory<Args...>(use_generic, params);
        }
        else
        {
            this->factory_impl_native<Args...>(params, false);
        }

        return *this;
    }

    template <typename... Args>
    reference_class& factory(std::string_view params, use_explicit_t)
    {
        if constexpr(ForceGeneric)
        {
            factory<Args...>(use_generic, params, use_explicit);
        }
        else
        {
            this->factory_impl_native<Args...>(params, true);
        }

        return *this;
    }

    std::string decl_list_factory(std::string_view pattern) const
    {
        if constexpr(Template)
            return string_concat(m_name, "@f(int&in,int&in){", pattern, "}");
        else
            return string_concat(m_name, "@f(int&in){", pattern, "}");
    }

    template <typename... Args, asECallConvTypes CallConv = asCALL_CDECL>
    requires(CallConv == asCALL_CDECL)
    reference_class& list_factory_function(
        std::string_view pattern,
        Class* (*fn)(Args...),
        call_conv_t<CallConv> = {}
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            asBEHAVE_LIST_FACTORY,
            decl_list_factory(pattern).c_str(),
            fn,
            call_conv<asCALL_CDECL>
        );

        return *this;
    }

    reference_class& list_factory_function(
        std::string_view pattern,
        asGENFUNC_t gfn,
        call_conv_t<asCALL_GENERIC> = {}
    )
    {
        this->behaviour_impl(
            asBEHAVE_LIST_FACTORY,
            decl_list_factory(pattern).c_str(),
            gfn,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <typename ListElementType = void>
    reference_class& list_factory(
        use_generic_t,
        std::string_view pattern
    )
    {
        asGENFUNC_t wrapper =
            wrappers::list_factory<Class, Template, ListElementType>{}(use_generic);

        list_factory_function(
            pattern,
            wrapper,
            call_conv<asCALL_GENERIC>
        );

        return *this;
    }

    template <typename ListElementType = void>
    reference_class& list_factory(std::string_view pattern)
    {
        if constexpr(ForceGeneric)
        {
            list_factory<ListElementType>(use_generic, pattern);
        }
        else
        {
            auto wrapper =
                wrappers::list_factory<Class, Template, ListElementType>{}(call_conv<asCALL_CDECL>);

            list_factory_function(
                pattern,
                wrapper,
                call_conv<asCALL_CDECL>
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
    ASBIND20_REFERENCE_CLASS_OP(opAddAssign);
    ASBIND20_REFERENCE_CLASS_OP(opSubAssign);
    ASBIND20_REFERENCE_CLASS_OP(opMulAssign);
    ASBIND20_REFERENCE_CLASS_OP(opDivAssign);

    ASBIND20_REFERENCE_CLASS_OP(opEquals);
    ASBIND20_REFERENCE_CLASS_OP(opCmp);

    ASBIND20_REFERENCE_CLASS_OP(opPreInc);
    ASBIND20_REFERENCE_CLASS_OP(opPreDec);

#undef ASBIND20_REFERENCE_CLASS_OP

    template <typename To>
    reference_class& opConv(use_generic_t, std::string_view to_decl)
    {
        this->template opConv_impl_generic<Class, To>(to_decl, false);

        return *this;
    }

    template <typename To>
    reference_class& opConv(std::string_view to_decl)
    {
        if constexpr(ForceGeneric)
            opConv<To>(use_generic, to_decl);
        else
        {
            this->template opConv_impl_native<Class, To>(to_decl, false);
        }

        return *this;
    }

    template <typename To>
    reference_class& opImplConv(use_generic_t, std::string_view to_decl)
    {
        this->template opConv_impl_generic<Class, To>(to_decl, true);

        return *this;
    }

    template <typename To>
    reference_class& opImplConv(std::string_view to_decl)
    {
        if constexpr(ForceGeneric)
            opImplConv<To>(use_generic, to_decl);
        else
        {
            this->template opConv_impl_native<Class, To>(to_decl, true);
        }

        return *this;
    }

    template <has_static_name To>
    reference_class& opConv(use_generic_t)
    {
        opConv<To>(use_generic, name_of<To>());

        return *this;
    }

    template <has_static_name To>
    reference_class& opConv()
    {
        opConv<To>(name_of<To>());

        return *this;
    }

    template <has_static_name To>
    reference_class& opImplConv(use_generic_t)
    {
        opImplConv<To>(use_generic, name_of<To>());

        return *this;
    }

    template <has_static_name To>
    reference_class& opImplConv()
    {
        opImplConv<To>(name_of<To>());

        return *this;
    }

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
        addref(to_asGENFUNC_t(fp<AddRef>, call_conv<asCALL_THISCALL>));

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
        release(to_asGENFUNC_t(fp<Release>, call_conv<asCALL_THISCALL>));

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
        get_refcount(to_asGENFUNC_t(fp<GetRefCount>, call_conv<asCALL_THISCALL>));

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
        set_gc_flag(to_asGENFUNC_t(fp<SetGCFlag>, call_conv<asCALL_THISCALL>));

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
        get_gc_flag(to_asGENFUNC_t(fp<GetGCFlag>, call_conv<asCALL_THISCALL>));

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
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterInterface(m_name);
        assert(r >= 0);
    }

    interface& method(const char* decl)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterInterfaceMethod(
            m_name,
            decl
        );
        assert(r >= 0);

        return *this;
    }

    interface& funcdef(std::string_view decl)
    {
        std::string full_decl = detail::generate_member_funcdef(
            m_name, decl
        );

        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterFuncdef(full_decl.data());
        assert(r >= 0);

        return *this;
    }

    asIScriptEngine* get_engine() const noexcept
    {
        return m_engine;
    }

private:
    asIScriptEngine* m_engine;
    const char* m_name;
};

template <typename Enum>
requires(std::is_enum_v<Enum>)
class enum_
{
public:
    enum_(asIScriptEngine* engine, const char* name)
        : m_engine(engine), m_name(name)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterEnum(m_name);
        assert(r >= 0);
    }

    enum_& value(Enum val, const char* decl)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterEnumValue(
            m_name,
            decl,
            static_cast<int>(val)
        );
        assert(r >= 0);

        return *this;
    }

    template <Enum Value>
    enum_& value()
    {
        this->value(
            Value,
            std::string(static_enum_name<Value>()).c_str()
        );
        return *this;
    }

    asIScriptEngine* get_engine() const noexcept
    {
        return m_engine;
    }

private:
    asIScriptEngine* m_engine;
    const char* m_name;
};
} // namespace asbind20

#endif
