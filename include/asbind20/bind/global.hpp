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
class global final : public binding_generator_base<ForceGeneric>
{
    using my_base = binding_generator_base<ForceGeneric>;

    using my_base::m_engine;

public:
    global() = delete;
    global(const global&) = default;

    global(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
        : my_base(engine) {}

    template <typename Auxiliary>
    [[nodiscard]]
    static void* get_auxiliary_address(auxiliary_wrapper<Auxiliary> aux)
    {
        static_assert(
            !std::same_as<Auxiliary, this_type_t>,
            "auxiliary(this_type) is invalid for a global function!"
        );

        return aux.get_address();
    }

    template <
        native_function Fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    global& function(
        cstring_ref decl,
        Fn&& fn,
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        static_assert(
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL || CallConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL,
            "Invalid calling convention for a global function"
        );

        [[maybe_unused]]
        int r = m_engine->RegisterGlobalFunction(
            decl.c_str(),
            detail::to_asSFuncPtr(fn),
            CallConv
        );
        assert(r >= 0);

        return *this;
    }

    template <native_function Fn>
    global& function(
        cstring_ref decl,
        Fn&& fn
    ) requires(!ForceGeneric)
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_function_callconv<std::decay_t<Fn>>();

        this->function(decl, fn, call_conv<conv>);

        return *this;
    }

    global& function(
        cstring_ref decl,
        generic_function gfn,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        [[maybe_unused]]
        int r = m_engine->RegisterGlobalFunction(
            decl.c_str(),
            detail::to_asSFuncPtr(gfn),
            AS_NAMESPACE_QUALIFIER asCALL_GENERIC
        );
        assert(r >= 0);

        return *this;
    }

    template <
        auto Function,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    global& function(
        use_generic_t,
        cstring_ref decl,
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
        cstring_ref decl,
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
        cstring_ref decl,
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
        cstring_ref decl,
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

    template <noncapturing_native_lambda Lambda>
    global& function(
        use_generic_t,
        cstring_ref decl,
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

    template <noncapturing_native_lambda Lambda>
    global& function(
        cstring_ref decl,
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
        cstring_ref decl,
        generic_function gfn,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        [[maybe_unused]]
        int r = m_engine->RegisterGlobalFunction(
            decl.c_str(),
            detail::to_asSFuncPtr(gfn),
            AS_NAMESPACE_QUALIFIER asCALL_GENERIC,
            get_auxiliary_address(aux)
        );
        assert(r >= 0);

        return *this;
    }

    template <typename Fn, typename Auxiliary>
    requires(std::is_member_function_pointer_v<Fn>)
    global& function(
        cstring_ref decl,
        Fn&& fn,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL> = {}
    ) requires(!ForceGeneric)
    {
        [[maybe_unused]]
        int r = m_engine->RegisterGlobalFunction(
            decl.c_str(),
            detail::to_asSFuncPtr(fn),
            AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL,
            get_auxiliary_address(aux)
        );
        assert(r >= 0);

        return *this;
    }

    template <
        auto Function,
        typename Auxiliary>
    global& function(
        use_generic_t,
        cstring_ref decl,
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
        cstring_ref decl,
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
        cstring_ref decl,
        T& val
    )
    {
        [[maybe_unused]]
        int r = m_engine->RegisterGlobalProperty(
            decl.c_str(),
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
        cstring_ref decl
    )
    {
        [[maybe_unused]]
        int r = m_engine->RegisterFuncdef(
            decl.c_str()
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
        cstring_ref type_decl,
        cstring_ref new_name
    )
    {
        [[maybe_unused]]
        int r = m_engine->RegisterTypedef(
            new_name.c_str(),
            type_decl.c_str()
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
        cstring_ref new_name,
        cstring_ref type_decl
    )
    {
        typedef_(type_decl, new_name);

        return *this;
    }

    /**
     * @brief Generic calling convention for message callback is not supported.
     */
    global& message_callback(
        generic_function gfn,
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
        generic_function gfn,
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
