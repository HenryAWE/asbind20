#ifndef ASBIND20_BIND_LISTENER_HPP
#define ASBIND20_BIND_LISTENER_HPP

#pragma once

#include <utility>
#include "../detail/throw_helper.hpp"
#include "../detail/include_as.hpp"
#include "../script_error.hpp"

namespace asbind20
{
/**
 * @brief Default listener. It does nothing.
 */
class default_listener
{};

template <typename Listener>
class listener_traits
{
public:
    using listener_type = Listener;

    listener_traits() = delete;

#define ASBIND20_IMPL_LISTENER_GENERAL_FUNC(sig, name)                  \
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

    ASBIND20_IMPL_LISTENER_GENERAL_FUNC(on_function, "function")
    ASBIND20_IMPL_LISTENER_GENERAL_FUNC(on_global_property, "global property")
    ASBIND20_IMPL_LISTENER_GENERAL_FUNC(on_funcdef, "funcdef")
    ASBIND20_IMPL_LISTENER_GENERAL_FUNC(on_typedef, "typedef")
    ASBIND20_IMPL_LISTENER_GENERAL_FUNC(on_method, "method")
    ASBIND20_IMPL_LISTENER_GENERAL_FUNC(on_enum, "enum")
    ASBIND20_IMPL_LISTENER_GENERAL_FUNC(on_class, "class")
    ASBIND20_IMPL_LISTENER_GENERAL_FUNC(on_interface, "interface")
    ASBIND20_IMPL_LISTENER_GENERAL_FUNC(on_enum_value, "enum value")

#undef ASBIND20_IMPL_LISTENER_GENERAL_FUNC

private:
    static void default_fallback(
        [[maybe_unused]] int val,
        [[maybe_unused]] const char* what
    )
    {
        if(val >= 0) [[likely]]
            return;

#ifndef ASBIND20_NO_THROW_ON_BAD_BINDING
        // Report error
        auto code = static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(val);
        ::asbind20::detail::throw_<std::system_error>(
            make_error_code(code), what
        );
#endif
    }
};
} // namespace asbind20

#endif
