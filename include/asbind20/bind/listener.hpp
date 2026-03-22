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

    template <typename BindingGenerator>
    static void on_function(Listener& listener, BindingGenerator&& gen, int id)
    {
        constexpr bool has_func = requires() {
            listener.on_function(std::forward<BindingGenerator>(gen), id);
        };
        if constexpr(has_func)
            listener.on_function(std::forward<BindingGenerator>(gen), id);
        else
            default_fallback(id);
    }

    template <typename BindingGenerator>
    static void on_global_property(Listener& listener, BindingGenerator&& gen, int id)
    {
        constexpr bool has_func = requires() {
            listener.on_global_property(std::forward<BindingGenerator>(gen), id);
        };
        if constexpr(has_func)
            listener.on_global_property(std::forward<BindingGenerator>(gen), id);
        else
            default_fallback(id);
    }

    template <typename BindingGenerator>
    static void on_funcdef(Listener& listener, BindingGenerator&& gen, int id)
    {
        constexpr bool has_func = requires() {
            listener.on_funcdef(std::forward<BindingGenerator>(gen), id);
        };
        if constexpr(has_func)
            listener.on_funcdef(std::forward<BindingGenerator>(gen), id);
        else
            default_fallback(id);
    }

    template <typename BindingGenerator>
    static void on_typedef(Listener& listener, BindingGenerator&& gen, int id)
    {
        constexpr bool has_func = requires() {
            listener.on_typedef(std::forward<BindingGenerator>(gen), id);
        };
        if constexpr(has_func)
            listener.on_typedef(std::forward<BindingGenerator>(gen), id);
        else
            default_fallback(id);
    }

private:
    static void default_fallback(int val)
    {
        if(val >= 0)
            return;

        // Report error
        auto code = static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(val);
        ::asbind20::detail::throw_<std::system_error>(
            make_error_code(code)
        );
    }
};
} // namespace asbind20

#endif
