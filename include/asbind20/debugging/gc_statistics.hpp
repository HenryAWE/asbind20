/**
 * @file debugging/gc_statistics.hpp
 * @author HenryAWE
 * @brief GC statistics tool
 */

#ifndef ASBIND20_DEBUGGING_GC_STATISTICS_HPP
#define ASBIND20_DEBUGGING_GC_STATISTICS_HPP

#pragma once

#include <cassert>
#include "../detail/include_as.hpp"

namespace asbind20::debugging
{
/**
 * @brief GC statistics
 */
struct gc_statistics
{
    using value_type = AS_NAMESPACE_QUALIFIER asUINT;

    value_type current_size;
    value_type total_destroyed;
    value_type total_detected;
    value_type new_objects;
    value_type total_new_destroyed;

    bool operator==(const gc_statistics&) const noexcept = default;
};

/**
 * @brief Get the GC statistics
 *
 * @param engine Script engine. It cannot be nullptr.
 */
[[nodiscard]]
inline gc_statistics get_gc_statistics(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    assert(engine != nullptr);

    gc_statistics result{};
    engine->GetGCStatistics(
        &result.current_size,
        &result.total_destroyed,
        &result.total_detected,
        &result.new_objects,
        &result.total_new_destroyed
    );
    return result;
}
} // namespace asbind20::debugging

#endif
