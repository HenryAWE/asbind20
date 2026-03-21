#ifndef ASBIND20_BIND_LISTENER_HPP
#define ASBIND20_BIND_LISTENER_HPP

#pragma once

#include <cassert>
#include <utility>
#include "../detail/include_as.hpp"

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
        {
            // TODO: fallback
        }
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
        {
            // TODO: fallback
        }
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
        {
            // TODO: fallback
        }
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
        {
            // TODO: fallback
        }
    }
};
} // namespace asbind20

#endif
