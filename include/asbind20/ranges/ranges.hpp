#ifndef ASBIND20_RANGES_RANGES_HPP
#define ASBIND20_RANGES_RANGES_HPP

#pragma once

#include <cassert>
#include <version>
#include <iterator>
#include "../detail/include_as.hpp"
#ifdef __cpp_lib_ranges
#    define ASBIND20_HAS_LIB_RANGES __cpp_lib_ranges
#    include <ranges>
#endif

namespace asbind20
{
namespace views
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
                Derived tmp(*this);
                ++derived().index;
                return tmp;
            }

            Derived& operator--() noexcept
            {
                assert(derived().index > 0);
                --derived().index;
                return derived();
            }

            Derived operator--(int) noexcept
            {
                Derived tmp(*this);
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

    class all_methods
#ifdef ASBIND20_HAS_LIB_RANGES
        : public std::ranges::view_interface<all_methods>
#endif
    {
    public:
        class iterator : public detail::indexed_iterator_interface<iterator>
        {
            friend all_methods;

            iterator(
                const all_methods* v,
                size_type idx
            ) noexcept
                : m_view(v), index(idx)
            {}

        public:
            using value_type = AS_NAMESPACE_QUALIFIER asIScriptFunction*;
            using reference = value_type;
            using size_type = AS_NAMESPACE_QUALIFIER asUINT;
            using difference_type = std::make_signed_t<size_type>;

            iterator() noexcept = default;

            iterator(const iterator&) noexcept = default;

            iterator& operator=(const iterator&) noexcept = default;

            bool operator==(const iterator& rhs) const noexcept
            {
                assert(
                    m_view == rhs.m_view &&
                    "Comparing unmatched iterator pair"
                );
                return index == rhs.index;
            }

            std::strong_ordering operator<=>(const iterator& rhs) const noexcept
            {
                assert(
                    m_view == rhs.m_view &&
                    "Comparing unmatched iterator pair"
                );
                return index <=> rhs.index;
            }

            explicit operator bool() const noexcept
            {
                return m_view != nullptr;
            }

            value_type operator*() const
            {
                return get_value(index);
            }

            value_type operator[](difference_type n) const
            {
                auto idx = static_cast<size_type>(
                    static_cast<difference_type>(index) + n
                );
                return get_value(idx);
            }

        private:
            const all_methods* m_view = nullptr;

            value_type get_value(size_type idx) const
            {
                assert(*this);
                return m_view->m_ti->GetMethodByIndex(idx, m_view->m_get_virtual);
            }

        public:
            size_type index = 0;
        };

        using size_type = iterator::size_type;

        all_methods() = delete;
        all_methods(const all_methods&) noexcept = default;

        explicit all_methods(std::nullptr_t) = delete;

        explicit all_methods(
            const AS_NAMESPACE_QUALIFIER asITypeInfo* ti,
            bool get_virtual = true
        ) noexcept
            : m_ti(ti), m_get_virtual(get_virtual)
        {
            assert(m_ti != nullptr);
        }

        [[nodiscard]]
        size_type size() const
        {
            if(m_ti == nullptr) [[unlikely]]
                return 0;
            return m_ti->GetMethodCount();
        }

        [[nodiscard]]
        iterator begin() const noexcept
        {
            return {this, 0};
        }

        [[nodiscard]]
        iterator end() const noexcept
        {
            return {this, size()};
        }

    private:
        const AS_NAMESPACE_QUALIFIER asITypeInfo* m_ti;
        bool m_get_virtual;
    };

    class all_behaviours
#ifdef ASBIND20_HAS_LIB_RANGES
        : public std::ranges::view_interface<all_behaviours>
#endif
    {
    public:
        class iterator : public detail::indexed_iterator_interface<iterator>
        {
            friend all_behaviours;

            iterator(
                const all_behaviours* v,
                size_type idx
            ) noexcept
                : m_view(v), index(idx)
            {}

        public:
            using value_type = std::pair<
                AS_NAMESPACE_QUALIFIER asEBehaviours,
                AS_NAMESPACE_QUALIFIER asIScriptFunction*>;
            using reference = value_type;
            using size_type = AS_NAMESPACE_QUALIFIER asUINT;
            using difference_type = std::make_signed_t<size_type>;

            iterator() noexcept = default;

            iterator(const iterator&) noexcept = default;

            iterator& operator=(const iterator&) noexcept = default;

            bool operator==(const iterator& rhs) const noexcept
            {
                assert(
                    m_view == rhs.m_view &&
                    "Comparing unmatched iterator pair"
                );
                return index == rhs.index;
            }

            std::strong_ordering operator<=>(const iterator& rhs) const noexcept
            {
                assert(
                    m_view == rhs.m_view &&
                    "Comparing unmatched iterator pair"
                );
                return index <=> rhs.index;
            }

            explicit operator bool() const noexcept
            {
                return m_view != nullptr;
            }

            value_type operator*() const
            {
                return get_value(index);
            }

            value_type operator[](difference_type n) const
            {
                auto idx = static_cast<size_type>(
                    static_cast<difference_type>(index) + n
                );
                return get_value(idx);
            }

        private:
            const all_behaviours* m_view = nullptr;

            value_type get_value(size_type idx) const
            {
                assert(*this);
                AS_NAMESPACE_QUALIFIER asEBehaviours beh;
                auto* f = m_view->m_ti->GetBehaviourByIndex(idx, &beh);
                return {beh, f};
            }

        public:
            size_type index = 0;
        };

        using size_type = iterator::size_type;

        all_behaviours() = delete;
        all_behaviours(const all_behaviours&) noexcept = default;

        explicit all_behaviours(std::nullptr_t) = delete;

        explicit all_behaviours(
            const AS_NAMESPACE_QUALIFIER asITypeInfo* ti
        )
            : m_ti(ti)
        {}

        [[nodiscard]]
        size_type size() const
        {
            if(m_ti == nullptr) [[unlikely]]
                return 0;
            return m_ti->GetBehaviourCount();
        }

        [[nodiscard]]
        iterator begin() const noexcept
        {
            return {this, 0};
        }

        [[nodiscard]]
        iterator end() const noexcept
        {
            return {this, size()};
        }

    private:
        const AS_NAMESPACE_QUALIFIER asITypeInfo* m_ti;
    };
} // namespace views

namespace ranges
{

}
} // namespace asbind20

#endif
