#ifndef ASBIND20_UTIL_SCRIPT_REFL_HPP
#define ASBIND20_UTIL_SCRIPT_REFL_HPP

#include "strutil.hpp"
#include "../fwd.hpp"

namespace asbind20
{
struct script_func_param_info
{
    using flags_type = AS_NAMESPACE_QUALIFIER asDWORD;

    int type_id = AS_NAMESPACE_QUALIFIER asTYPEID_VOID;
    flags_type flags = 0;
    cstring_ref name{};
    cstring_ref default_arg{};
};

[[nodiscard]]
inline script_func_param_info get_func_param_info(
    function_reference func, arg_index_type idx
)
{
    script_func_param_info result;
    func.GetParam(
        idx, &result.type_id, &result.flags, &result.name, &result.default_arg
    );
    return result;
}

[[nodiscard]]
inline script_func_param_info get_func_param_info(
    function_pointer func, arg_index_type idx
)
{
    if(!func) [[unlikely]]
        return {};
    return get_func_param_info(*func, idx);
}

struct script_func_var_info
{
    cstring_ref name{};
    int type_id = AS_NAMESPACE_QUALIFIER asTYPEID_VOID;
};

[[nodiscard]]
inline script_func_var_info get_func_var_info(
    function_reference func, arg_index_type idx
)
{
    script_func_var_info result;
    func.GetVar(
        idx, &result.name, &result.type_id
    );
    return result;
}

struct script_global_var_info
{
    cstring_ref name{};
    cstring_ref name_space{};
    int type_id = AS_NAMESPACE_QUALIFIER asTYPEID_VOID;
    bool is_const = false;
};

[[nodiscard]]
inline script_global_var_info get_global_var_info(
    module_reference m, arg_index_type idx
)
{
    script_global_var_info result;
    m.GetGlobalVar(
        idx,
        &result.name,
        &result.name_space,
        &result.type_id,
        &result.is_const
    );
    return result;
}

[[nodiscard]]
inline script_global_var_info get_global_var_info(
    module_pointer m, arg_index_type idx
)
{
    if(!m) [[unlikely]]
        return {};
    return get_global_var_info(*m, idx);
}
} // namespace asbind20

#endif
