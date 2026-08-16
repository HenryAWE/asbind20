#ifndef ASBIND20_RANGES_COMMON_HPP
#define ASBIND20_RANGES_COMMON_HPP

#pragma once

#include "../fwd.hpp"

// IWYU pragma: begin_exports

#include <iterator>
#ifdef ASBIND20_HAS_LIB_RANGES
#    include <ranges>
#endif

// IWYU pragma: end_exports

namespace asbind20::ranges
{
namespace detail
{
#ifdef ASBIND20_HAS_LIB_RANGES
    template <typename View>
    using view_interface = std::ranges::view_interface<View>;

#else
    // Placeholder if standard library doesn't provide ranges library.
    // Because this library is designed to support old toolchain like LLVM-15.
    template <typename View>
    class view_interface
    {};

#endif

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

        friend bool operator==(
            const Derived& lhs,
            const Derived& rhs
        ) noexcept
        {
            ASBIND20_ASSERT(
                lhs.m_view == rhs.m_view &&
                "Comparing unmatched iterator pair"
            );
            return lhs.index == rhs.index;
        }

        friend std::strong_ordering operator<=>(
            const Derived& lhs,
            const Derived& rhs
        ) noexcept
        {
            ASBIND20_ASSERT(
                lhs.m_view == rhs.m_view &&
                "Comparing unmatched iterator pair"
            );
            return lhs.index <=> rhs.index;
        }

        explicit operator bool() const noexcept
        {
            return this->derived().m_view != nullptr;
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
} // namespace asbind20::ranges

#endif
