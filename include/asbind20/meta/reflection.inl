#ifndef ASBIND20_META_REFLECTION_INL
#define ASBIND20_META_REFLECTION_INL

#pragma once

#include "reflection.hpp"

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

template <typename T>
consteval std::optional<T> extract_last_annotation(std::meta::info r)
{
    auto anns = std::meta::annotations_of_with_type(r, ^^T);
    if(anns.empty())
        return std::nullopt;
    return std::meta::extract<T>(anns.back());
}

consteval std::string_view script_integral_name_of(std::meta::info r)
{
    if(!std::meta::is_integral_type(r))
        throw std::meta::exception("r does not represent an integral type", r);

    if(std::meta::is_same_type(r, ^^bool))
        return "bool";
    // Special case
    if(std::meta::is_same_type(r, ^^std::byte))
        return "uint8";

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
        throw std::meta::exception("invalid integral type", r);
    }

    return std::define_static_string(result);
}

consteval std::string_view script_symbol_of(
    std::meta::operators op,
    unsigned int operand_count,
    bool prefer_r_suffix_or_postifx
)
{
    using namespace std::string_literals;
    using enum std::meta::operators;
    const bool has_reversed_version =
        op == op_plus ||
        (op == op_minus && operand_count == 2) ||
        op == op_star ||
        op == op_slash ||
        op == op_percent ||
        op == op_ampersand ||
        op == op_pipe ||
        op == op_caret ||
        op == op_less_less ||
        op == op_greater_greater;

    std::string result;
    switch(op)
    {
    case op_parentheses: return "opCall";
    case op_square_brackets: return "opIndex";

    case op_plus: result = "opAdd"; break;
    case op_minus:
        result = operand_count == 1 ? "opNeg" : "opSub";
        break;
    case op_star: result = "opMul"; break;
    case op_slash: result = "opDiv"; break;
    case op_percent: result = "opMod"; break;
    case op_ampersand: result = "opAnd"; break;
    case op_pipe: result = "opOr"; break;
    case op_caret: result = "opXor"; break;
    case op_less_less: result = "opShl"; break;
    case op_greater_greater: result = "opShr"; break;

    case op_spaceship: result = "opCmp"; break;
    case op_equals_equals: result = "opEquals"; break;

    case op_equals: return "opAssign";
    case op_plus_equals: return "opAddAssign";
    case op_minus_equals: return "opSubAssign";
    case op_star_equals: return "opMulAssign";
    case op_slash_equals: return "opDivAssign";
    case op_percent_equals: return "opModAssign";
    case op_ampersand_equals: return "opAndAssign";
    case op_pipe_equals: return "opOrAssign";
    case op_caret_equals: return "opXorAssign";

    case op_plus_plus:
        result = "op"s + (prefer_r_suffix_or_postifx ? "Post" : "Pre") + "Inc";
        break;
    case op_minus_minus:
        result = "op"s + (prefer_r_suffix_or_postifx ? "Post" : "Pre") + "Dec";
        break;

    default:
        throw std::invalid_argument(std::string("bad operator: ") + std::meta::symbol_of(op));
    }

    if(has_reversed_version && prefer_r_suffix_or_postifx)
        result += "_r";
    return std::define_static_string(result);
}

consteval std::string_view script_identifier_of(std::meta::info r)
{
    if(auto rename_ann = extract_last_annotation<annotations::rename>(r);
       rename_ann.has_value())
    {
        return rename_ann->get();
    }

    if(std::meta::is_type(r))
    {
        if(is_ptrref_type(r))
            throw std::meta::exception("r represents a reference or pointer type", r);

        if(std::meta::is_integral_type(r))
            return script_integral_name_of(r);
        std::string_view name = std::meta::has_identifier(r) ?
                                    std::meta::identifier_of(r) :
                                    std::meta::display_string_of(r);
        if(auto pos = name.rfind("::"); pos != std::string_view::npos)
        {
            name.remove_prefix(pos + 2);
        }
        return name;
    }

    return std::meta::identifier_of(r);
}

