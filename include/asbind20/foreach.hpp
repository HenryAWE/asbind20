/**
 * @file foreach.hpp
 * @author HenryAWE
 * @brief Helpers for foreach interfaces
 */

#ifndef ASBIND20_FOREACH_HPP
#define ASBIND20_FOREACH_HPP

#pragma once

#include "meta.hpp"
#include "bind.hpp" // IWYU pragma: exports

#ifndef ASBIND20_HAS_AS_FOREACH
#    warning Current AngelScript version does not support foreach loop
#endif

namespace asbind20
{
template <typename ValueType, typename IteratorRegister, bool Const>
class basic_foreach_register
{
public:
    using value_type = ValueType;
    using iterator = typename IteratorRegister::class_type;

    const IteratorRegister& iter;

    constexpr basic_foreach_register(const IteratorRegister& it) noexcept
        : iter(it) {}

    template <typename RegisterHelper>
    void operator()(RegisterHelper& helper)
    {
        using this_type = std::conditional_t<
            Const,
            const typename RegisterHelper::class_type,
            typename RegisterHelper::class_type>;

        const std::string& it_name = iter.get_name();

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
                iterator tmp(it);
                return ++tmp;
            }
        );
        helper.method(
            string_concat(name_of<value_type>(), " opForValue(const ", it_name, Const ? "&in)const" : "&in)"),
            [](this_type& this_, const iterator& it) -> value_type
            {
                (void)this_;
                return *it;
            }
        );
    }
};
} // namespace asbind20

#endif
