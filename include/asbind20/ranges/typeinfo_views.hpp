#ifndef ASBIND20_RANGES_TYPEINFO_VIEW_HPP
#define ASBIND20_RANGES_TYPEINFO_VIEW_HPP

#pragma once

#include <cassert>
#include "common.hpp"
#include "../detail/strutil.hpp"
#include "../detail/include_as.hpp"
#include "../detail/compat.hpp"

namespace asbind20::ranges
{
namespace detail
{
    template <typename Derived>
    class indexed_iterator_interface
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using size_type = AS_NAMESPACE_QUALIFIER asUINT;
        using difference_type = std::make_signed_t<size_type>;

        Derived& operator++() noexcept
        {
            ++derived().index;
            return derived();
        }

        Derived operator++(int) noexcept
        {
            Derived tmp(derived());
            ++derived().index;
            return tmp;
        }

        Derived& operator--() noexcept
        {
            ASBIND20_ASSERT(derived().index > 0);
            --derived().index;
            return derived();
        }

        Derived operator--(int) noexcept
        {
            Derived tmp(derived());
            --derived().index;
            return tmp;
        }

        Derived& operator+=(difference_type n) noexcept
        {
            derived().index += n;
            return derived();
        }

        Derived& operator-=(difference_type n) noexcept
        {
            derived().index -= n;
            return derived();
        }

        friend Derived operator+(const Derived& lhs, difference_type rhs) noexcept
        {
            Derived tmp(lhs);
            tmp += rhs;
            return tmp;
        }

        friend Derived operator+(difference_type lhs, const Derived& rhs) noexcept
        {
            Derived tmp(rhs);
            tmp += lhs;
            return tmp;
        }

        friend Derived operator-(const Derived& lhs, difference_type rhs) noexcept
        {
            Derived tmp(lhs);
            tmp -= rhs;
            return tmp;
        }

        friend difference_type operator-(const Derived& lhs, const Derived& rhs) noexcept
        {
            return static_cast<difference_type>(lhs.index) -
                   static_cast<difference_type>(rhs.index);
        }

        auto operator*() const
        {
            return derived().get_value(derived().index);
        }

        auto operator[](difference_type n) const
        {
            auto idx = static_cast<size_type>(
                static_cast<difference_type>(derived().index) + n
            );
            return derived().get_value(idx);
        }

    protected:
        constexpr indexed_iterator_interface() noexcept = default;
        constexpr indexed_iterator_interface(const indexed_iterator_interface&) noexcept = default;

        indexed_iterator_interface& operator=(const indexed_iterator_interface&) noexcept = default;

    private:
        constexpr Derived& derived() noexcept
        {
            return static_cast<Derived&>(*this);
        }

        constexpr const Derived& derived() const noexcept
        {
            return static_cast<const Derived&>(*this);
        }
    };
} // namespace detail

#define ASBIND20_VIEWS_COMMON_ITER_MEMBERS_IMPL()                        \
    iterator() noexcept = default;                                       \
    iterator(const iterator&) noexcept = default;                        \
    iterator& operator=(const iterator&) noexcept = default;             \
    bool operator==(const iterator& rhs) const noexcept                  \
    {                                                                    \
        ASBIND20_ASSERT(                                                 \
            this->m_view == rhs.m_view &&                                \
            "Comparing unmatched iterator pair"                          \
        );                                                               \
        return this->index == rhs.index;                                 \
    }                                                                    \
    std::strong_ordering operator<=>(const iterator& rhs) const noexcept \
    {                                                                    \
        ASBIND20_ASSERT(                                                 \
            this->m_view == rhs.m_view &&                                \
            "Comparing unmatched iterator pair"                          \
        );                                                               \
        return this->index <=> rhs.index;                                \
    }                                                                    \
    explicit operator bool() const noexcept                              \
    {                                                                    \
        return this->m_view != nullptr;                                  \
    }

