/**
 * @file debugging/gc_statistics.hpp
 * @author HenryAWE
 * @brief GC statistics tool
 */

#ifndef ASBIND20_DEBUGGING_GC_STATISTICS_HPP
#define ASBIND20_DEBUGGING_GC_STATISTICS_HPP

#pragma once

#include <tuple>
#include <string>
#include <string_view>
#include "../detail/config.hpp"
#include "../fwd.hpp"
#include "../utility.hpp"

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

    constexpr bool operator==(const gc_statistics&) const noexcept = default;
    constexpr std::strong_ordering operator<=>(const gc_statistics&) const noexcept = default;

    template <std::size_t I>
    friend constexpr value_type get(
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

    using tuple_type = std::tuple<
        value_type,
        value_type,
        value_type,
        value_type,
        value_type>;

    constexpr value_type operator[](std::size_t index) const noexcept
    {
        ASBIND20_ASSERT(index < 5);
        switch(index)
        {
        case 0:
            return current_size;
        case 1:
            return total_destroyed;
        case 2:
            return total_detected;
        case 3:
            return new_objects;
        case 4:
            return total_new_destroyed;
        [[unlikely]] default:
            return 0;
        }
    }

    explicit operator tuple_type() const noexcept
    {
        return {
            current_size,
            total_destroyed,
            total_detected,
            new_objects,
            total_new_destroyed
        };
    }

    [[nodiscard]]
    std::string description(std::string_view sep = "; ") const
    {
        using namespace std::string_view_literals;
        using std::to_string;

        return string_concat(
            "current size: "sv,
            to_string(current_size),
            sep,
            "total destroyed: "sv,
            to_string(total_destroyed),
            sep,
            "total detected: "sv,
            to_string(total_detected),
            sep,
            "new objects: "sv,
            to_string(new_objects),
            sep,
            "total new destroyed: "sv,
            to_string(total_new_destroyed)
        );
    }
};

/**
 * @brief Get the GC statistics
 *
 * @param engine Script engine.
 */
[[nodiscard]]
inline gc_statistics get_gc_statistics(
    const_engine_reference engine
)
{
    gc_statistics result;
    engine.GetGCStatistics(
        &result.current_size,
        &result.total_destroyed,
        &result.total_detected,
        &result.new_objects,
        &result.total_new_destroyed
    );
    return result;
}

/**
 * @brief Get the GC statistics
 *
 * @param engine Script engine.
 */
[[nodiscard]]
inline gc_statistics get_gc_statistics(
    const_engine_pointer engine
)
{
    if(!engine) [[unlikely]]
        return {};

    return get_gc_statistics(*engine);
}
} // namespace asbind20::debugging

template <>
struct std::tuple_size<asbind20::debugging::gc_statistics> :
    std::integral_constant<std::size_t, 5>
{};

template <std::size_t Index>
requires(Index < 5)
struct std::tuple_element<Index, asbind20::debugging::gc_statistics>
{
    using type = asbind20::debugging::gc_statistics::value_type;
};

#endif
