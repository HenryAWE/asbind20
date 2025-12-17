#ifndef ASBIND20_RANGES_RANGES_HPP
#define ASBIND20_RANGES_RANGES_HPP

#pragma once

#include <cassert>
#include <iterator>
#include "../detail/config.hpp"
#include "../detail/strutil.hpp"
#include "../detail/include_as.hpp"
#ifdef __cpp_lib_ranges
#    define ASBIND20_HAS_LIB_RANGES __cpp_lib_ranges
#    include <ranges>
#endif

namespace asbind20
{
namespace ranges
{
    namespace detail
    {
#ifdef ASBIND20_HAS_LIB_RANGES
        template <typename View>
        using view_interface = std::ranges::view_interface<View>;

#else
        // Placeholder if standard library doesn't provide ranges library
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
        assert(                                                          \
            this->m_view == rhs.m_view &&                                \
            "Comparing unmatched iterator pair"                          \
        );                                                               \
        return this->index == rhs.index;                                 \
    }                                                                    \
    std::strong_ordering operator<=>(const iterator& rhs) const noexcept \
    {                                                                    \
        assert(                                                          \
            this->m_view == rhs.m_view &&                                \
            "Comparing unmatched iterator pair"                          \
        );                                                               \
        return this->index <=> rhs.index;                                \
    }                                                                    \
    explicit operator bool() const noexcept                              \
    {                                                                    \
        return this->m_view != nullptr;                                  \
    }

#define ASBIND20_VIEWS_TYPEINFO_BASED_VIEW_ITER_IMPL(size_getter) \
    [[nodiscard]]                                                 \
    size_type size() const                                        \
    {                                                             \
        if(m_ti == nullptr) [[unlikely]]                          \
            return 0;                                             \
        return m_ti->size_getter();                               \
    }                                                             \
    [[nodiscard]]                                                 \
    iterator begin() const noexcept                               \
    {                                                             \
        return {this, 0};                                         \
    }                                                             \
    [[nodiscard]]                                                 \
    iterator end() const noexcept                                 \
    {                                                             \
        return {this, this->size()};                              \
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
            using value_type = AS_NAMESPACE_QUALIFIER asIScriptFunction*;
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
                assert(*this);
                return m_view->m_ti->GetMethodByIndex(idx, m_view->m_get_virtual);
            }

        public:
            size_type index = 0;
        };

        using size_type = typename iterator::size_type;

        all_methods_view() = delete;
        all_methods_view(const all_methods_view&) noexcept = default;

        explicit all_methods_view(
            const AS_NAMESPACE_QUALIFIER asITypeInfo* ti,
            bool get_virtual = true
        ) noexcept
            : m_ti(ti), m_get_virtual(get_virtual)
        {
            assert(m_ti != nullptr);
        }

        ASBIND20_VIEWS_TYPEINFO_BASED_VIEW_ITER_IMPL(GetMethodCount)

    private:
        const AS_NAMESPACE_QUALIFIER asITypeInfo* m_ti;
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
                AS_NAMESPACE_QUALIFIER asIScriptFunction*>;
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
                assert(*this);
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
            const AS_NAMESPACE_QUALIFIER asITypeInfo* ti
        ) noexcept
            : m_ti(ti)
        {}

        ASBIND20_VIEWS_TYPEINFO_BASED_VIEW_ITER_IMPL(GetBehaviourCount)

    private:
        const AS_NAMESPACE_QUALIFIER asITypeInfo* m_ti;
    };

    template <typename UnderlyingType = int>
    class all_enums_view : public detail::view_interface<all_enums_view<UnderlyingType>>
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
            friend all_enums_view;
            friend my_base;

            // TODO: Merge this alias with that in bind/enum.hpp
#ifndef ASBIND20_HAS_ENUM_UNDERLYING_TYPE
            using script_enum_value_type = int;
#else
            using script_enum_value_type = AS_NAMESPACE_QUALIFIER asINT64;
#endif

        public:
            using size_type = AS_NAMESPACE_QUALIFIER asUINT;
            using value_type = std::pair<cstring_ref, UnderlyingType>;

        private:
            iterator(const all_enums_view* view, size_type idx) noexcept
                : m_view(view), index(idx) {}

        public:
            ASBIND20_VIEWS_COMMON_ITER_MEMBERS_IMPL()

        private:
            const all_enums_view* m_view = nullptr;

            value_type get_value(size_type idx) const
            {
                assert(*this);
                script_enum_value_type val;
                const char* name = m_view->m_ti->GetEnumValueByIndex(idx, &val);
                return {name, static_cast<UnderlyingType>(val)};
            }

        public:
            size_type index = 0;
        };

        using size_type = typename iterator::size_type;

        all_enums_view() = delete;
        all_enums_view(const all_enums_view&) noexcept = default;

        explicit all_enums_view(
            const AS_NAMESPACE_QUALIFIER asITypeInfo* ti
        ) noexcept
            : m_ti(ti)
        {}

        ASBIND20_VIEWS_TYPEINFO_BASED_VIEW_ITER_IMPL(GetEnumValueCount)

    private:
        const AS_NAMESPACE_QUALIFIER asITypeInfo* m_ti;
    };

#undef ASBIND20_VIEWS_COMMON_ITER_MEMBERS_IMPL
#undef ASBIND20_VIEWS_TYPEINFO_BASED_VIEW_ITER_IMPL

    namespace views
    {
        namespace detail
        {
            struct all_methods_t
            {
                all_methods_view operator()(
                    AS_NAMESPACE_QUALIFIER asITypeInfo* ti,
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
                    const AS_NAMESPACE_QUALIFIER asITypeInfo* ti
                ) const
                {
                    return all_behaviours_view(ti);
                }
            };
        } // namespace detail

        inline constexpr detail::all_behaviours_t all_behaviours{};

        namespace detail
        {
            template <typename UnderlyingType>
            struct all_enums_of_t
            {
                all_enums_view<UnderlyingType> operator()(
                    const AS_NAMESPACE_QUALIFIER asITypeInfo* ti
                ) const
                {
                    return all_enums_view<int>(ti);
                }
            };
        } // namespace detail

        inline constexpr detail::all_enums_of_t<int> all_enums{};
        template <typename Underlying>
        inline constexpr detail::all_enums_of_t<Underlying> all_enums_of{};
    } // namespace views
} // namespace ranges

namespace views = ranges::views;
} // namespace asbind20

#endif
