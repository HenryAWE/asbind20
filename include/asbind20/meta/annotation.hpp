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

struct rename_t;

struct default_arg_t;
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

struct rename_t : annotation_with_name
{
    using annotation_with_name::annotation_with_name;
};

consteval rename_t rename(const char* name)
{
    return rename_t(name);
}

consteval rename_t rename(const std::string& name)
{
    return rename_t(name.c_str());
}

struct default_arg_t : annotation_with_name
{
    using annotation_with_name::annotation_with_name;
};

consteval default_arg_t default_arg(const char* name)
{
    return default_arg_t(name);
}

consteval default_arg_t default_arg(const std::string& name)
{
    return default_arg_t(name.c_str());
}
} // namespace asbind20::inline annotations

#endif

#endif
