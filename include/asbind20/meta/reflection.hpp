#ifndef ASBIND20_META_REFLECTION_HPP
#define ASBIND20_META_REFLECTION_HPP

#include <cstdint>
#include "../util/strutil.hpp"
#include "../utility.hpp"
#include "refl_common.hpp"
#include "../bind/calling_convention.hpp"
#include "annotation.hpp"

#ifdef ASBIND20_HAS_LIB_REFLECTION

namespace asbind20::meta
{
consteval bool is_generic_function_type(std::meta::info r);

consteval bool is_ptrref_type(std::meta::info r);

consteval std::meta::info remove_ptrref(std::meta::info r);

consteval bool has_annotation_with_type(std::meta::info r, std::meta::info ann);

template <typename T>
consteval std::optional<T> extract_last_annotation(std::meta::info r);

consteval std::string_view script_integral_name_of(std::meta::info r);

consteval std::string_view script_symbol_of(
    std::meta::operators op,
    unsigned int operand_count,
    bool prefer_r_suffix_or_postifx = false
);

consteval std::string_view script_identifier_of(std::meta::info r);

consteval std::vector<std::meta::info> parameters_of_with_calling_convention(
    std::meta::info r,
    asbind20::detail::call_conv_type conv
);

consteval std::string_view script_type_declaration_of(
    std::meta::info r,
    bool prefer_handle = false
);

consteval std::string_view script_parameter_type_modifier_of(
    std::meta::info r,
    bool prefer_inout_for_mutable_ptrref = true
);

consteval std::string_view script_parameter_declaration_of(
    std::meta::info r
);

consteval std::string_view script_namespace_declaration_of(
    std::meta::info r, bool full_declaration = false
);

consteval std::string_view script_parameter_list_declaration_of_with_calling_convention(
    std::meta::info func,
    asbind20::detail::call_conv_type conv = AS_NAMESPACE_QUALIFIER asCALL_CDECL
);

consteval bool is_const_method_with_calling_convention(
    std::meta::info func,
    asbind20::detail::call_conv_type conv
)
{
    if(conv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL)
        return false;

    if(conv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL)
    {
        if(!std::meta::is_class_member(func))
            throw std::meta::exception("asCALL_THISCALL requires member function", func);
        return std::meta::is_const(func);
    }

    const bool obj_first =
        conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        conv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST;
    const bool obj_last =
        conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST ||
        conv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST;

    if(!obj_first && !obj_last)
        return false;

    auto params = std::meta::parameters_of(func);
    if(params.empty())
        return false;

    auto param_type = std::meta::type_of(
        // Prefer OBJFIRST like rest of the library if both are true
        obj_first ? params.front() : params.back()
    );
    return std::meta::is_const(remove_ptrref(param_type));
}

consteval std::string_view script_return_type_declaration_of(std::meta::info func);

consteval std::string_view script_function_declaration_of_with_calling_convention(
    std::meta::info func,
    asbind20::detail::call_conv_type conv,
    bool skip_func_name = false
);

consteval std::string_view script_property_declaration_of(
    std::meta::info prop,
    bool prefer_handle = false
);

template <std::meta::info Function>
struct function_refl_proxy
{
    constexpr function_refl_proxy() = default;

    template <
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv = AS_NAMESPACE_QUALIFIER asCALL_CDECL>
    static consteval cstring_ref get_decl(
        asbind20::detail::call_conv_t<CallConv> = {}
    )
    {
        return std::define_static_string(
            script_function_declaration_of_with_calling_convention(
                Function, CallConv
            )
        );
    }

    static constexpr auto get_func()
    {
        return &[:Function:];
    }
};

template <std::meta::info Property>
struct prop_refl_proxy
{
    constexpr prop_refl_proxy() = default;

    static constexpr cstring_ref get_decl()
    {
        return std::define_static_string(
            script_property_declaration_of(
                Property,
                has_annotation_with_type(Property, ^^annotations::as_handle_t)
            )
        );
    }

    // For global properties
    static constexpr auto* get_addr()
    {
        static_assert(std::meta::is_variable(Property));
        return std::addressof([:Property:]);
    }

    // For member variables
    static constexpr std::size_t get_off()
    {
        static_assert(std::meta::is_class_member(Property));

        constexpr auto off = std::meta::offset_of(Property);
        static_assert(off.bits == 0, "No bit field");
        return static_cast<std::size_t>(off.bytes);
    }
};

template <std::meta::info Enumerator>
struct enum_refl_proxy
{
    static constexpr cstring_ref get_decl()
    {
        return std::define_static_string(
            script_identifier_of(Enumerator)
        );
    }

    static constexpr compat::script_enum_value_type get_val()
    {
        return static_cast<compat::script_enum_value_type>([:Enumerator:]);
    }
};

template <std::meta::info TypeInfo>
struct type_refl_proxy
{
    using type = typename[:TypeInfo:];

    static constexpr cstring_ref get_decl()
    {
        return std::define_static_string(
            script_identifier_of(std::meta::remove_cvref(TypeInfo))
        );
    }
};
} // namespace asbind20::meta

namespace asbind20
{
template <std::meta::info Info>
consteval auto reflect()
{
    if constexpr(std::meta::is_function(Info))
        return meta::function_refl_proxy<Info>{};
    else if constexpr(std::meta::is_type(Info))
        return meta::type_refl_proxy<Info>{};
    else if constexpr(std::meta::is_enumerator(Info))
        return meta::enum_refl_proxy<Info>{};
    else if constexpr(std::meta::is_variable(Info))
        return meta::prop_refl_proxy<Info>{};
    else if constexpr(std::meta::is_class_member(Info))
        return meta::prop_refl_proxy<Info>{};
    else
        static_assert(false);
}
} // namespace asbind20

#endif

#include "reflection.inl"

#endif
