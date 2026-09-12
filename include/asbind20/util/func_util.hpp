#ifndef ASBIND20_UTIL_FUNC_UTIL_HPP
#define ASBIND20_UTIL_FUNC_UTIL_HPP

#include "../fwd.hpp"

namespace asbind20
{
/**
 * @brief Convert a (member) function pointer to script function
 */
template <typename Func>
internal_func_type to_asSFuncPtr(Func f)
{
    static_assert(
        std::is_member_function_pointer_v<Func> ||
            std::is_function_v<Func> ||
            std::is_function_v<std::remove_pointer_t<Func>>,
        "Requires function or member function"
    );

    // Reference: asFUNCTION and asMETHOD from the AngelScript interface
    if constexpr(std::is_member_function_pointer_v<Func>)
        return AS_NAMESPACE_QUALIFIER asSMethodPtr<sizeof(f)>::Convert(f);
    else
        return AS_NAMESPACE_QUALIFIER asFunctionPtr(f);
}
} // namespace asbind20

#endif
