#ifndef ASBIND20_RANGES_RANGES_HPP
#define ASBIND20_RANGES_RANGES_HPP

#pragma once

#include <cassert>
#include <ranges>
#include "../detail/include_as.hpp"

namespace asbind20
{
namespace views
{
    namespace detail
    {
        class sentinel_base
        {
        public:
            using size_type = AS_NAMESPACE_QUALIFIER asUINT;

            constexpr sentinel_base(const sentinel_base&) noexcept = default;

            [[nodiscard]]
            constexpr size_type max_index() const noexcept
            {
                return m_max_index;
            }

        protected:
            constexpr sentinel_base(size_type max_idx) noexcept
                : m_max_index(max_idx)
            {}

        private:
            size_type m_max_index;
        };
    } // namespace detail

    class all_methods
    {
    public:
        using size_type = AS_NAMESPACE_QUALIFIER asUINT;

        class iterator
        {
            friend all_methods;

            iterator(
                const AS_NAMESPACE_QUALIFIER asITypeInfo* ti,
                size_type idx,
                bool get_virtual = true
            ) noexcept
                : m_ti(ti), m_index(idx), m_get_virtual(get_virtual)
            {}

        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = AS_NAMESPACE_QUALIFIER asIScriptFunction*;
            using reference = value_type;
            using size_type = AS_NAMESPACE_QUALIFIER asUINT;
            using difference_type = std::make_signed_t<size_type>;

            iterator() noexcept
                : m_ti(nullptr), m_index(0), m_get_virtual(true)
            {}

            iterator(const iterator&) noexcept = default;

            iterator& operator++() noexcept
            {
                ++m_index;
                return *this;
            }

            iterator operator++(int) noexcept
            {
                iterator tmp(*this);
                ++m_index;
                return tmp;
            }

            iterator& operator--() noexcept
            {
                assert(m_index > 0);
                --m_index;
                return *this;
            }

            iterator operator--(int) noexcept
            {
                iterator tmp(*this);
                --m_index;
                return tmp;
            }

            bool operator==(const iterator& rhs) const noexcept
            {
                assert(
                    (m_ti == rhs.m_ti && m_get_virtual == rhs.m_get_virtual) &&
                    "Comparing unmatched iterator pair"
                );
                return m_index == rhs.m_index;
            }

            std::strong_ordering operator<=>(const iterator& rhs) const noexcept
            {
                assert(
                    (m_ti == rhs.m_ti && m_get_virtual == rhs.m_get_virtual) &&
                    "Comparing unmatched iterator pair"
                );
                return m_index <=> rhs.m_index;
            }

            value_type operator*() const
            {
                assert(m_ti != nullptr);
                return m_ti->GetMethodByIndex(m_index, m_get_virtual);
            }

        private:
            const AS_NAMESPACE_QUALIFIER asITypeInfo* m_ti;
            size_type m_index;
            bool m_get_virtual;
        };

        all_methods() = delete;
        all_methods(const all_methods&) noexcept = default;

        all_methods(std::nullptr_t) = delete;

        all_methods(
            const AS_NAMESPACE_QUALIFIER asITypeInfo* ti,
            bool get_virtual = true
        )
            : m_ti(ti), m_get_virtual(get_virtual)
        {
            assert(m_ti != nullptr);
        }

        iterator begin() const noexcept
        {
            return iterator(m_ti, 0, m_get_virtual);
        }

        iterator end() const noexcept
        {
            return iterator(m_ti, m_ti->GetMethodCount(), m_get_virtual);
        }

    private:
        const AS_NAMESPACE_QUALIFIER asITypeInfo* m_ti;
        bool m_get_virtual;
    };
} // namespace views

namespace ranges
{

}
} // namespace asbind20

#endif
