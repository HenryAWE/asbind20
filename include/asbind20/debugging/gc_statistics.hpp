/**
 * @file debugging/gc_statistics.hpp
 * @author HenryAWE
 * @brief GC statistics tool
 */

#ifndef ASBIND20_DEBUGGING_GC_STATISTICS_HPP
#define ASBIND20_DEBUGGING_GC_STATISTICS_HPP

#pragma once

#include <cassert>
#include <utility>
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

    template <std::size_t I>
    friend value_type get(
        const gc_statistics& stats
    ) noexcept
    {
        static_assert(I < 5, "Index out of range");
        if constexpr(I == 0)
            return stats.current_size;
        else if constexpr(I == 1)
            return stats.total_destroyed;
        else if constexpr(I == 2)
            return stats.total_detected;
        else if constexpr(I == 3)
            return stats.new_objects;
        else // I == 4
            return stats.total_new_destroyed;
    }
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

    gc_statistics result;
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

template <>
struct std::tuple_size<asbind20::debugging::gc_statistics> :
    public std::integral_constant<std::size_t, 5>
{};

template <std::size_t Index>
requires(Index < 5)
struct std::tuple_element<Index, asbind20::debugging::gc_statistics>
{
    using type = asbind20::debugging::gc_statistics::value_type;
};

#endif
