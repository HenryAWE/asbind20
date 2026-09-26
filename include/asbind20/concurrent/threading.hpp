/**
 * @file concurrent/threading.hpp
 * @author HenryAWE
 * @brief Tools for multithreading
 */

#ifndef ASBIND20_CONCURRENT_THREADING_HPP
#define ASBIND20_CONCURRENT_THREADING_HPP

#pragma once

#include <future>
#include <thread>
#include "mutex.hpp" // IWYU pragma: export
#include "../detail/include_as.hpp"
#include "../script_error.hpp"

namespace asbind20::concurrent
{
struct thread_cleaner
{
    thread_cleaner() = default;
    thread_cleaner(const thread_cleaner&) = delete;

    thread_cleaner& operator=(const thread_cleaner&) = delete;

    ~thread_cleaner() noexcept(false)
    {
        int r = AS_NAMESPACE_QUALIFIER asThreadCleanup();
        if(r < 0) [[unlikely]]
        {
            asbind20::detail::throw_<std::system_error>(
                make_error_code(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r)),
                "asThreadCleanup() failed"
            );
        }
    }
};

/**
 * @brief Mark this thread needs to clean up AngelScript data before terminating.
 *
 * @note Remember to call this function **in any thread other than main thread** to prevent memory leak.
 *
 * @details It's safe to call this function for the second time in the same thread.
 */
inline void auto_thread_cleanup() noexcept
{
    static thread_local thread_cleaner helper{};
}

/**
 * @brief Call this function in the @b main thread to prepare for multithreading
 *
 * @warning Call this function @b before any script engine is created,
 *          and make sure global variables don't contain any script engine that may be released
 *          after `asUnprepareMultithread` being called.
 */
inline void prepare_multithread(
    AS_NAMESPACE_QUALIFIER asIThreadManager* external_mgr = nullptr
)
{
    struct helper_t
    {
        helper_t(AS_NAMESPACE_QUALIFIER asIThreadManager* external_mgr)
        {
            int r = AS_NAMESPACE_QUALIFIER asPrepareMultithread(external_mgr);
            if(r < 0) [[unlikely]]
            {
                detail::throw_<std::system_error>(
                    make_error_code(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r)),
                    "asPrepareMultithread() failed"
                );
            }
        }

        ~helper_t()
        {
            AS_NAMESPACE_QUALIFIER asUnprepareMultithread();
        }
    };

    static helper_t helper{external_mgr};
}

template <typename F, typename... Args>
auto async(std::launch policy, F&& f, Args&&... args)
{
    return std::async(
        policy,
        [f = std::forward<F>(f)]<typename... Ts>(Ts&&... args)
        {
            auto_thread_cleanup();
            return std::invoke(std::move(f), std::forward<Ts>(args)...);
        },
        std::forward<Args>(args)...
    );
}

template <typename F, typename... Args>
auto async(F&& f, Args&&... args)
{
    return concurrent::async(
        std::launch::async | std::launch::deferred,
        std::forward<F>(f),
        std::forward<Args>(args)...
    );
}
} // namespace asbind20::concurrent

#endif
