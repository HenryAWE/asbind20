#ifndef ASBIND20_DETAIL_CMP_HELPERS_HPP
#define ASBIND20_DETAIL_CMP_HELPERS_HPP

#include <concepts>

namespace asbind20::detail
{
template <typename T, typename U>
concept check_op_eq = requires(const T& lhs, const U& rhs) {
    { lhs == rhs } -> std::convertible_to<bool>;
};

template <typename T, typename U>
concept check_op_cmp = requires(const T& lhs, const U& rhs) {
    { lhs == rhs } -> std::convertible_to<bool>;
    { lhs < rhs } -> std::convertible_to<bool>;
    { rhs < lhs } -> std::convertible_to<bool>;
} || requires(const T& lhs, const U& rhs) {
    { lhs <=> rhs } -> std::convertible_to<std::partial_ordering>;
};

template <typename T, typename U>
constexpr std::partial_ordering cmp_weak_ord_helper(T&& lhs, U&& rhs)
{
    using std::partial_ordering;
    constexpr bool use_three_way = requires() {
        { lhs <=> rhs } -> std::convertible_to<std::partial_ordering>;
    };
    if constexpr(use_three_way)
        return std::forward<T>(lhs) <=> std::forward<U>(rhs);
    else
    {
        // Logic of std::compare_partial_order_fallback
        return std::forward<T>(lhs) == std::forward<U>(rhs) ? partial_ordering::equivalent :
               std::forward<T>(lhs) < std::forward<U>(rhs)  ? partial_ordering::less :
               std::forward<U>(rhs) < std::forward<T>(lhs)  ? partial_ordering::greater :
                                                              partial_ordering::unordered;
    }
}
} // namespace asbind20::detail

#endif
