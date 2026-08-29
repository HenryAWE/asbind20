#ifndef ASBIND20_META_REFLECTION_HPP
#define ASBIND20_META_REFLECTION_HPP

#include <cstdint>
#include "../util/strutil.hpp"
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
    consteval std::string_view calc_type_name(std::meta::info type_info)
    {
        // "^^std::int8_t" will cause compilation error, WHY?

        if(std::meta::is_same_type(type_info, ^^int8_t))
            return "int8";
        return std::meta::display_string_of(type_info);
    }

    template <std::meta::info func_info>
    constexpr std::string get_param_str()
    {
        constexpr static auto params =
            std::define_static_array(std::meta::parameters_of(func_info));

        std::string params_str;
        bool first = true;
        template for(constexpr auto param : params)
        {
            if(!first)
                params_str += ',';
            first = false;
            params_str += detail::calc_type_name(std::meta::type_of(param));
            if constexpr(std::meta::has_identifier(param))
            {
                params_str += ' ';
                params_str += std::meta::identifier_of(param);
            }
        }
        return params_str;
    }
} // namespace detail

#    if defined(__GNUC__) && !defined(__clang__)
#        pragma GCC diagnostic pop
#    endif

template <std::meta::info FuncInfo>
constexpr cstring_ref refl_function_sig()
{
    constexpr auto ret_t = std::meta::return_type_of(FuncInfo);

    return std::define_static_string(
        detail::calc_type_name(ret_t) +
        std::string(1, ' ') +
        std::meta::identifier_of(FuncInfo) +
        std::string(1, '(') +
        detail::get_param_str<FuncInfo>() +
        std::string(1, ')')
    );
}
} // namespace asbind20::meta

#else

// Namespace placeholder
namespace asbind20::meta
{}

#endif

#endif
