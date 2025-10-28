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
template <typename IteratorRegister, bool Const>
class foreach_impl
{
private:
    template <typename RegisterHelper>
    static void setup_foreach_controller(const std::string& it_name, RegisterHelper& helper)
    {
        using this_type = std::conditional_t<
            Const,
            const typename RegisterHelper::class_type,
            typename RegisterHelper::class_type>;

        helper.method(
            use_generic,
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
            use_generic,
            string_concat(it_name, " opForNext(const ", it_name, Const ? "&in)const" : "&in)"),
            [](this_type& this_, const iterator& it) -> iterator
            {
                (void)this_;
                return std::next(it);
            }
        );
    }

public:
    using iterator = typename IteratorRegister::class_type;

    const IteratorRegister& iter;

    constexpr foreach_impl(const IteratorRegister& it) noexcept
        : iter(it) {}

    template <typename ValueType>
    class value_proxy
    {
        friend foreach_impl;

        const foreach_impl& m_this;

        value_proxy(const foreach_impl& this_)
            : m_this(this_) {}

    public:
        template <typename RegisterHelper>
        void operator()(RegisterHelper& helper) const
        {
            using this_type = std::conditional_t<
                Const,
                const typename RegisterHelper::class_type,
                typename RegisterHelper::class_type>;

            const std::string& it_name = m_this.iter.get_name();
            setup_foreach_controller(it_name, helper);
            helper.method(
                use_generic,
                string_concat(name_of<ValueType>(), " opForValue(const ", it_name, Const ? "&in)const" : "&in)"),
                [](this_type& this_, const iterator& it) -> ValueType
                {
                    (void)this_;
                    return *it;
                }
            );
        }
    };

    template <typename ValueType>
    class value_proxy_with_name
    {
        friend foreach_impl;

        const foreach_impl& m_this;
        std::string m_name;

        value_proxy_with_name(const foreach_impl& this_, std::string name)
            : m_this(this_), m_name(std::move(name)) {}

    public:
        template <typename RegisterHelper>
        void operator()(RegisterHelper& helper) const
        {
            using this_type = std::conditional_t<
                Const,
                const typename RegisterHelper::class_type,
                typename RegisterHelper::class_type>;

            const std::string& it_name = m_this.iter.get_name();
            setup_foreach_controller(it_name, helper);
            helper.method(
                use_generic,
                string_concat(m_name, " opForValue(const ", it_name, Const ? "&in)const" : "&in)"),
                [](this_type& this_, const iterator& it) -> ValueType
                {
                    (void)this_;
                    return *it;
                }
            );
        }
    };

    foreach_impl* operator->()
    {
        return this;
    }

    template <has_static_name ValueType>
    value_proxy<ValueType> value()
    {
        return value_proxy<ValueType>(*this);
    }

    template <typename ValueType>
    value_proxy_with_name<ValueType> value(std::string value_name)
    {
        return value_proxy_with_name<ValueType>(
            *this, std::move(value_name)
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
        using value_type = typename std::iterator_traits<iterator>::reference;
        helper.method(
            use_generic,
            string_concat(name_of<value_type>(), " opForValue(const ", it_name, Const ? "&in)const" : "&in)"),
            [](this_type& this_, const iterator& it) -> value_type
            {
                (void)this_;
                return *it;
            }
        );
    }
};

namespace detail
{
    template <bool Const>
    class foreach_func_t
    {
    public:
        template <typename RegisterHelper>
        requires(std::derived_from<RegisterHelper, register_helper_base<true>> || std::derived_from<RegisterHelper, register_helper_base<false>>)
        auto operator()(RegisterHelper& helper) const
        {
            return foreach_impl<RegisterHelper, Const>(helper);
        }
    };
} // namespace detail

inline constexpr detail::foreach_func_t<false> foreach{};
inline constexpr detail::foreach_func_t<true> const_foreach{};
} // namespace asbind20

#endif