#define ASBIND20_VIEWS_TYPEINFO_BASED_VIEW_COMMON_MEMBERS_IMPL(size_getter) \
    [[nodiscard]]                                                           \
    size_type size() const                                                  \
    {                                                                       \
        if(this->m_ti == nullptr) [[unlikely]]                              \
            return 0;                                                       \
        return this->m_ti->size_getter();                                   \
    }                                                                       \
    [[nodiscard]]                                                           \
    bool empty() const                                                      \
    {                                                                       \
        return size() == 0;                                                 \
    }                                                                       \
    [[nodiscard]]                                                           \
    iterator begin() const noexcept                                         \
    {                                                                       \
        return {this, 0};                                                   \
    }                                                                       \
    [[nodiscard]]                                                           \
    iterator end() const noexcept                                           \
    {                                                                       \
        return {this, this->size()};                                        \
    }                                                                       \
    [[nodiscard]]                                                           \
    const_typeinfo_pointer get_type_info() const noexcept                   \
    {                                                                       \
        return this->m_ti;                                                  \
    }

class all_methods_view : public detail::view_interface<all_methods_view>
{
public:
    class iterator : public detail::indexed_iterator_interface<iterator>
    {
        using my_base = detail::indexed_iterator_interface<iterator>;
        friend all_methods_view;
        friend my_base;

    public:
        using value_type = function_pointer;
        using size_type = typename my_base::size_type;

    private:
        iterator(
            const all_methods_view* v,
            size_type idx
        ) noexcept
            : m_view(v), index(idx)
        {}

    public:
        ASBIND20_VIEWS_COMMON_ITER_MEMBERS_IMPL()

    private:
        const all_methods_view* m_view = nullptr;

        value_type get_value(size_type idx) const
        {
            ASBIND20_ASSERT(*this);
            return m_view->m_ti->GetMethodByIndex(idx, m_view->m_get_virtual);
        }

    public:
        size_type index = 0;
    };

    using size_type = typename iterator::size_type;

    all_methods_view() = delete;
    all_methods_view(const all_methods_view&) noexcept = default;

    explicit all_methods_view(
        const_typeinfo_pointer ti,
        bool get_virtual = true
    ) noexcept
        : m_ti(ti), m_get_virtual(get_virtual)
    {}

    ASBIND20_VIEWS_TYPEINFO_BASED_VIEW_COMMON_MEMBERS_IMPL(GetMethodCount)

private:
    const_typeinfo_pointer m_ti;
    bool m_get_virtual;
};

class all_behaviours_view : public detail::view_interface<all_behaviours_view>
{
public:
    class iterator : public detail::indexed_iterator_interface<iterator>
    {
        using my_base = detail::indexed_iterator_interface<iterator>;
        friend all_behaviours_view;
        friend my_base;

    public:
        using value_type = std::pair<
            AS_NAMESPACE_QUALIFIER asEBehaviours,
            function_pointer>;
        using size_type = typename my_base::size_type;

    private:
        iterator(
            const all_behaviours_view* v,
            size_type idx
        ) noexcept
            : m_view(v), index(idx)
        {}

    public:
        ASBIND20_VIEWS_COMMON_ITER_MEMBERS_IMPL()

    private:
        const all_behaviours_view* m_view = nullptr;

        value_type get_value(size_type idx) const
        {
            ASBIND20_ASSERT(*this);
            AS_NAMESPACE_QUALIFIER asEBehaviours beh;
            auto* f = m_view->m_ti->GetBehaviourByIndex(idx, &beh);
            return {beh, f};
        }

    public:
        size_type index = 0;
    };

    using size_type = typename iterator::size_type;

    all_behaviours_view() = delete;
    all_behaviours_view(const all_behaviours_view&) noexcept = default;

    explicit all_behaviours_view(
        const_typeinfo_pointer ti
    ) noexcept
        : m_ti(ti)
    {}

    ASBIND20_VIEWS_TYPEINFO_BASED_VIEW_COMMON_MEMBERS_IMPL(GetBehaviourCount)

private:
    const_typeinfo_pointer m_ti;
};

class all_factories_view : public detail::view_interface<all_factories_view>
{
public:
    class iterator : public detail::indexed_iterator_interface<iterator>
    {
        using my_base = detail::indexed_iterator_interface<iterator>;
        friend all_factories_view;
        friend my_base;

    public:
        using value_type = function_pointer;
        using size_type = typename my_base::size_type;

    private:
        iterator(
            const all_factories_view* v,
            size_type idx
        ) noexcept
            : m_view(v), index(idx)
        {}

    public:
        ASBIND20_VIEWS_COMMON_ITER_MEMBERS_IMPL()

    private:
        const all_factories_view* m_view = nullptr;

