#ifndef ASBIND20_META_ANNOTATION_HPP
#define ASBIND20_META_ANNOTATION_HPP

#include "refl_common.hpp"

#ifdef ASBIND20_HAS_LIB_REFLECTION


namespace asbind20
{
struct rename
{
    const char* name;

    explicit rename(std::nullptr_t) = delete;

    explicit consteval rename(const char* name_)
    {
        // We need to promote the string here,
        // otherwise we'll get error when using this str
        name = std::define_static_string(std::string_view(name_));
    }

    [[nodiscard]]
    constexpr std::string_view get() const
    {
        return name;
    }
};

struct default_arg
{
    const char* arg;

    explicit default_arg(std::nullptr_t) = delete;

    explicit consteval default_arg(const char* arg_)
    {
        // We need to promote the string here,
        // otherwise we'll get error when using this str
        arg = std::define_static_string(std::string_view(arg_));
    }

    [[nodiscard]]
    constexpr std::string_view get() const
    {
        return arg;
    }
};
} // namespace asbind20

#endif

#endif
