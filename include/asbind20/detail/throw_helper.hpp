/**
 * @file throw_helper.hpp
 * @author HenryAWE
 * @brief Helper for throwing exception
 */

#ifndef ASBIND20_DETAIL_THROW_HELPER
#define ASBIND20_DETAIL_THROW_HELPER

// IWYU pragma: begin_exports

#include <exception>
#include <utility>
#include "config.hpp"

// IWYU pragma: end_exports

namespace asbind20::detail
{
template <typename Exception, typename... Args>
[[noreturn]]
void throw_([[maybe_unused]] Args&&... args)
{
#ifndef ASBIND20_NO_EXCEPTIONS
    throw Exception(std::forward<Args>(args)...);
#else


    std::terminate();
#endif
}
} // namespace asbind20::detail

#endif