        value_type get_value(size_type idx) const
        {
            ASBIND20_ASSERT(*this);
            return m_view->m_ti->GetFactoryByIndex(idx);
        }

    public:
        size_type index = 0;
    };

    using size_type = typename iterator::size_type;

    all_factories_view() = delete;
    all_factories_view(const all_factories_view&) noexcept = default;

    explicit all_factories_view(
        const_typeinfo_pointer ti
    ) noexcept
        : m_ti(ti)
    {}

    ASBIND20_VIEWS_TYPEINFO_BASED_VIEW_COMMON_MEMBERS_IMPL(GetFactoryCount)

private:
    const_typeinfo_pointer m_ti;
};

template <std::integral UnderlyingType = int>
class all_enum_values_view :
    public detail::view_interface<all_enum_values_view<UnderlyingType>>
{
public:
#ifndef ASBIND20_HAS_ENUM_UNDERLYING_TYPE
    static_assert(
        std::same_as<UnderlyingType, int>,
        "Older AngelScript (<= 2.38) only allows int as underlying type of enum"
    );

#endif

    using underlying_type = UnderlyingType;

    class iterator : public detail::indexed_iterator_interface<iterator>
    {
        using my_base = detail::indexed_iterator_interface<iterator>;
        friend all_enum_values_view;
        friend my_base;

    public:
        using size_type = AS_NAMESPACE_QUALIFIER asUINT;
        using value_type = std::pair<cstring_ref, UnderlyingType>;

    private:
        iterator(const all_enum_values_view* view, size_type idx) noexcept
            : m_view(view), index(idx) {}

    public:
        ASBIND20_VIEWS_COMMON_ITER_MEMBERS_IMPL()

    private:
        const all_enum_values_view* m_view = nullptr;

        value_type get_value(size_type idx) const
        {
            ASBIND20_ASSERT(*this);
            compat::script_enum_value_type val;
            const char* name = m_view->m_ti->GetEnumValueByIndex(idx, &val);
            return {name, static_cast<UnderlyingType>(val)};
        }

    public:
        size_type index = 0;
    };

    using size_type = typename iterator::size_type;

    all_enum_values_view() = delete;
    all_enum_values_view(const all_enum_values_view&) noexcept = default;

    explicit all_enum_values_view(
        const_typeinfo_pointer ti
    ) noexcept
        : m_ti(ti)
    {}

    ASBIND20_VIEWS_TYPEINFO_BASED_VIEW_COMMON_MEMBERS_IMPL(GetEnumValueCount)

private:
    const_typeinfo_pointer m_ti;
};

#undef ASBIND20_VIEWS_COMMON_ITER_MEMBERS_IMPL
#undef ASBIND20_VIEWS_TYPEINFO_BASED_VIEW_COMMON_MEMBERS_IMPL

namespace views
{
    namespace detail
    {
        struct all_methods_t
        {
            all_methods_view operator()(
                typeinfo_pointer ti,
                bool get_virtual = true
            ) const
            {
                return all_methods_view(ti, get_virtual);
            }
        };
    } // namespace detail

    inline constexpr detail::all_methods_t all_methods{};

    namespace detail
    {
        struct all_behaviours_t
        {
            all_behaviours_view operator()(
                const_typeinfo_pointer ti
            ) const
            {
                return all_behaviours_view(ti);
            }
        };
    } // namespace detail

    inline constexpr detail::all_behaviours_t all_behaviours{};

    namespace detail
    {
        struct all_factories_t
        {
            all_factories_view operator()(
                const_typeinfo_pointer ti
            ) const
            {
                return all_factories_view(ti);
            }
        };
    } // namespace detail

    inline constexpr detail::all_factories_t all_factories{};

    namespace detail
    {
        template <typename UnderlyingType>
        struct all_enum_values_of_t
        {
            all_enum_values_view<UnderlyingType> operator()(
                const_typeinfo_pointer ti
            ) const
            {
                return all_enum_values_view<UnderlyingType>(ti);
            }
        };
    } // namespace detail

    inline constexpr detail::all_enum_values_of_t<int> all_enum_values{};
    template <std::integral Underlying>
    inline constexpr detail::all_enum_values_of_t<Underlying> all_enum_values_of{};
} // namespace views
} // namespace asbind20::ranges

#endif
