#ifndef ASBIND20_UTIL_CTXUTIL_HPP
#define ASBIND20_UTIL_CTXUTIL_HPP

#include "../detail/config.hpp"
#include "strutil.hpp"

namespace asbind20
{
/**
 * @brief Get current script context from a function called by script
 *
 * @return A pointer to the currently executing context, or null if no context is executing
 */
[[nodiscard]]
inline context_pointer current_context()
{
    return AS_NAMESPACE_QUALIFIER asGetActiveContext();
}

[[nodiscard]]
inline bool has_script_exception(const_context_reference ctx)
{
    return ctx.GetState() ==
           AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION;
}

[[nodiscard]]
inline bool has_script_exception(const_context_pointer ctx)
{
    if(!ctx) [[unlikely]]
        return false;

    return has_script_exception(*ctx);
}

/**
 * @brief Check if the current script context has exception
 */
[[nodiscard]]
inline bool has_script_exception()
{
    return has_script_exception(current_context());
}
} // namespace asbind20

#endif
