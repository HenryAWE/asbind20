/**
 * @file bind/global.hpp
 * @author HenryAWE
 * @brief Binding generator for global functions and variables
 */

#ifndef ASBIND20_BIND_GLOBAL_HPP
#define ASBIND20_BIND_GLOBAL_HPP

#pragma once

#include "common.hpp"
#include "wrappers.hpp"

namespace asbind20
{
template <bool ForceGeneric>
class global final : public register_helper_base<ForceGeneric>
{
    using my_base = register_helper_base<ForceGeneric>;

    using my_base::m_engine;

public:
    global() = delete;
    global(const global&) = default;

    global(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
        : my_base(engine) {}

    template <typename Auxiliary>
    static void* get_auxiliary_address(auxiliary_wrapper<Auxiliary> aux)
    {
        if constexpr(std::same_as<Auxiliary, this_type_t>)
        {
            static_assert(!sizeof(Auxiliary), "auxiliary(this_type) is invalid for a global function!");
        }
        else
        {
            return aux.get_address();
        }
    }

    template <
        native_function Fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    global& function(
        std::string_view decl,
        Fn&& fn,
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        static_assert(
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL || CallConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL,
            "Invalid calling convention for a global function"
        );

        [[maybe_unused]]
        int r = with_cstr(
            [this, &fn](const char* decl)
            {
                return m_engine->RegisterGlobalFunction(
                    decl,
                    detail::to_asSFuncPtr(fn),
                    CallConv
                );
            },
            decl
        );
        assert(r >= 0);

        return *this;
    }

    template <native_function Fn>
    global& function(
        std::string_view decl,
        Fn&& fn
    ) requires(!ForceGeneric)
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_function_callconv<std::decay_t<Fn>>();

        this->function(decl, fn, call_conv<conv>);

        return *this;
    }

    global& function(
        std::string_view decl,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        [[maybe_unused]]
        int r = with_cstr(
            [this, &gfn](const char* decl)
            {
                return m_engine->RegisterGlobalFunction(
                    decl,
                    detail::to_asSFuncPtr(gfn),
                    AS_NAMESPACE_QUALIFIER asCALL_GENERIC
                );
            },
            decl
        );
        assert(r >= 0);

        return *this;
    }

    template <
        auto Function,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    global& function(
        use_generic_t,
        std::string_view decl,
        fp_wrapper<Function>,
        call_conv_t<CallConv>
    )
    {
        static_assert(
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL || CallConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL,
            "Invalid calling convention for a global function"
        );

        this->function(
            decl,
            detail::to_asGENFUNC_t(fp<Function>, call_conv<CallConv>),
            generic_call_conv
        );

        return *this;
    }

    template <
        auto Function,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    global& function(
        std::string_view decl,
        fp_wrapper<Function>,
        call_conv_t<CallConv>
    )
    {
        static_assert(
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL || CallConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL,
            "Invalid calling convention for a global function"
        );

        if constexpr(ForceGeneric)
        {
            function(use_generic, decl, fp<Function>, call_conv<CallConv>);
        }
        else
        {
            this->function(decl, Function, call_conv<CallConv>);
        }

        return *this;
    }

    template <auto Function>
    global& function(
        use_generic_t,
        std::string_view decl,
        fp_wrapper<Function>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_function_callconv<std::decay_t<decltype(Function)>>();

        function(use_generic, decl, fp<Function>, call_conv<conv>);

        return *this;
    }

    template <auto Function>
    global& function(
        std::string_view decl,
        fp_wrapper<Function>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_function_callconv<std::decay_t<decltype(Function)>>();

        if constexpr(ForceGeneric)
        {
            function(use_generic, decl, fp<Function>, call_conv<conv>);
        }
        else
        {
            this->function(decl, Function, call_conv<conv>);
        }

        return *this;
    }

    template <noncapturing_lambda Lambda>
    global& function(
        use_generic_t,
        std::string_view decl,
        const Lambda&
    )
    {
        this->function(
            decl,
            detail::to_asGENFUNC_t(Lambda{}, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>),
            generic_call_conv
        );

        return *this;
    }

    template <noncapturing_lambda Lambda>
    global& function(
        std::string_view decl,
        const Lambda&
    )
    {
        if constexpr(ForceGeneric)
            this->function(use_generic, decl, Lambda{});
        else
            this->function(decl, +Lambda{}, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);

        return *this;
    }

    template <typename Auxiliary>
    global& function(
        std::string_view decl,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        [[maybe_unused]]
        int r = with_cstr(
            [this, &gfn, &aux](const char* decl)
            {
                return m_engine->RegisterGlobalFunction(
                    decl,
                    detail::to_asSFuncPtr(gfn),
                    AS_NAMESPACE_QUALIFIER asCALL_GENERIC,
                    get_auxiliary_address(aux)
                );
            },
            decl
        );
        assert(r >= 0);

        return *this;
    }

