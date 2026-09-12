#ifndef ASBIND20_META_REFLECTION_HPP
#define ASBIND20_META_REFLECTION_HPP

#include <cstdint>
#include "../util/strutil.hpp"
#include "../utility.hpp"
#include "refl_common.hpp"
#include "../bind/calling_convention.hpp"
#include "annotation.hpp"

#ifdef ASBIND20_HAS_LIB_REFLECTION

#    if defined(__GNUC__) && !defined(__clang__)
#        pragma GCC diagnostic push
// False positive for template for
#        pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#    endif

namespace asbind20::meta
{
consteval std::meta::info remove_ptrref(std::meta::info type)
{
    return std::meta::remove_pointer(
        std::meta::remove_reference(type)
    );
}

namespace detail
{
    template <std::meta::info TypeInfo>
    consteval std::string_view calc_type_name()
    {
        constexpr auto type_info = std::meta::remove_cvref(TypeInfo);

        constexpr auto rename_ann =
           std::define_static_array(std::meta::annotations_of_with_type(type_info, ^^asbind20::rename));
        if constexpr(!rename_ann.empty())
        {
            return std::meta::extract<asbind20::rename>(
                       rename_ann.back()
            )
                .get();
        }

        // "^^std::int8_t" will cause compilation error,
        // because reflection has limitation on `using decl;`
        // Use the old "std::same_as" solution.
        using type = typename[:type_info:];

        if(std::same_as<type, std::int8_t>)
            return "int8";
        if(std::same_as<type, std::int16_t>)
            return "int16";
        if(std::same_as<type, std::int32_t>)
            return "int";
        if(std::same_as<type, std::int64_t>)
            return "int64";

        if(std::same_as<type, std::uint8_t>)
            return "uint8";
        if(std::same_as<type, std::uint16_t>)
            return "uint16";
        if(std::same_as<type, std::uint32_t>)
            return "uint";
        if(std::same_as<type, std::uint64_t>)
            return "uint64";

        std::string_view name = std::meta::display_string_of(type_info);
        if(auto pos = name.rfind("::"); pos != name.npos)
        {
            name.remove_prefix(pos + 2);
        }
        return name;
    }

    template <std::meta::info TypeInfo>
    consteval std::string calc_full_type_name(
        bool no_additional_ref_mod
    )
    {
        constexpr bool is_const = std::meta::is_const_type(
            std::meta::remove_reference(TypeInfo)
        );
        std::string result;
        if constexpr(is_const)
            result += "const ";
        result += calc_type_name<TypeInfo>();
        if constexpr(std::meta::is_reference_type(TypeInfo))
        {
            result += '&';
            if(!no_additional_ref_mod)
            {
                // TODO: Let user decide "inout" or "out" for mutable reference
                result += is_const ? "in" : "inout";
            }
        }

        return result;
    }

    template <std::meta::info FuncInfo>
    consteval std::span<const std::meta::info> params_of(
        bool no_first, bool no_last
    )
    {
        constexpr auto params = std::define_static_array(
            std::meta::parameters_of(FuncInfo)
        );
        if(no_first && !params.empty())
            return std::span(params.begin() + 1, params.end());
        if(no_last && !params.empty())
            return std::span(params.begin(), params.end() - 1);
        return params;
    }

    template <
        std::meta::info FuncInfo,
        bool NoFirst,
        bool NoLast,
        bool ParseDefaultArg>
    constexpr std::string calc_param_list_str()
    {
        constexpr static auto params = params_of<FuncInfo>(
            NoFirst, NoLast
        );

        std::string params_str;
        params_str += '(';

        bool first = true;
        template for(constexpr auto param : params)
        {
            if(!first)
                params_str += ',';
            first = false;
            params_str +=
                detail::calc_full_type_name<std::meta::type_of(param)>(false);
            if constexpr(std::meta::has_identifier(param))
            {
                params_str += ' ';
                params_str += std::meta::identifier_of(param);
            }

            if(!ParseDefaultArg)
                continue;
            constexpr static auto arg_ann = std::define_static_array(
                std::meta::annotations_of_with_type(param, ^^asbind20::default_arg)
            );
            if constexpr(!arg_ann.empty())
            {
                params_str += '=';
                params_str += std::meta::extract<asbind20::default_arg>(
                              arg_ann.back()
                )
                              .get();
            }
        }

        params_str += ')';
        return params_str;
    }

    template <std::meta::info FuncInfo>
    consteval bool is_const_method(bool check_first, bool check_last)
    {
        constexpr auto params = std::define_static_array(
            std::meta::parameters_of(FuncInfo)
        );

        if(params.empty())
            return false;

        if(check_first)
        {
            return std::meta::is_const(
                remove_ptrref(std::meta::type_of(params.front()))
            );
        }
        if(check_last)
        {
            return std::meta::is_const(
                remove_ptrref(std::meta::type_of(params.back()))
            );
        }
        return false;
    }
} // namespace detail

#    if defined(__GNUC__) && !defined(__clang__)
#        pragma GCC diagnostic pop
#    endif

template <
    std::meta::info FuncInfo,
    bool NoFirst = false,
    bool NoLast = false,
    bool ParseDefaultArg = false>
consteval cstring_ref refl_function_sig(
    bool skip_mem_fn_const = false,
    bool skip_func_name = false,
    bool force_const = false
)
{
    constexpr auto ret_t = std::meta::return_type_of(FuncInfo);

    std::string_view func_identifier =
        skip_func_name ? "f" : std::meta::identifier_of(FuncInfo);

    std::string suffix;
    if(force_const)
        suffix = "const";
    // Constant member functions
    else if(!skip_mem_fn_const &&
            std::meta::is_class_member(FuncInfo) &&
            std::meta::is_const(FuncInfo))
    {
        suffix += "const";
    }

    return std::define_static_string(
        string_concat(
            detail::calc_full_type_name<ret_t>(true),
            ' ',
            func_identifier,
            detail::calc_param_list_str<FuncInfo, NoFirst, NoLast, ParseDefaultArg>(),
            suffix
        )
    );
}

template <std::meta::info PropInfo>
constexpr cstring_ref refl_property_decl()
{
    constexpr auto type = std::meta::type_of(PropInfo);

    return std::define_static_string(
        string_concat(
            detail::calc_full_type_name<type>(true),
            ' ',
            std::meta::identifier_of(PropInfo)
        )
    );
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
        constexpr bool skip_mem_fn_const =
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL;
        constexpr bool no_first =
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST;
        constexpr bool no_last =
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST ||
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST;
        return refl_function_sig<Function, no_first, no_last, true>(
            skip_mem_fn_const,
            false,
            meta::detail::is_const_method<Function>(no_first, no_last)
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

    static constexpr cstring_ref get_decl() noexcept
    {
        return refl_property_decl<Property>();
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
            std::meta::identifier_of(Enumerator)
        );
    }

    static constexpr compat::script_enum_value_type get_val()
    {
        return static_cast<compat::script_enum_value_type>([:Enumerator:]);
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
