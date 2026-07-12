#ifndef ASBIND20_RANGES_GENERIC_VIEW_HPP
#define ASBIND20_RANGES_GENERIC_VIEW_HPP

#pragma once

#include "common.hpp"
#include "../fwd.hpp"
#include "../detail/include_as.hpp"
#include "../detail/err_handler.hpp"

namespace asbind20::ranges
{
class generic_view : public detail::view_interface<generic_view>
{
public:
    using size_type = int;

    generic_view() = delete;
    generic_view(const generic_view&) = default;

    generic_view(
        generic_pointer gen,
        size_type offset = 0
    ) noexcept
        : m_gen(gen), m_off(offset) {}

    class sentinel
    {};

    class iterator
    {
        friend generic_view;

        explicit iterator(const generic_view* view) noexcept
            : m_view(view)
        {
            if(m_view != nullptr) [[likely]]
                m_offset = static_cast<int>(m_view->m_off);
        }

    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<int, void*>;
        using size_type = int;
        using difference_type = int;

        iterator() = default;

        explicit operator bool() const noexcept
        {
            return m_view != nullptr;
        }

        bool operator==(const iterator& rhs) const noexcept
        {
            ASBIND20_ASSERT(this->m_view == rhs.m_view);
            return this->m_offset == rhs.m_offset;
        }

        friend bool operator==(const iterator& lhs, sentinel) noexcept
        {
            return lhs.at_sentinel();
        }

        friend bool operator==(sentinel, const iterator& rhs) noexcept
        {
            return rhs.at_sentinel();
        }

        iterator& operator++()
        {
            next();
            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp = *this;
            next();
            return tmp;
        }

        value_type operator*() const noexcept
        {
            return {
                m_view->m_gen->GetArgTypeId(m_offset),
                m_view->m_gen->GetAddressOfArg(m_offset)
            };
        }

    private:
        const generic_view* m_view = nullptr;
        int m_offset = 0;

        bool at_sentinel() const noexcept
        {
            if(m_view == nullptr) [[unlikely]]
                return true;
            return m_view->get_generic()->GetArgCount() <= m_offset;
        }

        void next()
        {
            ++m_offset;
        }
    };

    [[nodiscard]]
    bool empty() const noexcept
    {
        return m_gen == nullptr;
    }

    iterator begin() const noexcept
    {
        return iterator(m_gen == nullptr ? nullptr : this);
    }

    static sentinel end() noexcept
    {
        return {};
    }

    [[nodiscard]]
    generic_pointer get_generic() const noexcept
    {
        return m_gen;
    }

    [[nodiscard]]
    int size() const
    {
        if(m_gen == nullptr) [[unlikely]]
            return 0;
        return m_gen->GetArgCount();
    }

private:
    const generic_pointer m_gen = nullptr;
    size_type m_off = 0;
};

namespace views
{
    namespace detail
    {
        struct all_generic_args_t
        {
            generic_view operator()(
                generic_pointer gen,
                generic_view::size_type offset = 0
            ) const
            {
                return generic_view(gen, offset);
            }
        };
    } // namespace detail

    inline constexpr detail::all_generic_args_t all_generic_args{};
} // namespace views
} // namespace asbind20::ranges

#endif