consteval std::vector<std::meta::info> parameters_of_with_calling_convention(
    std::meta::info r, asbind20::detail::call_conv_type conv
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
    std::meta::info r, bool prefer_handle
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
    std::meta::info r, bool prefer_inout_for_mutable_ptrref
)
{
    if(!std::meta::is_type(r) || !is_ptrref_type(r))
        throw std::meta::exception("r does not represent a reference or pointer type", r);

    auto referred_type = remove_ptrref(r);
    if(std::meta::is_const_type(referred_type))
        return "in";

    // Mutable pointer or reference
    if(prefer_inout_for_mutable_ptrref)
        return "inout";
    return "out";
}

consteval std::string_view script_parameter_declaration_of(std::meta::info r)
{
    if(!std::meta::is_function_parameter(r))
        throw std::meta::exception("r does not represent a function parameter", r);

    std::string result;
    const auto type_info = std::meta::type_of(r);
    auto ann_for_handle = extract_last_annotation<as_handle_t>(r);
    const bool param_as_handle = ann_for_handle.has_value();
    result += script_type_declaration_of(type_info, param_as_handle);
    if(is_ptrref_type(type_info))
    {
        const bool prefer_out_ref = has_annotation_with_type(r, ^^annotations::out_ref_t);
        if(prefer_out_ref && param_as_handle)
            throw std::meta::exception("as_handle and out_ref are mutually exclusive", r);
        if(param_as_handle && ann_for_handle->auto_handle)
            result += '+'; // auto handle of AngelScript "T@+"
        if(!param_as_handle)
            result += script_parameter_type_modifier_of(type_info, !prefer_out_ref);
    }

    if(std::meta::has_identifier(r))
    {
        result += ' ';
        result += script_identifier_of(r);
    }

    auto ann_for_arg = extract_last_annotation<annotations::default_arg>(r);
    if(ann_for_arg.has_value())
    {
        result += '=';
        result += ann_for_arg->get();
    }

    return std::define_static_string(result);
}

consteval std::string_view script_namespace_declaration_of(
    std::meta::info r, bool full_declaration
)
{
    bool is_alias = std::meta::is_namespace_alias(r);
    if(is_alias && full_declaration)
        throw std::meta::exception("full_declaration is not supported to namespace alias", r);
    if(!std::meta::is_namespace(r))
        throw std::meta::exception("r does not represent a namespace", r);

    if(!full_declaration)
        return std::meta::identifier_of(r);

    std::string result(script_identifier_of(r));
    std::meta::info current = r;
    while(std::meta::has_parent(r))
    {
        current = std::meta::parent_of(current);
        if(!std::meta::is_namespace(current))
            break;
        // Anonymous namespace
        if(!std::meta::has_identifier(current))
            break;
        result = string_concat(
            script_identifier_of(current),
            "::",
            result
        );
    }

    return std::define_static_string(result);
}

consteval std::string_view script_parameter_list_declaration_of_with_calling_convention(
    std::meta::info func, asbind20::detail::call_conv_type conv
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

consteval std::string_view script_return_type_declaration_of(std::meta::info func)
{
    std::string result;

    auto ann_of_handle = extract_last_annotation<as_handle_t>(func);
    auto ret_type = std::meta::return_type_of(func);
    result = script_type_declaration_of(
        ret_type, ann_of_handle.has_value()
    );

    if(is_ptrref_type(ret_type) &&
       ann_of_handle.has_value() && ann_of_handle->auto_handle)
    {
        result += '+';
    }

    return std::define_static_string(result);
}

consteval std::string_view script_function_declaration_of_with_calling_convention(
    std::meta::info func,
    asbind20::detail::call_conv_type conv,
    bool skip_func_name
)
{
    if(!std::meta::has_identifier(func))
        skip_func_name = true;

    std::string suffix;
    if(is_const_method_with_calling_convention(func, conv))
        suffix += "const";

    std::string_view func_identifer =
        skip_func_name ? "f" : script_identifier_of(func);

    std::string decl = string_concat(
        script_return_type_declaration_of(func),
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
    std::meta::info prop, bool prefer_handle
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
} // namespace asbind20::meta

#endif

#endif
