#ifndef ASBIND20_INVOKE_RESULT_HPP
#define ASBIND20_INVOKE_RESULT_HPP

#include <optional>
#include "../fwd.hpp"
#include "../type_traits.hpp"
#include "../util/unreachable.hpp"
#ifdef ASBIND20_HAS_LIB_EXPECTED
#    include <expected>
#endif

namespace asbind20
{
#if defined(_MSC_VER)
#    pragma warning(push)
// Unreachable code
#    pragma warning(disable : 4702)
#endif

namespace detail
{
    template <typename T>
    concept is_script_obj =
        std::same_as<T, object_pointer> ||
        std::same_as<T, const_object_pointer>;
} // namespace detail

template <typename T>
requires(!std::is_const_v<T> && !std::is_volatile_v<T>)
[[nodiscard]]
decltype(auto) get_script_return(context_reference ctx)
{
    ASBIND20_ASSERT(ctx.GetState() == (AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED));

    constexpr bool is_customized = requires() {
        { type_traits<T>::get_return(ctx) } -> std::convertible_to<T>;
    };

    if constexpr(is_customized)
    {
        return type_traits<T>::get_return(ctx);
    }
    else if constexpr(detail::is_script_obj<std::remove_cvref_t<T>>)
    {
        object_pointer ptr =
            *static_cast<object_pointer*>(ctx.GetAddressOfReturnValue());
        return T(ptr);
    }
    else if constexpr(std::is_reference_v<T>)
    {
        using ptr_t = std::add_pointer_t<std::remove_reference_t<T>>;
        return *reinterpret_cast<ptr_t>(ctx.GetReturnAddress());
    }
    else if constexpr(std::is_pointer_v<T>)
    {
        return static_cast<T>(ctx.GetReturnAddress());
    }
    else if constexpr(std::is_class_v<T>)
    {
        using ptr_t = std::add_pointer_t<std::remove_reference_t<T>>;
        return *reinterpret_cast<ptr_t>(ctx.GetReturnObject());
    }
    else
    {
        using primitive_t = typename std::conditional_t<
            std::is_enum_v<std::remove_cvref_t<T>>,
            compat::script_enum_value_type,
            std::remove_cvref_t<T>>;
        if constexpr(std::integral<primitive_t>)
        {
            if constexpr(sizeof(primitive_t) == 1)
                return static_cast<T>(ctx.GetReturnByte());
            else if constexpr(sizeof(primitive_t) == 2)
                return static_cast<T>(ctx.GetReturnWord());
            else if constexpr(sizeof(primitive_t) == 4)
                return static_cast<T>(ctx.GetReturnDWord());
            else if constexpr(sizeof(primitive_t) == 8)
                return static_cast<T>(ctx.GetReturnQWord());
            else // Compiler extensions like __int128
                return *static_cast<T*>(ctx.GetAddressOfReturnValue());
        }
        else if constexpr(std::same_as<primitive_t, float>)
        {
            return ctx.GetReturnFloat();
        }
        else if constexpr(std::same_as<primitive_t, double>)
        {
            return ctx.GetReturnDouble();
        }
        else
            static_assert(!sizeof(T), "Invalid type");
    }

    // Suppress warning
    util::unreachable();
}

template <typename T>
requires(!std::is_const_v<T> && !std::is_volatile_v<T>)
[[nodiscard]]
decltype(auto) get_script_return(context_pointer ctx)
{
    ASBIND20_ASSERT(ctx != nullptr);
    return get_script_return<T>(*ctx);
}

#if defined(_MSC_VER)
#    pragma warning(pop)
#endif

class bad_script_invoke_result_access : public std::exception
{
public:
    using error_code_type = AS_NAMESPACE_QUALIFIER asEContextState;

    explicit bad_script_invoke_result_access(error_code_type r) noexcept
        : m_r(r) {}

    [[nodiscard]]
    const char* what() const noexcept override
    {
        return "bad script invoke result access";
    }