    template <typename Fn, typename Auxiliary>
    requires(std::is_member_function_pointer_v<Fn>)
    global& function(
        std::string_view decl,
        Fn&& fn,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL> = {}
    ) requires(!ForceGeneric)
    {
        [[maybe_unused]]
        int r = with_cstr(
            [this, &fn, &aux](const char* decl)
            {
                return m_engine->RegisterGlobalFunction(
                    decl,
                    detail::to_asSFuncPtr(fn),
                    AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL,
                    get_auxiliary_address(aux)
                );
            },
            decl
        );
        assert(r >= 0);

        return *this;
    }

    template <
        auto Function,
        typename Auxiliary>
    global& function(
        use_generic_t,
        std::string_view decl,
        fp_wrapper<Function>,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL> = {}
    )
    {
        static_assert(
            std::is_member_function_pointer_v<std::decay_t<decltype(Function)>>,
            "Function for asCALL_THISCALL_ASGLOBAL must be a member function"
        );

        function(
            decl,
            detail::to_asGENFUNC_t(fp<Function>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL>),
            aux,
            generic_call_conv
        );

        return *this;
    }

    template <
        auto Function,
        typename Auxiliary>
    global& function(
        std::string_view decl,
        fp_wrapper<Function>,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL> = {}
    )
    {
        static_assert(
            std::is_member_function_pointer_v<std::decay_t<decltype(Function)>>,
            "Function for asCALL_THISCALL_ASGLOBAL must be a member function"
        );

        if constexpr(ForceGeneric)
            function(use_generic, decl, fp<Function>, aux);
        else
            function(decl, Function, aux);

        return *this;
    }

    /**
     * @brief Register a global property
     */
    template <typename T>
    global& property(
        std::string_view decl,
        T& val
    )
    {
        [[maybe_unused]]
        int r = with_cstr(
            &AS_NAMESPACE_QUALIFIER asIScriptEngine::RegisterGlobalProperty,
            m_engine,
            decl,
            (void*)std::addressof(val)
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Register a funcdef
     *
     * @param decl Function declaration
     */
    global& funcdef(
        std::string_view decl
    )
    {
        [[maybe_unused]]
        int r = with_cstr(
            &AS_NAMESPACE_QUALIFIER asIScriptEngine::RegisterFuncdef,
            m_engine,
            decl
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Register a typedef
     *
     * @param type_decl Type declaration
     * @param new_name Aliased type name
     */
    global& typedef_(
        std::string_view type_decl,
        std::string_view new_name
    )
    {
        [[maybe_unused]]
        int r = with_cstr(
            &AS_NAMESPACE_QUALIFIER asIScriptEngine::RegisterTypedef,
            m_engine,
            new_name,
            type_decl
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Register a typedef in C++11 style
     *
     * @param new_name Aliased type name
     * @param type_decl Type declaration
     */
    global& using_(
        std::string_view new_name,
        std::string_view type_decl
    )
    {
        typedef_(type_decl, new_name);

        return *this;
    }

    /**
     * @brief Generic calling convention for message callback is not supported.
     */
    global& message_callback(
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        void* obj = nullptr
    ) = delete;

    /**
     * @brief Set the message callback.
     */
    template <native_function Callback>
    requires(!std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& message_callback(Callback fn, void* obj = nullptr)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->SetMessageCallback(
            detail::to_asSFuncPtr(fn),
            obj,
            AS_NAMESPACE_QUALIFIER asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Set a member function as the message callback.
     */
    template <native_function Callback, typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& message_callback(Callback fn, T& obj)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->SetMessageCallback(
            detail::to_asSFuncPtr(fn),
            (void*)std::addressof(obj),
            AS_NAMESPACE_QUALIFIER asCALL_THISCALL
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Generic calling convention for exception translator is not supported.
     */
    global& exception_translator(
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        void* obj = nullptr
    ) = delete;

    /**
     * @brief Set the exception translator.
     */
    template <native_function Callback>
    requires(!std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& exception_translator(Callback fn, void* obj = nullptr)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->SetTranslateAppExceptionCallback(
            detail::to_asSFuncPtr(fn),
            obj,
            AS_NAMESPACE_QUALIFIER asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Set a member function as the exception translator.
     */
    template <native_function Callback, typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& exception_translator(Callback fn, T& obj)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->SetTranslateAppExceptionCallback(
            detail::to_asSFuncPtr(fn),
            (void*)std::addressof(obj),
            AS_NAMESPACE_QUALIFIER asCALL_THISCALL
        );
        assert(r >= 0);

        return *this;
    }
};

global(AS_NAMESPACE_QUALIFIER asIScriptEngine*) -> global<false>;

global(const script_engine&) -> global<false>;
} // namespace asbind20

#endif
