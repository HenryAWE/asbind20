#ifndef ASBIND20_FUNCTION_TOOLS_HPP
#define ASBIND20_FUNCTION_TOOLS_HPP

#include <functional>
#include "../detail/calling_convention.hpp"
#include "genfunc.hpp"
#include "../meta.hpp"

namespace asbind20::fn_tools
{
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
} // namespace asbind20::fn_tools

#endif
