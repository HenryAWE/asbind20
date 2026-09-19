#ifndef ASBIND20_META_ANNOTATION_HPP
#define ASBIND20_META_ANNOTATION_HPP

#include <stdexcept>
#include "refl_common.hpp"

namespace asbind20::inline annotations
{
struct out_ref_t
{};

/**
 * @brief Mark a mutable reference / pointer parameter as output
 */
inline constexpr out_ref_t out_ref{};

struct as_handle_t
{
    const bool auto_handle = false;

    consteval as_handle_t() = default;

    consteval as_handle_t operator+() const
    {
        return as_handle_t{true};
    }

private:
    consteval as_handle_t(bool auto_)
        : auto_handle(auto_) {}
};

/**
 * @brief Mark a reference / pointer parameter as AngelScript handle instead of reference
 */
inline constexpr as_handle_t as_handle{};

struct rename;

struct default_arg;
} // namespace asbind20::inline annotations

#ifdef ASBIND20_HAS_LIB_REFLECTION

namespace asbind20::inline annotations
{
struct annotation_with_name
{
    const char* const name;

    explicit consteval annotation_with_name(const char* name_)
        // We need to promote the string to static storage here,
        // otherwise we'll get error later when using this string at compile-time
        : name(std::define_static_string(std::string_view(name_)))
    {}

    [[nodiscard]]
    constexpr std::string_view get() const
    {
        return name;
    }
};

struct rename : annotation_with_name
{
    explicit consteval rename(const char* name)
        : annotation_with_name(name) {}
};

struct default_arg : annotation_with_name
{
    explicit default_arg(std::nullptr_t) = delete;

    explicit consteval default_arg(const char* name)
        : annotation_with_name(name) {}
};
} // namespace asbind20::inline annotations

#endif

#endif
