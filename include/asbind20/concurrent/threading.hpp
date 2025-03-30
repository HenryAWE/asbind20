#ifndef ASBIND20_CONCURRENT_THREADING_HPP
#define ASBIND20_CONCURRENT_THREADING_HPP

#include "../detail/include_as.hpp"

namespace asbind20::concurrent
{
/**
 * @brief Mark this thread needs to clean up AngelScript data before terminating.
 *
 * @note Remember to call this function **in any thread other than main thread** to prevent memory leak.
 *
 * @details It's safe to call this function for the second time in the same thread.
 */
inline void auto_thread_cleanup() noexcept
{
    struct helper_t
    {
        ~helper_t()
        {
            AS_NAMESPACE_QUALIFIER asThreadCleanup();
        }
    };

    static thread_local helper_t helper{};
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
            AS_NAMESPACE_QUALIFIER asPrepareMultithread(external_mgr);
        }

        ~helper_t()
        {
            AS_NAMESPACE_QUALIFIER asUnprepareMultithread();
        }
    };

    static helper_t helper{external_mgr};
}
} // namespace asbind20::concurrent

#endif
