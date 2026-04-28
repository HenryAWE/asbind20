/**
 * @file err_handler.hpp
 * @author HenryAWE
 * @brief Helper for throwing exception
 */

#ifndef ASBIND20_DETAIL_ERR_HANDLER_HPP
#define ASBIND20_DETAIL_ERR_HANDLER_HPP

// IWYU pragma: begin_exports

#include <exception>
#include <utility>
#include "config.hpp"

// IWYU pragma: end_exports

namespace asbind20
{
/**
 * @brief Define the macro ASBIND20_CUSTOM_ON_EXCEPTION and provide your definition of this function
 *        for custom fallback when exception is disabled
 *
 * @tparam Exception The exception type
 * @tparam Args Arguments
 */
template <typename Exception, typename... Args>
void on_exception(Args&&... args);

namespace detail
{
    template <typename Exception, typename... Args>
    [[noreturn]]
    void throw_([[maybe_unused]] Args&&... args)
    {
#ifndef ASBIND20_NO_EXCEPTIONS
        throw Exception(std::forward<Args>(args)...);

#else

#    ifdef ASBIND20_CUSTOM_ON_EXCEPTION
        ::asbind20::on_exception<Exception>(std::forward<Args>(args)...);
#    endif

        std::terminate();
#endif
    }
} // namespace detail
} // namespace asbind20

#endif
