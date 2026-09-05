/**
 * @file invoke.hpp
 * @author HenryAWE
 * @brief Invocation tools
 */

#ifndef ASBIND20_INVOKE_HPP
#define ASBIND20_INVOKE_HPP

#pragma once

#include "fwd.hpp"
#include "utility.hpp"
#include "memory.hpp"

// IWYU pragma: begin_exports

#include "invoke/set_arg.hpp"
#include "invoke/invoke_result.hpp"
#include "invoke/script_func.hpp"
#include "invoke/compile_func.hpp"

// IWYU pragma: end_exports

namespace asbind20
{

/**
 * @brief Instantiate a script class using its default factory function
 *
 * @param ctx Script context
 * @param class_info Script class type information
 *
 * @return Instantiated script object, or empty object if failed
 *
 * @note This function requires the class to be default constructible
 */
[[nodiscard]]
inline script_object instantiate_class(
    context_pointer ctx,
    const_typeinfo_pointer class_info
)
{
    if(!class_info) [[unlikely]]
        return {};

    function_pointer factory = nullptr;
    if(AS_NAMESPACE_QUALIFIER asQWORD flags = class_info->GetFlags();
       flags & (AS_NAMESPACE_QUALIFIER asOBJ_SCRIPT_OBJECT))
    {
        factory = get_default_factory(class_info);
    }

    if(!factory) [[unlikely]]
        return {};

    auto result = script_invoke<script_object>(ctx, factory);
    return result.has_value() ? *result : script_object();
}

} // namespace asbind20

#endif