    [[nodiscard]]
    error_code_type error() const noexcept
    {
        return m_r;
    }

private:
    error_code_type m_r;
};

namespace detail
{
    template <typename T>
    struct invoke_result_traits
    {
        using pointer_type = T*;
        static constexpr bool to_pointer_support = false;
    };

    template <typename T>
    struct invoke_result_traits<T*>
    {
        using pointer_type = T*;
        static constexpr bool to_pointer_support = true;
    };

    template <typename T>
    struct invoke_result_traits<T[]>
    {
        using pointer_type = T*;
        static constexpr bool to_pointer_support = true;
    };

    template <typename T, std::size_t Size>
    struct invoke_result_traits<T[Size]>
    {
        using pointer_type = T*;
        static constexpr bool to_pointer_support = true;
    };

    template <typename T>
    struct invoke_result_traits<T&>
    {
        using pointer_type = T*;
        static constexpr bool to_pointer_support = true;
    };
} // namespace detail

template <typename R>
auto get_context_result(context_reference ctx);

/**
 * @brief Base class of script results
 */
class script_invoke_result_base
{
public:
    using error_type = AS_NAMESPACE_QUALIFIER asEContextState;

    script_invoke_result_base() = delete;
    script_invoke_result_base(const script_invoke_result_base&) noexcept = default;

    script_invoke_result_base& operator=(
        const script_invoke_result_base& other
    ) noexcept = default;

    ~script_invoke_result_base() = default;

    /**
     * @brief Get the script context associated with this result
     *
     * @return Never returns `nullptr`
     */
    [[nodiscard]]
    context_pointer get_context() const noexcept
    {
        return m_ctx;
    }

    /**
     * @brief Returns the AngelScript error code of context state
     */
    [[nodiscard]]
    error_type error() const
    {
        return m_ctx->GetState();
    }

    /**
     * @name Value Status
     * @brief Checks whether the object contains a returned value
     */
    /// @{

    [[nodiscard]]
    bool has_value() const noexcept
    {
        return error() == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED;
    }

    explicit operator bool() const noexcept
    {
        return has_value();
    }

    [[nodiscard]]
    bool has_uncaught_exception() const noexcept
    {
        return m_ctx->GetState() == AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION;
    }

    /// @}

protected:
    explicit script_invoke_result_base(
        context_reference ctx
    ) noexcept
        : m_ctx(std::addressof(ctx))
    {}

    [[noreturn]]
    void throw_bad_access() const
    {
        detail::throw_<bad_script_invoke_result_access>(error());
    }

    void swap(script_invoke_result_base& other) noexcept
    {
        std::swap(m_ctx, other.m_ctx);
    }

private:
    // Never be nullptr
    context_pointer m_ctx;
};

/**
 * @brief Script invocation result
 *
 * @tparam T Result type
 */
template <typename T>
class script_invoke_result : public script_invoke_result_base
{
    template <typename R>
    friend auto get_context_result(context_reference ctx);

    script_invoke_result(
        context_reference ctx
    ) noexcept
        : script_invoke_result_base(ctx) {}

public:
    using value_type = T;
    using return_type =
        decltype(get_script_return<T>(std::declval<context_pointer>()));
    using pointer_type =
        typename detail::invoke_result_traits<return_type>::pointer_type;

    script_invoke_result(
        const script_invoke_result&
    ) noexcept = default;

    script_invoke_result& operator=(
        const script_invoke_result& other
    ) noexcept = default;

    ~script_invoke_result() = default;

    /**
     * @name Unchecked Accessors
     *
     * @note Please check the status of object before directly accessing the value!
     */
    /// @{

    return_type operator*() const
    {
        ASBIND20_ASSERT(has_value());
        return get_script_return<T>(get_context());
    }

    /** @note This function is only available if return type is reference or convertible to pointer */
    pointer_type operator->() const
        requires(detail::invoke_result_traits<return_type>::to_pointer_support)
    {
        ASBIND20_ASSERT(has_value());
        if constexpr(!std::is_reference_v<return_type>)
            return static_cast<pointer_type>(**this);
        else
            return std::addressof(**this);
    }

