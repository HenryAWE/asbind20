#ifndef ASBIND20_BIND_LISTENER_HPP
#define ASBIND20_BIND_LISTENER_HPP

#pragma once

#include <utility>
#include "../detail/throw_helper.hpp"
#include "../detail/include_as.hpp"
#include "../script_error.hpp"

namespace asbind20
{
class default_listener
{
public:
};

template <typename Listener>
class listener_traits
{
public:
    using listener_type = Listener;

    listener_traits() = delete;

#define ASBIND20_LISTENER_GENERAL_FUNC_IMPL(sig, name)                  \
    template <typename BindingGenerator>                                \
    static void sig(Listener& listener, BindingGenerator&& gen, int id) \
    {                                                                   \
        constexpr bool has_func = requires() {                          \
            listener.sig(std::forward<BindingGenerator>(gen), id);      \
        };                                                              \
        if constexpr(has_func)                                          \
            listener.sig(std::forward<BindingGenerator>(gen), id);      \
        else                                                            \
            default_fallback(id, "bad " name);                          \
    }

    ASBIND20_LISTENER_GENERAL_FUNC_IMPL(on_function, "function")
    ASBIND20_LISTENER_GENERAL_FUNC_IMPL(on_global_property, "global property")
    ASBIND20_LISTENER_GENERAL_FUNC_IMPL(on_funcdef, "funcdef")
    ASBIND20_LISTENER_GENERAL_FUNC_IMPL(on_typedef, "typedef")

#undef ASBIND20_LISTENER_GENERAL_FUNC_IMPL

private:
    static void default_fallback(int val, const char* what)
    {
        if(val >= 0) [[likely]]
            return;

        // Report error
        auto code = static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(val);
        ::asbind20::detail::throw_<std::system_error>(
            make_error_code(code), what
        );
    }
};
} // namespace asbind20

#endif
