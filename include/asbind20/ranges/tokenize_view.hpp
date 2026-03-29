#ifndef ASBIND20_RANGES_TOKENIZE_VIEW_HPP
#define ASBIND20_RANGES_TOKENIZE_VIEW_HPP

#pragma once

#include <cassert>
#include "common.hpp"
#include "../detail/include_as.hpp"

namespace asbind20::ranges
{
class tokenize_view : public detail::view_interface<tokenize_view>
{
public:
    tokenize_view() = delete;
    tokenize_view(const tokenize_view&) = default;

    tokenize_view(
        const AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        std::string_view code
    ) noexcept
        : m_engine(engine), m_sv(code) {}

    class sentinel
    {};

    class iterator
    {
        friend tokenize_view;

        explicit iterator(const tokenize_view* view) noexcept
            : m_view(view)
        {
            if(m_view)
                update_cache();
        }

    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<
            AS_NAMESPACE_QUALIFIER asETokenClass,
            std::string_view>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

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

        void operator++(int)
        {
            next();
        }

        const value_type& operator*() const noexcept
        {
            return m_cache;
        }

    private:
        const tokenize_view* m_view = nullptr;
        std::size_t m_offset = 0;
        value_type m_cache{AS_NAMESPACE_QUALIFIER asTC_UNKNOWN, {}};

        bool at_sentinel() const noexcept
        {
            if(m_view == nullptr) [[unlikely]]
                return true;
            return m_view->m_sv.size() <= m_offset;
        }

        void update_cache()
        {
            ASBIND20_ASSERT(this->m_view != nullptr);
            const std::string_view sv = m_view->m_sv;

            AS_NAMESPACE_QUALIFIER asUINT len;
            AS_NAMESPACE_QUALIFIER asETokenClass tc = m_view->m_engine->ParseToken(
                sv.data() + m_offset,
                sv.size() - m_offset,
                &len
            );
            m_cache = value_type{tc, sv.substr(m_offset, len)};
        }

        void next()
        {
            m_offset += m_cache.second.size();
            update_cache();
        }
    };

    bool empty() const noexcept
    {
        if(m_engine == nullptr) [[unlikely]]
            return true;
        return m_sv.empty();
    }

    iterator begin() const noexcept
    {
        return iterator(m_engine == nullptr ? nullptr : this);
    }

    static sentinel end() noexcept
    {
        return {};
    }

    [[nodiscard]]
    auto get_engine() const noexcept
        -> const AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine;
    }

private:
    const AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine;
    std::string_view m_sv;
};

namespace views
{
    namespace detail
    {
        struct tokenize_t
        {
            tokenize_view operator()(
                const AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
                std::string_view code
            ) const
            {
                return tokenize_view(engine, code);
            }

            class proxy
            {
            public:
                proxy() = delete;
                proxy(const proxy&) = default;

                explicit proxy(const AS_NAMESPACE_QUALIFIER asIScriptEngine* engine) noexcept
                    : m_engine(engine)
                {}

                template <std::ranges::contiguous_range R>
                requires(std::same_as<std::ranges::range_value_t<R>, char>)
                friend tokenize_view operator|(R&& lhs, const proxy& rhs)
                {
                    return tokenize_view(
                        rhs.m_engine,
                        std::string_view(
                            std::ranges::cdata(lhs), std::ranges::size(lhs)
                        )
                    );
                }

            private:
                const AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine;
            };

            proxy operator()(
                const AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
            ) const noexcept
            {
                return proxy(engine);
            }
        };
    } // namespace detail

    inline constexpr detail::tokenize_t tokenize{};
} // namespace views
} // namespace asbind20::ranges

#endif
