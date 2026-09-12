#ifndef ASBIND20_UTIL_SCRIPT_REFL_HPP
#define ASBIND20_UTIL_SCRIPT_REFL_HPP

#include <utility>
#include "strutil.hpp"
#include "../fwd.hpp"
#include "../util/script_result.hpp"

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

/**
 * @brief Gets the information of the parameter at the given index
 *
 * @param func The script function
 * @param idx The index of the parameter
 * @return The parameter information, or an error code if the index is out of range
 */
[[nodiscard]]
inline auto get_func_param_info(
    function_reference func, arg_index_type idx
)
    -> script_result<script_func_param_info>
{
    script_func_param_info result;
    int r = func.GetParam(
        idx, &result.type_id, &result.flags, &result.name, &result.default_arg
    );
    if(r < 0) [[unlikely]]
        return {bad_script_result, r};

    return script_result{std::move(result), r};
}

/**
 * @brief Gets the information of the parameter at the given index
 *
 * @param func The script function, which can be `nullptr`
 * @param idx The index of the parameter
 * @return The parameter information, or an error code if `func` is `nullptr`
 *         or the index is out of range
 */
[[nodiscard]]
inline auto get_func_param_info(
    function_pointer func, arg_index_type idx
)
    -> script_result<script_func_param_info>
{
    if(!func) [[unlikely]]
        return {bad_script_result, AS_NAMESPACE_QUALIFIER asINVALID_ARG};

    return get_func_param_info(*func, idx);
}

struct script_func_var_info
{
    cstring_ref name{};
    int type_id = AS_NAMESPACE_QUALIFIER asTYPEID_VOID;
};

/**
 * @brief Gets the information of the local variable at the given index
 *
 * @note The parameters are stored in the variable list as well
 *
 * @param func The script function
 * @param idx The index of the variable
 * @return The variable information, or an error code if the index is out of range
 *         or the function has no script data
 */
[[nodiscard]]
inline auto get_func_var_info(
    function_reference func, arg_index_type idx
)
    -> script_result<script_func_var_info>
{
    script_func_var_info result;
    int r = func.GetVar(
        idx, &result.name, &result.type_id
    );
    if(r < 0) [[unlikely]]
        return {bad_script_result, r};

    return script_result{std::move(result), r};
}

/**
 * @brief Gets the information of the local variable at the given index
 *
 * @note The parameters are stored in the variable list as well
 *
 * @param func The script function, which can be `nullptr`
 * @param idx The index of the variable
 * @return The variable information, or an error code if `func` is `nullptr`,
 *         the index is out of range, or the function has no script data
 */
[[nodiscard]]
inline auto get_func_var_info(
    function_pointer func, arg_index_type idx
)
    -> script_result<script_func_var_info>
{
    if(!func) [[unlikely]]
        return {bad_script_result, AS_NAMESPACE_QUALIFIER asINVALID_ARG};

    return get_func_var_info(*func, idx);
}

struct script_global_var_info
{
    cstring_ref name{};
    cstring_ref name_space{};
    int type_id = AS_NAMESPACE_QUALIFIER asTYPEID_VOID;
    bool is_const = false;
};

/**
 * @brief Gets the information of the global variable at the given index
 *
 * @param m The script module
 * @param idx The index of the global variable
 * @return The global variable information, or an error code if the index is out of range
 */
[[nodiscard]]
inline auto get_global_var_info(
    module_reference m, arg_index_type idx
)
    -> script_result<script_global_var_info>
{
    script_global_var_info result;
    int r = m.GetGlobalVar(
        idx,
        &result.name,
        &result.name_space,
        &result.type_id,
        &result.is_const
    );
    if(r < 0) [[unlikely]]
        return {bad_script_result, r};

    return script_result{std::move(result), r};
}

/**
 * @brief Gets the information of the global variable at the given index
 *
 * @param m The script module, which can be `nullptr`
 * @param idx The index of the global variable
 * @return The global variable information, or an error code if `m` is `nullptr`
 *         or the index is out of range
 */
[[nodiscard]]
inline auto get_global_var_info(
    module_pointer m, arg_index_type idx
)
    -> script_result<script_global_var_info>
{
    if(!m) [[unlikely]]
        return {bad_script_result, AS_NAMESPACE_QUALIFIER asINVALID_ARG};

    return get_global_var_info(*m, idx);
}
} // namespace asbind20

#endif
