#ifndef ASBIND20_META_REFLECTION_HPP
#define ASBIND20_META_REFLECTION_HPP

#include <cstdint>
#include "../util/strutil.hpp"
#include "../utility.hpp"
#if ASBIND20_HAS_LIB_REFLECTION
#    include <meta>

#    if defined(__GNUC__) && !defined(__clang__)
#        pragma GCC diagnostic push
// False positive for template for
#        pragma GCC diagnostic ignored "-Wunused-but-set-variable="
#    endif

namespace asbind20::meta
{
namespace detail
{
    template <std::meta::info TypeInfo>
    consteval std::string_view calc_type_name()
    {
        constexpr auto type_info = std::meta::remove_cvref(TypeInfo);

        // "^^std::int8_t" will cause compilation error, WHY?
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

        return std::meta::display_string_of(type_info);
    }

    template <std::meta::info TypeInfo>
    consteval std::string calc_full_type_name(
        bool is_return
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
            if(!is_return)
            {
                // TODO: Let user decide "inout" or "out" for mutable reference
                result += is_const ? "in" : "inout";
            }
        }

        return result;
    }

    template <std::meta::info func_info>
    constexpr std::string calc_param_list_str()
    {
        constexpr static auto params =
            std::define_static_array(std::meta::parameters_of(func_info));

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
        }

        params_str += ')';
        return params_str;
    }
} // namespace detail

#    if defined(__GNUC__) && !defined(__clang__)
#        pragma GCC diagnostic pop
#    endif

template <std::meta::info FuncInfo>
consteval cstring_ref refl_function_sig(bool skip_func_name = false)
{
    constexpr auto ret_t = std::meta::return_type_of(FuncInfo);

    std::string_view func_identifier =
        skip_func_name ? "f" : std::meta::identifier_of(FuncInfo);

    return std::define_static_string(
        string_concat(
            detail::calc_full_type_name<ret_t>(true),
            ' ',
            func_identifier,
            detail::calc_param_list_str<FuncInfo>()
        )
    );
}

template <std::meta::info Function>
struct function_refl_proxy
{
    constexpr function_refl_proxy() = default;

    static constexpr cstring_ref get_decl() noexcept
    {
        return refl_function_sig<Function>();
    }

    static constexpr auto get_func()
    {
        return &[:Function:];
    }
};

template <std::meta::info Info>
constexpr  auto get_proxy()
{
    return function_refl_proxy<Info>{};
}
} // namespace asbind20::meta

namespace asbind20
{
template <std::meta::info FuncInfo>
consteval auto reflect()
{
    //return std::meta::reflect_function<T>(f);
    return meta::get_proxy<FuncInfo>();
}
}

#else

// Namespace placeholder
namespace asbind20::meta
{}

#endif

#endif