    /// @}

    /**
     * @name Checked Accessors
     *
     * @brief Throws an exception when the object does not contain a returned value
     */
    /// @{

    [[nodiscard]]
    return_type value() const
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

    template <typename U = std::remove_cv_t<T>>
    value_type value_or(U&& default_val) const
    {
        if(has_value())
            return **this;
        return static_cast<T>(std::forward<U>(default_val));
    }

    /// @}

    [[nodiscard]]
    std::optional<T> to_optional() const
    {
        if(!has_value())
            return std::nullopt;
        return std::optional<T>(**this);
    }

    explicit operator std::optional<T>() const
    {
        return to_optional();
    }

#ifdef ASBIND20_HAS_LIB_EXPECTED

    using expected_type = std::expected<T, error_type>;

    [[nodiscard]]
    expected_type to_expected() const
    {
        if(has_value())
            return expected_type(**this);
        return std::unexpected<error_type>(error());
    }

    explicit operator std::expected<T, error_type>() const
    {
        return to_expected();
    }

#endif

    void swap(script_invoke_result& other) noexcept
    {
        script_invoke_result_base::swap(other);
    }
};

/**
 * @brief Script invocation result for references
 */
template <typename T>
class script_invoke_result<T&> : public script_invoke_result_base
{
    template <typename R>
    friend auto get_context_result(context_reference ctx);

    script_invoke_result(
        context_reference ctx
    ) noexcept
        : script_invoke_result_base(ctx) {}

public:
    using value_type = T&;
    using return_type = T&;
    using pointer_type = T*;

    ~script_invoke_result() = default;

    script_invoke_result& operator=(
        const script_invoke_result& other
    ) noexcept = default;

    return_type operator*() const noexcept
    {
        ASBIND20_ASSERT(has_value());
        return get_script_return<T&>(get_context());
    }

    pointer_type operator->() const noexcept
    {
        ASBIND20_ASSERT(has_value());
        return std::addressof(**this);
    }

    [[nodiscard]]
    return_type value() const
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

    template <typename U = std::remove_cv_t<T>>
    T value_or(U&& default_val) const
    {
        if(has_value())
            return **this;
        else
            return static_cast<T>(std::forward<U>(default_val));
    }

    void swap(script_invoke_result& other) noexcept
    {
        script_invoke_result_base::swap(other);
    }
};

/**
 * @brief Script invocation result for void type
 */
template <>
class script_invoke_result<void> : public script_invoke_result_base
{
    template <typename R>
    friend auto get_context_result(context_reference ctx);

    script_invoke_result(
        context_reference ctx
    ) noexcept
        : script_invoke_result_base(ctx) {}

public:
    using value_type = void;
    using return_type = void;

    script_invoke_result(
        const script_invoke_result& other
    ) noexcept = default;

    template <typename U>
    explicit script_invoke_result(
        const script_invoke_result<U>& other
    ) noexcept
        : script_invoke_result(other.get_context())
    {}

    ~script_invoke_result() = default;

    script_invoke_result& operator=(
        const script_invoke_result& other
    ) noexcept = default;

    template <typename U>
    script_invoke_result& operator=(
        const script_invoke_result<U>& other
    ) noexcept
    {
        script_invoke_result(other).swap(*this);
        return *this;
    }

    void operator*() const noexcept
    {
        ASBIND20_ASSERT(has_value());
    }

    void value() const
    {
        if(!has_value())
            throw_bad_access();
    }

    void swap(script_invoke_result& other) noexcept
    {
        script_invoke_result_base::swap(other);
    }
};

template <typename T>
void swap(script_invoke_result<T>& lhs, script_invoke_result<T>& rhs) noexcept
{
    lhs.swap(rhs);
}

namespace detail
{
    template <typename T>
    struct is_script_invoke_result_impl : std::false_type
    {};

