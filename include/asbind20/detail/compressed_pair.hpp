#ifndef ASBIND20_DETAIL_COMPRESSED_PAIR_HPP
#define ASBIND20_DETAIL_COMPRESSED_PAIR_HPP

#include <tuple>
#include <utility>

namespace asbind20::util
{
namespace detail
{
    template <typename T>
    concept compressible = !std::is_final_v<T> && std::is_empty_v<T>;

    template <typename T1, typename T2>
    consteval int select_compressed_pair_impl()
    {
        if constexpr(!compressible<T1> && !compressible<T2>)
        {
            return 0; // Not compressible. Store them like the `std::pair`.
        }
        else if constexpr(compressible<T1> && !compressible<T2>)
        {
            return 1; // First type is compressible.
        }
        else if constexpr(!compressible<T1> && compressible<T2>)
        {
            return 2; // Second type is compressible.
        }
        else
        {
            // Both are compressible
            if constexpr(std::same_as<T1, T2>)
            {
                // C++ disallows inheriting from two same types,
                // so fallback to as if only the first type is compressible.
                return 1;
            }
            else
                return 3;
        }
    }

    template <
        typename T1,
        typename T2,
        int ImplType = select_compressed_pair_impl<T1, T2>()>
    class compressed_pair_impl;

#define ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_ORDINARY(name) \
    name##_type& name() & noexcept                            \
    {                                                         \
        return m_##name;                                      \
    }                                                         \
    const name##_type& name() const& noexcept                 \
    {                                                         \
        return m_##name;                                      \
    }                                                         \
    name##_type&& name() && noexcept                          \
    {                                                         \
        return std::move(m_##name);                           \
    }                                                         \
    const name##_type&& name() const&& noexcept               \
    {                                                         \
        return std::move(m_##name);                           \
    }

#define ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_COMPRESSED(name) \
    name##_type& name() & noexcept                              \
    {                                                           \
        return *this;                                           \
    }                                                           \
    const name##_type& name() const& noexcept                   \
    {                                                           \
        return *this;                                           \
    }                                                           \
    name##_type&& name() && noexcept                            \
    {                                                           \
        return std::move(*this);                                \
    }                                                           \
    const name##_type&& name() const&& noexcept                 \
    {                                                           \
        return std::move(*this);                                \
    }

    template <typename T1, typename T2>
    class compressed_pair_impl<T1, T2, 0>
    {
    public:
        using first_type = T1;
        using second_type = T2;

        ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_ORDINARY(first);
        ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_ORDINARY(second);

    protected:
        compressed_pair_impl() noexcept(std::is_nothrow_default_constructible_v<T1> && std::is_nothrow_default_constructible_v<T2>) = default;
        compressed_pair_impl(const compressed_pair_impl&) noexcept(std::is_nothrow_copy_constructible_v<T1> && std::is_nothrow_copy_constructible_v<T2>) = default;

        template <typename Tuple1, typename Tuple2, std::size_t... Indices1, std::size_t... Indices2>
        compressed_pair_impl(
            Tuple1&& tuple1,
            Tuple2&& tuple2,
            std::index_sequence<Indices1...>,
            std::index_sequence<Indices2...>
        )
            : m_first(std::get<Indices1>(tuple1)...), m_second(std::get<Indices2>(tuple2)...)
        {}

    private:
        T1 m_first;
        T2 m_second;
    };

    template <typename T1, typename T2>
    class compressed_pair_impl<T1, T2, 1> : public T1
    {
    public:
        using first_type = T1;
        using second_type = T2;

        ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_COMPRESSED(first);
        ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_ORDINARY(second);

    protected:
        compressed_pair_impl() noexcept(std::is_nothrow_default_constructible_v<T1> && std::is_nothrow_default_constructible_v<T2>) = default;
        compressed_pair_impl(const compressed_pair_impl&) noexcept(std::is_nothrow_copy_constructible_v<T1> && std::is_nothrow_copy_constructible_v<T2>) = default;

        template <typename Tuple1, typename Tuple2, std::size_t... Indices1, std::size_t... Indices2>
        compressed_pair_impl(
            Tuple1&& tuple1,
            Tuple2&& tuple2,
            std::index_sequence<Indices1...>,
            std::index_sequence<Indices2...>
        )
            : T1(std::get<Indices1>(tuple1)...), m_second(std::get<Indices2>(tuple2)...)
        {}

    private:
        T2 m_second;
    };

    template <typename T1, typename T2>
    class compressed_pair_impl<T1, T2, 2> : public T2
    {
    public:
        using first_type = T1;
        using second_type = T2;

        ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_ORDINARY(first);
        ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_COMPRESSED(second);

    protected:
        compressed_pair_impl() noexcept(std::is_nothrow_default_constructible_v<T1> && std::is_nothrow_default_constructible_v<T2>) = default;
        compressed_pair_impl(const compressed_pair_impl&) noexcept(std::is_nothrow_copy_constructible_v<T1> && std::is_nothrow_copy_constructible_v<T2>) = default;

        template <typename Tuple1, typename Tuple2, std::size_t... Indices1, std::size_t... Indices2>
        compressed_pair_impl(
            Tuple1&& tuple1,
            Tuple2&& tuple2,
            std::index_sequence<Indices1...>,
            std::index_sequence<Indices2...>
        )
            : T2(std::get<Indices2>(tuple2)...), m_first(std::get<Indices1>(tuple1)...)
        {}

    private:
        T1 m_first;
    };

    template <typename T1, typename T2>
    class compressed_pair_impl<T1, T2, 3> : public T1, public T2
    {
    public:
        using first_type = T1;
        using second_type = T2;

        ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_COMPRESSED(first);
        ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_COMPRESSED(second);

    protected:
        compressed_pair_impl() noexcept(std::is_nothrow_default_constructible_v<T1> && std::is_nothrow_default_constructible_v<T2>) = default;
        compressed_pair_impl(const compressed_pair_impl&) noexcept(std::is_nothrow_copy_constructible_v<T1> && std::is_nothrow_copy_constructible_v<T2>) = default;

        template <typename Tuple1, typename Tuple2, std::size_t... Indices1, std::size_t... Indices2>
        compressed_pair_impl(
            Tuple1&& tuple1,
            Tuple2&& tuple2,
            std::index_sequence<Indices1...>,
            std::index_sequence<Indices2...>
        )
            : T1(std::get<Indices1>(tuple1)...), T2(std::get<Indices2>(tuple2)...)
        {}
    };

#undef ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_COMPRESSED
#undef ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_ORDINARY
} // namespace detail

/**
 * @brief Compressed pair for saving storage space
 *
 * This class will use the empty base optimization (EBO) to reduce the size of compressed pair.
 *
 * @tparam T1 First member type
 * @tparam T2 Second member type
 */
template <typename T1, typename T2>
class compressed_pair
#ifndef ASBIND20_DOXYGEN
    : public detail::compressed_pair_impl<T1, T2>
#endif
{
    using my_base = detail::compressed_pair_impl<T1, T2>;

public:
    using first_type = T1;
    using second_type = T2;

    template <std::size_t Idx>
    requires(Idx < 2)
    using element_type = std::conditional_t<Idx == 0, T1, T2>;

    compressed_pair() = default;

    compressed_pair(const compressed_pair&) = default;

    compressed_pair(compressed_pair&&) noexcept(std::is_nothrow_move_constructible_v<T1> && std::is_nothrow_move_constructible_v<T2>) = default;

    template <typename Tuple1, typename Tuple2>
    compressed_pair(std::piecewise_construct_t, Tuple1&& tuple1, Tuple2&& tuple2)
        : my_base(
              std::forward<Tuple1>(tuple1),
              std::forward<Tuple2>(tuple2),
              std::make_index_sequence<std::tuple_size_v<Tuple1>>(),
              std::make_index_sequence<std::tuple_size_v<Tuple2>>()
          )
    {}

    compressed_pair(const T1& t1, const T2& t2)
        : compressed_pair(std::piecewise_construct, std::make_tuple(t1), std::make_tuple(t2)) {}

    compressed_pair(T1&& t1, T2&& t2) noexcept(std::is_nothrow_move_constructible_v<T1> && std::is_nothrow_move_constructible_v<T2>)
        : compressed_pair(std::piecewise_construct, std::make_tuple(std::move(t1)), std::make_tuple(std::move(t2))) {}

    template <std::size_t Idx>
    friend element_type<Idx>& get(compressed_pair& cp) noexcept
    {
        if constexpr(Idx == 0)
            return cp.first();
        else
            return cp.second();
    }

    template <std::size_t Idx>
    friend const element_type<Idx>& get(const compressed_pair& cp) noexcept
    {
        if constexpr(Idx == 0)
            return cp.first();
        else
            return cp.second();
    }

    template <std::size_t Idx>
    friend element_type<Idx>&& get(compressed_pair&& cp) noexcept
    {
        if constexpr(Idx == 0)
            return std::move(cp).first();
        else
            return std::move(cp).second();
    }

    template <std::size_t Idx>
    friend const element_type<Idx>&& get(const compressed_pair&& cp) noexcept
    {
        if constexpr(Idx == 0)
            return std::move(cp).first();
        else
            return std::move(cp).second();
    }

    void swap(compressed_pair& other) noexcept(std::is_nothrow_swappable_v<T1> && std::is_nothrow_swappable_v<T2>)
    {
        using std::swap;

        swap(this->first(), other.first());
        swap(this->second(), other.second());
    }
};
} // namespace asbind20::util

template <typename T1, typename T2>
struct std::tuple_element<0, asbind20::util::compressed_pair<T1, T2>>
{
    using type = T1;
};

template <typename T1, typename T2>
struct std::tuple_element<1, asbind20::util::compressed_pair<T1, T2>>
{
    using type = T2;
};

template <typename T1, typename T2>
struct std::tuple_size<asbind20::util::compressed_pair<T1, T2>> :
    integral_constant<std::size_t, 2>
{};

#endif
