/**
 * @file foreach.hpp
 * @author HenryAWE
 * @brief Helpers for foreach interfaces
 */

#ifndef ASBIND20_FOREACH_HPP
#define ASBIND20_FOREACH_HPP

#pragma once

#include <iterator>
#include "meta.hpp"
#include "bind.hpp" // IWYU pragma: exports

#ifndef ASBIND20_HAS_AS_FOREACH
#    warning Current AngelScript version does not support foreach loop
#endif

namespace asbind20
{
template <typename IteratorRegister, bool Const = true>
class foreach
{
public:
    using iterator = typename IteratorRegister::class_type;

    const IteratorRegister& iter;

    constexpr foreach(const IteratorRegister& it) noexcept
        : iter(it) {}

private:
    template <typename RegisterHelper>
    static void setup_foreach_controller(const std::string& it_name, RegisterHelper& helper)
    {
        using this_type = std::conditional_t<
            Const,
            const typename RegisterHelper::class_type,
            typename RegisterHelper::class_type>;

        helper.method(
            string_concat(it_name, Const ? " opForBegin()const" : " opForBegin()"),
            [](this_type& this_) -> iterator
            {
                if constexpr(Const)
                    return std::cbegin(this_);
                else
                    return std::begin(this_);
            }
        );
        helper.method(
            string_concat("bool opForEnd(const ", it_name, Const ? "&in)const" : "&in)"),
            [](this_type& this_, const iterator& it) -> bool
            {
                auto get_sentinel = [&]()
                {
                    if constexpr(Const)
                        return std::cend(this_);
                    else
                        return std::end(this_);
                };
                return it == get_sentinel();
            }
        );
        helper.method(
            string_concat(it_name, " opForNext(const ", it_name, Const ? "&in)const" : "&in)"),
            [](this_type& this_, const iterator& it) -> iterator
            {
                (void)this_;
                return std::next(it);
            }
        );
    }

public:
    template <typename RegisterHelper>
    void operator()(RegisterHelper& helper)
    {
        using this_type = std::conditional_t<
            Const,
            const typename RegisterHelper::class_type,
            typename RegisterHelper::class_type>;

        const std::string& it_name = iter.get_name();

        setup_foreach_controller(it_name, helper);

        using return_type = typename std::iterator_traits<iterator>::reference;

        helper.method(
            string_concat(name_of<return_type>(), " opForValue(const ", it_name, Const ? "&in)const" : "&in)"),
            [](this_type& this_, const iterator& it) -> return_type
            {
                (void)this_;
                return *it;
            }
        );
    }
};
} // namespace asbind20

#endif
