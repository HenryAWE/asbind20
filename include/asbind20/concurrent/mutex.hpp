#ifndef ASBIND20_CONCURRENT_MUTEX_HPP
#define ASBIND20_CONCURRENT_MUTEX_HPP

#pragma once

// IWYU pragma: begin_exports
#include <mutex>
#include <shared_mutex>
// IWYU pragma: end_exports
#include "../detail/include_as.hpp"

namespace asbind20
{
/**
 * @brief Wrapper for global lock of AngelScript library
 */
class script_lock_t
{
public:
    constexpr script_lock_t() = default;
    script_lock_t(const script_lock_t&) = delete;

    script_lock_t& operator=(const script_lock_t&) = delete;

    /**
     * @brief Locks the mutex, blocks if the mutex is not available
     */
    static void lock()
    {
        AS_NAMESPACE_QUALIFIER asAcquireExclusiveLock();
    }

    /**
     * @brief Unlocks the mutex
     */
    static void unlock()
    {
        AS_NAMESPACE_QUALIFIER asReleaseExclusiveLock();
    }

    /**
     * @brief Locks the mutex for shared ownership, blocks if the mutex is not available
     */
    static void lock_shared()
    {
        AS_NAMESPACE_QUALIFIER asAcquireSharedLock();
    }

    /**
     * @brief Unlocks the mutex (shared ownership)
     */
    static void unlock_shared()
    {
        AS_NAMESPACE_QUALIFIER asReleaseSharedLock();
    }
};

/**
 * @brief Global lock of AngelScript library
 */
inline constexpr script_lock_t script_lock = {};
} // namespace asbind20

#endif
