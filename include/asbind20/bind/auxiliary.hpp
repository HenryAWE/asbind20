/**
 * @file bind/auxiliary.hpp
 * @author HenryAWE
 * @brief Auxiliary facility for binding generator
 */

#ifndef ASBIND20_BIND_AUXILIARY_HPP
#define ASBIND20_BIND_AUXILIARY_HPP

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <bit>
#include <type_traits>
#include "../fwd.hpp"

namespace asbind20
{
template <typename T>
class auxiliary_wrapper
{
public:
    auxiliary_wrapper() = delete;
    constexpr auxiliary_wrapper(const auxiliary_wrapper&) noexcept = default;

    auxiliary_wrapper& operator=(const auxiliary_wrapper&) = delete;

    explicit constexpr auxiliary_wrapper(const T* aux) noexcept
        : m_aux(const_cast<T*>(aux)) {}

    [[nodiscard]]
    void* get_address() const noexcept
    {
        return m_aux;
    }

private:
    T* m_aux;
};

// Default to void
auxiliary_wrapper(std::nullptr_t) -> auxiliary_wrapper<void>;

template <typename T>
[[nodiscard]]
constexpr auto auxiliary(T& aux) noexcept
{
    using type = std::remove_cv_t<T>;
    return auxiliary_wrapper<type>(std::addressof(aux));
}

template <typename T>
[[nodiscard]]
constexpr auto auxiliary(T* aux) noexcept
{
    using type = std::remove_cv_t<T>;
    return auxiliary_wrapper<type>(aux);
}

[[nodiscard]]
constexpr auxiliary_wrapper<void> auxiliary(std::nullptr_t) noexcept
{
    return auxiliary_wrapper<void>(nullptr);
}

struct this_type_t
{};

/**
 * @brief Tag indicating current type. Its exact meaning depends on context.
 */
inline constexpr this_type_t this_type{};

template <>
class auxiliary_wrapper<this_type_t>
{
public:
    auxiliary_wrapper() = delete;
    constexpr auxiliary_wrapper(const auxiliary_wrapper&) noexcept = default;

    auxiliary_wrapper& operator=(const auxiliary_wrapper&) = delete;

    explicit constexpr auxiliary_wrapper(this_type_t) noexcept {};
};

[[nodiscard]]
constexpr auxiliary_wrapper<this_type_t> auxiliary(this_type_t) noexcept
{
    return auxiliary_wrapper<this_type_t>(this_type);
}

// R-value reference will create dangling reference
template <typename T>
constexpr auto auxiliary(T&& aux)
    -> auxiliary_wrapper<std::remove_cv_t<T>> = delete;

/**
 * @brief Store a pointer-sized integer value as auxiliary object
 *
 * @note DO NOT use this unless you know what you are doing!
 *
 * @warning Only use this with the @b generic calling convention!
 *
 * @param val Integer value
 */
[[nodiscard]]
constexpr auxiliary_wrapper<void> aux_value(std::intptr_t val) noexcept
{
    return auxiliary_wrapper<void>(std::bit_cast<void*>(val));
}

namespace detail
{
    // Helper for retrieving actual auxiliary type from the helper
    template <typename Auxiliary>
    struct auxiliary_traits
    {
        using value_type = Auxiliary;
    };

    template <>
    struct auxiliary_traits<this_type_t>
    {
        using value_type = AS_NAMESPACE_QUALIFIER asITypeInfo;
    };
} // namespace detail

} // namespace asbind20

#endif