    template <typename T>
    struct is_script_invoke_result_impl<script_invoke_result<T>> : std::true_type
    {};
} // namespace detail

template <typename T>
struct is_script_invoke_result :
    detail::is_script_invoke_result_impl<std::remove_cv_t<T>>
{};

template <typename T>
inline constexpr bool is_script_invoke_result_v = is_script_invoke_result<T>::value;

namespace detail
{
    template <typename T, typename U>
    concept check_op_eq = requires(const T& lhs, const U& rhs) {
        { lhs == rhs } -> std::convertible_to<bool>;
    };
} // namespace detail

template <typename T, typename U>
bool operator==(const script_invoke_result<T>& lhs, const script_invoke_result<U>& rhs)
    requires(detail::check_op_eq<T, U>)
{
    if(lhs.has_value() != rhs.has_value())
        return false;
    return lhs.has_value() ?
               *lhs == *rhs :
               lhs.error() == rhs.error();
}

template <typename T, typename U>
bool operator==(const script_invoke_result<T>& lhs, const U& rhs)
    requires(!is_script_invoke_result_v<U> && detail::check_op_eq<T, U>)
{
    return lhs.has_value() ? *lhs == rhs : false;
}

template <typename T, typename U>
bool operator==(const T& lhs, const script_invoke_result<U>& rhs)
    requires(!is_script_invoke_result_v<T> && detail::check_op_eq<T, U>)
{
    return rhs.has_value() ? lhs == *rhs : false;
}

namespace detail
{
    template <typename T, typename U>
    concept check_op_cmp = requires(const T& lhs, const U& rhs) {
        { lhs == rhs } -> std::convertible_to<bool>;
        { lhs < rhs } -> std::convertible_to<bool>;
        { rhs < lhs } -> std::convertible_to<bool>;
    } || requires(const T& lhs, const U& rhs) {
        { lhs <=> rhs } -> std::convertible_to<std::partial_ordering>;
    };

    template <typename T, typename U>
    std::partial_ordering cmp_weak_ord_helper(T&& lhs, U&& rhs)
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
} // namespace detail

template <typename T, typename U>
std::partial_ordering operator<=>(const script_invoke_result<T>& lhs, const script_invoke_result<U>& rhs)
    requires(detail::check_op_cmp<T, U>)
{
    if(lhs.has_value() != rhs.has_value())
        return std::partial_ordering::unordered;
    if(!lhs.has_value())
    {
        return lhs.error() == rhs.error() ?
                   std::partial_ordering::equivalent :
                   std::partial_ordering::unordered;
    }
    return detail::cmp_weak_ord_helper(*lhs, *rhs);
}

template <typename T, typename U>
std::partial_ordering operator<=>(const script_invoke_result<T>& lhs, const U& rhs)
    requires(!is_script_invoke_result_v<U> && detail::check_op_cmp<T, U>)
{
    if(!lhs.has_value())
        return std::partial_ordering::unordered;
    return detail::cmp_weak_ord_helper(*lhs, rhs);
}

template <typename T, typename U>
std::partial_ordering operator<=>(const T& lhs, const script_invoke_result<U>& rhs)
    requires(!is_script_invoke_result_v<T> && detail::check_op_cmp<T, U>)
{
    if(!rhs.has_value())
        return std::partial_ordering::unordered;
    return detail::cmp_weak_ord_helper(lhs, *rhs);
}

/**
 * @brief Get the result of context
 *
 * @tparam R Return type. It can be safely ignored by `void` if you only want the error code
 *
 * @param ctx Script context.
 *
 * @return Result of the execution
 */
template <typename R>
auto get_context_result(context_reference ctx)
{
    return script_invoke_result<R>(ctx);
}

/**
 * @brief Get the result of context
 *
 * @tparam R Return type. It can be safely ignored by `void` if you only want the error code
 *
 * @param ctx Script context. Cannot be `nullptr`.
 *
 * @return Result of the execution
 */
template <typename R>
auto get_context_result(context_pointer ctx)
{
    ASBIND20_ASSERT(ctx != nullptr);
    return get_context_result<R>(*ctx);
}
} // namespace asbind20

#endif
