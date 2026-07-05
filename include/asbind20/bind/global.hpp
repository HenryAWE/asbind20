/**
 * @file bind/global.hpp
 * @author HenryAWE
 * @brief Binding generator for global functions and variables
 */

#ifndef ASBIND20_BIND_GLOBAL_HPP
#define ASBIND20_BIND_GLOBAL_HPP

#pragma once

#include "common.hpp"
#include "function_tools.hpp"
#include "genfunc.hpp"

namespace asbind20
{
template <bool ForceGeneric, typename Listener = default_listener>
class global final : public binding_generator_interface<ForceGeneric, Listener>
{
    using my_base = binding_generator_interface<ForceGeneric, Listener>;
    using listener_traits_type = listener_traits<Listener>;

    template <typename Fn>
    void register_function(
        cstring_ref decl,
        Fn&& fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes conv,
        void* auxiliary = nullptr
    )
    {
        int r = get_engine()->RegisterGlobalFunction(
            decl.c_str(),
            detail::to_asSFuncPtr(fn),
            conv,
            auxiliary
        );
        listener_traits_type::on_function(this->get_listener(), *this, r);
    }

public:
    global() = delete;
    global(const global&) = default;

    explicit global(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
        : my_base(engine) {}

    // For keeping consistency with other binding generators
    template <bool AppendOnly>
    global(
        appending_t<AppendOnly>,
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
    )
        : global(engine)
    {}

    using my_base::get_engine;

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

    template <native_function Fn>
    global& function(
        cstring_ref decl,
        Fn&& fn
    ) requires(!ForceGeneric)
    {
        constexpr auto conv =
            detail::deduce_function_callconv<Fn>();
        this->register_function(decl, fn, conv);

        return *this;
    }

    global& function(
        cstring_ref decl,
        generic_function gfn
    )
    {
        this->register_function(decl, gfn, detail::generic_cc);
        return *this;
    }

    template <auto Function>
    global& function(
        use_generic_t,
        cstring_ref decl,
        fp_wrapper<Function>
    )
    {
        constexpr auto conv =
            detail::deduce_function_callconv<decltype(Function)>();
        this->register_function(
            decl,
            detail::to_asGENFUNC_t(fp<Function>, detail::cc<conv>),
            detail::generic_cc
        );

        return *this;
    }

    template <auto Function>
    global& function(
        cstring_ref decl,
        fp_wrapper<Function>
    )
    {
        if constexpr(ForceGeneric)
            function(use_generic, decl, fp<Function>);
        else
            this->function(decl, Function);
        return *this;
    }

    template <noncapturing_native_lambda Lambda>
    global& function(
        use_generic_t,
        cstring_ref decl,
        const Lambda&
    )
    {
        constexpr auto conv =
            detail::deduce_function_callconv<decltype(+Lambda{})>();
        this->register_function(
            decl,
            detail::to_asGENFUNC_t(
                Lambda{}, detail::cc<conv>
            ),
            detail::generic_cc
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
            this->function(decl, +Lambda{});
        return *this;
    }

    template <typename Auxiliary>
    global& function(
        cstring_ref decl,
        generic_function gfn,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        this->register_function(
            decl,
            gfn,
            detail::generic_cc,
            get_auxiliary_address(aux)
        );

        return *this;
    }

    template <typename Fn, typename Auxiliary>
    requires(std::is_member_function_pointer_v<Fn>)
    global& function(
        cstring_ref decl,
        Fn&& fn,
        auxiliary_wrapper<Auxiliary> aux
    ) requires(!ForceGeneric)
    {
        this->register_function(
            decl,
            fn,
            detail::cc<AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL>,
            get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        auto Function,
        typename Auxiliary>
    requires(std::is_member_function_pointer_v<decltype(Function)>)
    global& function(
        use_generic_t,
        cstring_ref decl,
        fp_wrapper<Function>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        this->register_function(
            decl,
            detail::to_asGENFUNC_t(
                fp<Function>, detail::cc<AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL>
            ),
            detail::generic_cc,
            get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        auto Function,
        typename Auxiliary>
    requires(std::is_member_function_pointer_v<decltype(Function)>)
    global& function(
        cstring_ref decl,
        fp_wrapper<Function>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        if constexpr(ForceGeneric)
            function(use_generic, decl, fp<Function>, aux);
        else
            function(decl, Function, aux);
        return *this;
    }

    template <fn_tools::wrapped_function Function>
    global& function(
        use_generic_t,
        cstring_ref decl,
        const Function&
    )
    {
        this->register_function(
            decl,
            Function::template generate<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>(),
            AS_NAMESPACE_QUALIFIER asCALL_GENERIC
        );
        return *this;
    }

    template <fn_tools::wrapped_function Function>
    global& function(
        cstring_ref decl,
        const Function&
    )
    {
        if constexpr(ForceGeneric)
            this->function(use_generic, decl, Function{});
        else
        {
            this->register_function(
                decl,
                Function::template generate<AS_NAMESPACE_QUALIFIER asCALL_CDECL>(),
                AS_NAMESPACE_QUALIFIER asCALL_CDECL
            );
        }
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
        int r = get_engine()->RegisterGlobalProperty(
            decl.c_str(),
            (void*)std::addressof(val)
        );
        listener_traits_type::on_global_property(
            this->get_listener(), *this, r
        );

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
        int r = get_engine()->RegisterFuncdef(
            decl.c_str()
        );
        listener_traits_type::on_funcdef(
            this->get_listener(), *this, r
        );
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
        int r = get_engine()->RegisterTypedef(
            new_name.c_str(),
            type_decl.c_str()
        );
        listener_traits_type::on_typedef(
            this->get_listener(), *this, r
        );
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
};

global(AS_NAMESPACE_QUALIFIER asIScriptEngine*) -> global<false>;

global(const script_engine&) -> global<false>;

template <bool AppendOnly>
global(appending_t<AppendOnly>, AS_NAMESPACE_QUALIFIER asIScriptEngine*) -> global<false>;
} // namespace asbind20

#endif
