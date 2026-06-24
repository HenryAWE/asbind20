#ifndef ASBIND20_FUNCTION_TOOLS_HPP
#define ASBIND20_FUNCTION_TOOLS_HPP

#include <functional>
#include "../detail/calling_convention.hpp"
#include "genfunc.hpp"
#include "../meta.hpp"

namespace asbind20::fn_tools
{
template <typename ArgsTuple, typename Fn>
decltype(auto) apply_generic(Fn&& fn, generic_pointer gen)
{
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) -> decltype(auto)
    {
        return std::invoke(
            std::forward<Fn>(fn),
            get_generic_arg<std::tuple_element_t<Is, ArgsTuple>>(
                gen, static_cast<detail::gen_idx_t>(Is)
            )...
        );
    }(std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{});
}

namespace detail
{
    using namespace asbind20::detail;

    template <auto Fn>
    class ctor_fn_wrapper
    {
        using traits = function_traits<std::decay_t<decltype(Fn)>>;

        template <typename Class, typename... Args>
        static void do_construct(void* mem, Args&&... args)
        {
            new(mem) Class(std::invoke(Fn, std::forward<Args>(args)...));
        }

        template <typename Class, typename... Args>
        static void impl_cdecl_objfirst(void* mem, Args... args)
        {
            do_construct<Class>(mem, args...);
        }

        template <typename Class, typename... Args>
        static void impl_generic(generic_pointer gen)
        {
            using args_tuple = std::tuple<Args...>;
            [&]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                void* mem = gen->GetObject();
                do_construct<Class>(
                    mem,
                    get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                        gen, static_cast<gen_idx_t>(Is)
                    )...
                );
            }(std::make_index_sequence<sizeof...(Args)>{});
        }

    public:
        template <
            typename Class,
            AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate() noexcept
        {
            using args_tuple = typename traits::args_tuple;

            return []<std::size_t... Is>(std::index_sequence<Is...>)
            {
                if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                    return &impl_generic<Class, std::tuple_element_t<Is, args_tuple>...>;
                else
                    return &impl_cdecl_objfirst<Class, std::tuple_element_t<Is, args_tuple>...>;
            }(std::make_index_sequence<traits::arg_count_v>{});
        }
    };
} // namespace detail

template <auto Fn>
constexpr auto as_ctor_fn() noexcept
{
    return detail::ctor_fn_wrapper<Fn>{};
}

namespace detail
{
    template <typename Return, auto Fn>
    class map_ret_wrapper;

    template <typename Return, auto Fn>
    requires(!std::is_member_function_pointer_v<decltype(Fn)>)
    class map_ret_wrapper<Return, Fn>
    {
        using traits = function_traits<std::decay_t<decltype(Fn)>>;

        template <typename... Args>
        static Return do_invoke(Args&&... args)
        {
            return Return(std::invoke(Fn, std::forward<Args>(args)...));
        }

        template <typename... Args>
        static Return impl_native(Args... args)
        {
            return do_invoke(args...);
        }

        template <typename ArgsTuple>
        static void impl_generic(generic_pointer gen)
        {
            set_generic_return<Return>(
                gen, static_cast<Return>(apply_generic<ArgsTuple>(Fn, gen))
            );
        }

    public:
        using wrapped_function_tag = void;

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate() noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return &impl_generic<typename traits::args_tuple>;
            }
            else
            {
                using args_tuple = typename traits::args_tuple;
                return []<std::size_t... Is>(std::index_sequence<Is...>)
                {
                    return &impl_native<std::tuple_element_t<Is, args_tuple>...>;
                }(std::make_index_sequence<traits::arg_count_v>{});
            }
        }
    };

    template <typename Return, auto Fn>
    requires(std::is_member_function_pointer_v<decltype(Fn)>)
    class map_ret_wrapper<Return, Fn>
    {
        using traits = function_traits<std::decay_t<decltype(Fn)>>;
        using class_type = typename traits::class_type;

        template <typename... Args>
        static Return do_invoke(class_type* obj, Args&&... args)
        {
            return Return(std::invoke(Fn, obj, std::forward<Args>(args)...));
        }

        // Native: asCALL_CDECL_OBJFIRST — object pointer is the first argument
        template <typename... Args>
        static Return impl_objfirst(void* obj, Args... args)
        {
            return do_invoke(static_cast<class_type*>(obj), args...);
        }

        // Native: asCALL_CDECL_OBJLAST — object pointer is the last argument
        template <typename... Args>
        static Return impl_objlast(Args... args, void* obj)
        {
            return do_invoke(static_cast<class_type*>(obj), args...);
        }

        // Generic: asCALL_GENERIC — extract everything from generic_pointer
        template <typename... Args>
        static void impl_generic(generic_pointer gen)
        {
            using args_tuple = std::tuple<Args...>;
            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                set_generic_return_by<Return>(
                    gen,
                    Fn,
                    get_generic_object<class_type*>(gen),
                    get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                        gen, static_cast<gen_idx_t>(Is)
                    )...
                );
            }(std::make_index_sequence<sizeof...(Args)>{});
        }

    public:
        using wrapped_function_tag = void;

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate() noexcept
        {
            using args_tuple = typename traits::args_tuple;

            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return []<std::size_t... Is>(std::index_sequence<Is...>)
                {
                    return &impl_generic<std::tuple_element_t<Is, args_tuple>...>;
                }(std::make_index_sequence<traits::arg_count_v>{});
            }
            else if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
            {
                return []<std::size_t... Is>(std::index_sequence<Is...>)
                {
                    return &impl_objlast<std::tuple_element_t<Is, args_tuple>...>;
                }(std::make_index_sequence<traits::arg_count_v>{});
            }
            else // asCALL_CDECL_OBJFIRST (default native convention)
            {
                return []<std::size_t... Is>(std::index_sequence<Is...>)
                {
                    return &impl_objfirst<std::tuple_element_t<Is, args_tuple>...>;
                }(std::make_index_sequence<traits::arg_count_v>{});
            }
        }
    };
} // namespace detail

template <typename T>
concept wrapped_function = requires()
{
    typename T::wrapped_function_tag;
};

template <typename Return, auto Fn>
constexpr auto map_ret(fp_wrapper<Fn> = {})
{
    return detail::map_ret_wrapper<Return, Fn>{};
}
} // namespace asbind20::fn_tools

#endif
