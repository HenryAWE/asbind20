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
consteval bool is_generic_function_type(std::meta::info r)
{
    return std::meta::is_convertible_type(r, ^^generic_function);
}

consteval bool is_ptrref_type(std::meta::info r)
{
    return std::meta::is_pointer_type(r) || std::meta::is_reference_type(r);
}

consteval std::meta::info remove_ptrref(std::meta::info r)
{
    return std::meta::remove_pointer(
        std::meta::remove_reference(r)
    );
}

consteval bool has_annotation_with_type(std::meta::info r, std::meta::info ann)
{
    auto anns = std::meta::annotations_of_with_type(r, ann);
    return !anns.empty();
}

consteval std::string_view script_integral_name_of(std::meta::info r)
{
    if(!std::meta::is_integral_type(r))
        throw "r does not represent an integral type";

    if(std::meta::is_same_type(r, ^^bool))
        return "bool";

    std::string result;
    if(std::meta::is_unsigned_type(r))
        result += 'u';
    result += "int";
    switch(std::meta::size_of(r))
    {
    case 1:
        result += '8';
        break;
    case 2:
        result += "16";
        break;
    case 4:
        // 32bit integers in AngelScript don't have suffix
        break;
    case 8:
        result += "64";
        break;

    default:
        // Compiler built-in 128bit integers or other strange integral types
        throw "invalid integral type";
    }

    return std::define_static_string(result);
}

consteval std::string_view script_identifier_of(std::meta::info r)
{
    auto rename_ann = std::meta::annotations_of_with_type(r, ^^asbind20::rename);
    if(!rename_ann.empty())
    {
        return std::meta::extract<asbind20::rename>(rename_ann.back()).get();
    }

    if(std::meta::is_type(r))
    {
        if(is_ptrref_type(r))
            throw "r represents a reference or pointer type";

        if(std::meta::is_integral_type(r))
            return script_integral_name_of(r);
        std::string_view name = std::meta::display_string_of(r);
        if(auto pos = name.rfind("::"); pos != std::string_view::npos)
        {
            name.remove_prefix(pos + 2);
        }
        return name;
    }

    return std::meta::identifier_of(r);
}

consteval std::vector<std::meta::info> parameters_of_with_calling_convention(
    std::meta::info r,
    asbind20::detail::call_conv_type conv
    )
{
    auto params = std::meta::parameters_of(r);
    if(params.empty())
        return {};

    const bool no_first =
        conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        conv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST;
    const bool no_last =
        conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST ||
        conv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST;

    if(no_first)
        params.erase(params.begin());
    if(no_last)
        params.pop_back();
    return params;
}

consteval std::string_view script_type_declaration_of(
    std::meta::info r,
    bool prefer_handle = false
)
{
    std::string result;
    if(std::meta::is_const_type(remove_ptrref(r)))
        result += "const ";
    result += script_identifier_of(std::meta::remove_cvref(remove_ptrref(r)));
    if(is_ptrref_type(r))
    {
        result += prefer_handle ? '@' : '&';
    }

    return std::define_static_string(result);
}

consteval std::string_view script_parameter_type_modifier_of(
    std::meta::info r,
    bool prefer_inout_for_mutable_ptrref = true
    )
{
    if(!std::meta::is_type(r) || !is_ptrref_type(r))
        throw "r does not represent a reference or pointer type";

    auto referred_type = remove_ptrref(r);
    if(std::meta::is_const_type(referred_type))
        return "in";

    // Mutable pointer or reference
    if(prefer_inout_for_mutable_ptrref)
        return "inout";
    return "out";
}

consteval std::string_view script_parameter_declaration_of(
    std::meta::info r
)
{
    if(!std::meta::is_function_parameter(r))
        throw "r does not represent a function parameter";

    std::string result;
    const auto type_info = std::meta::type_of(r);
    const bool param_as_handle = has_annotation_with_type(r, ^^asbind20::as_handle);
    result += script_type_declaration_of(type_info, param_as_handle);
    if(is_ptrref_type(type_info))
    {
        const bool prefer_out_ref = has_annotation_with_type(r, ^^asbind20::out_ref);
        if(prefer_out_ref && param_as_handle)
            throw "as_handle and out_ref are mutually exclusive";
        if(!param_as_handle)
            result += script_parameter_type_modifier_of(type_info, !prefer_out_ref);
    }

    if(std::meta::has_identifier(r))
    {
        result += ' ';
        result += script_identifier_of(r);
    }

    auto arg_ann = std::meta::annotations_of_with_type(
        r, ^^asbind20::default_arg
    );
    if(!arg_ann.empty())
    {
        result += '=';
        result += std::meta::extract<asbind20::default_arg>(arg_ann.back()).get();
    }

    return std::define_static_string(result);
}

consteval std::string_view script_parameter_list_declaration_of_with_calling_convention(
    std::meta::info func,
    asbind20::detail::call_conv_type conv = AS_NAMESPACE_QUALIFIER asCALL_CDECL
)
{
    auto params = std::define_static_array(
        parameters_of_with_calling_convention(func, conv)
    );

    std::string result;
    bool first = true;
    for(const auto& param : params)
    {
        if(!first)
            result += ',';
        first = false;
        result += script_parameter_declaration_of(param);
    }

    return std::define_static_string(result);
}

// Detect the "const method" case for functions registered with OBJFIRST/LAST
consteval bool is_const_method_with_calling_convention(
    std::meta::info func,
    asbind20::detail::call_conv_type conv
    )
{
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

consteval std::string_view script_function_declaration_of_with_calling_convention(
    std::meta::info func,
    asbind20::detail::call_conv_type conv,
    bool skip_func_name = false
    )
{
    if(!std::meta::has_identifier(func))
        skip_func_name = true;

    std::string suffix;
    if(conv != AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL)
    {
        if((std::meta::is_class_member(func) && std::meta::is_const(func)) ||
           is_const_method_with_calling_convention(func, conv))
        {
            suffix += "const";
        }
    }

    std::string_view func_identifer =
        skip_func_name ? "f" : script_identifier_of(func);

    auto ret_type = std::meta::return_type_of(func);
    std::string decl = string_concat(
        script_type_declaration_of(
            ret_type, has_annotation_with_type(func, ^^as_handle)
        ),
        ' ',
        func_identifer,
        '(',
        script_parameter_list_declaration_of_with_calling_convention(
            func, conv
        ),
        ')'
    );

    return std::define_static_string(decl + suffix);
}

consteval std::string_view script_property_declaration_of(
    std::meta::info prop,
    bool prefer_handle = false
)
{
    std::string result;

    result += script_type_declaration_of(
        std::meta::type_of(prop), prefer_handle
    );
    result += ' ';
    result += script_identifier_of(prop);

    return std::define_static_string(result);
}

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
                has_annotation_with_type(Property, ^^as_handle)
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

#else

// Namespace placeholder
namespace asbind20::meta
{}

#endif

#endif
