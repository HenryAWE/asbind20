/**
 * @file script_func.hpp
 * @author HenryAWE
 * @brief Script function and method wrappers
 */

#ifndef ASBIND20_INVOKE_SCRIPT_FUNC_HPP
#define ASBIND20_INVOKE_SCRIPT_FUNC_HPP

#pragma once

#include <tuple>
#include <functional>
#include <utility>
#include "../fwd.hpp"
#include "../detail/err_handler.hpp"
#include "set_arg.hpp"
#include "result.hpp"

namespace asbind20
{
/**
 * @brief Call a script function
 */
template <typename R, typename... Args>
script_invoke_result<R> script_invoke(
    context_pointer ctx,
    function_pointer func,
    Args&&... args
)
{
    ASBIND20_ASSERT(func != nullptr);
    ASBIND20_ASSERT(ctx != nullptr);

    [[maybe_unused]]
    int r = 0;
    r = ctx->Prepare(func);
    ASBIND20_ASSERT(r >= 0);

    apply_script_args(ctx, std::forward_as_tuple(args...));

    ctx->Execute();
    return get_context_result<R>(ctx);
}


/**
 * @brief Call a method on script object
 */
template <typename R, script_object_handle Object, typename... Args>
script_invoke_result<R> script_invoke(
    context_pointer ctx,
    Object&& obj,
    function_pointer func,
    Args&&... args
)
{
    ASBIND20_ASSERT(func != nullptr);
    ASBIND20_ASSERT(ctx != nullptr);

    [[maybe_unused]]
    int r = 0;
    r = ctx->Prepare(func);
    ASBIND20_ASSERT(r >= 0);
    r = set_script_object(ctx, std::forward<Object>(obj));
    ASBIND20_ASSERT(r >= 0);

    apply_script_args(ctx, std::forward_as_tuple(std::forward<Args>(args)...));

    ctx->Execute();
    return get_context_result<R>(ctx);
}

namespace detail
{
    [[noreturn]]
    inline void throw_bad_call()
    {
        asbind20::detail::throw_<std::bad_function_call>();
    }
} // namespace detail

/**
 * @brief Script function wrapper without ownership, i.e., no reference counting support
 */
template <typename R, typename... Args>
class script_function_ref<R(Args...)>
{
public:
    using handle_type = function_pointer;
    using result_type = script_invoke_result<R>;

    script_function_ref() noexcept
        : script_function_ref(nullptr) {}

    script_function_ref(handle_type func) noexcept
        : m_func(func) {}

    void reset(std::nullptr_t = nullptr) noexcept
    {
        m_func = nullptr;
    }

    void reset(handle_type func) noexcept
    {
        m_func = func;
    }

    [[nodiscard]]
    handle_type target() const noexcept
    {
        return m_func;
    }

    explicit operator bool() const noexcept
    {
        return m_func != nullptr;
    }

    bool operator==(const script_function_ref& other) const noexcept
    {
        return m_func == other.m_func;
    }

    friend bool operator==(const script_function_ref& lhs, handle_type rhs) noexcept
    {
        return lhs.target() == rhs;
    }

    friend bool operator==(handle_type lhs, const script_function_ref& rhs) noexcept
    {
        return lhs == rhs.target();
    }

    result_type operator()(
        context_pointer ctx, Args... args
    ) const
    {
        handle_type func = target();
        if(!func) [[unlikely]]
            detail::throw_bad_call();

        return script_invoke<R>(ctx, func, std::forward<Args>(args)...);
    }

private:
    handle_type m_func;
};

/**
 * @brief Script method wrapper without ownership, i.e., no reference counting support
 */
template <typename R, typename... Args>
class script_method_ref<R(Args...)>
{
public:
    using handle_type = function_pointer;
    using result_type = script_invoke_result<R>;

    script_method_ref() noexcept
        : script_method_ref(nullptr) {}

    script_method_ref(handle_type func) noexcept
        : m_func(func) {}

    void reset(std::nullptr_t = nullptr) noexcept
    {
        m_func = nullptr;
    }

    void reset(handle_type func) noexcept
    {
        m_func = func;
    }

    [[nodiscard]]
    handle_type target() const noexcept
    {
        return m_func;
    }

    explicit operator bool() const noexcept
    {
        return m_func != nullptr;
    }

    bool operator==(const script_method_ref& other) const noexcept
    {
        return m_func == other.m_func;
    }

    friend bool operator==(const script_method_ref& lhs, handle_type rhs) noexcept
    {
        return lhs.target() == rhs;
    }

    friend bool operator==(handle_type lhs, const script_method_ref& rhs) noexcept
    {
        return lhs == rhs.target();
    }

    template <script_object_handle Object>
    result_type operator()(
        context_pointer ctx, Object&& obj, Args... args
    ) const
    {
        handle_type func = target();
        if(!func)
            detail::throw_bad_call();

        return script_invoke<R>(ctx, std::forward<Object>(obj), func, std::forward<Args>(args)...);
    }

private:
    handle_type m_func;
};

template <>
class script_function<void>
{
public:
    using handle_type = function_pointer;

    script_function() noexcept
        : m_func(nullptr) {}

    script_function(const script_function& other)
        : script_function(other.target()) {}

    script_function(script_function&& other) noexcept
        : m_func(std::exchange(other.m_func, nullptr)) {}

    script_function(handle_type func)
        : m_func(func)
    {
        if(m_func)
            (void)m_func->AddRef();
    }

    /**
     * @brief Assign a function object. It @b won't increase the reference count!
     *
     * @warning DON'T use this constructor unless you know what you are doing!
     *          The ownership of the passed function object is transferred to
     *          this wrapper, which will release it on destruction.
     */
    script_function(std::in_place_t, handle_type func) noexcept
        : m_func(func) {}

    ~script_function()
    {
        reset();
    }

    script_function& operator=(const script_function& other)
    {
        if(this == &other)
            return *this;

        reset(other.target());

        return *this;
    }

    script_function& operator=(script_function&& other) noexcept
    {
        script_function(std::move(other)).swap(*this);
        return *this;
    }

    [[nodiscard]]
    handle_type target() const noexcept
    {
        return m_func;
    }

    bool operator==(script_function const& other) const noexcept = default;

    friend bool operator==(const script_function& lhs, handle_type rhs) noexcept
    {
        return lhs.target() == rhs;
    }

    friend bool operator==(handle_type lhs, const script_function& rhs) noexcept
    {
        return lhs == rhs.target();
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(target());
    }

    explicit operator handle_type() const noexcept
    {
        return target();
    }

    handle_type operator->() const noexcept
    {
        return target();
    }

    void reset(std::nullptr_t = nullptr) noexcept
    {
        if(m_func)
        {
            (void)m_func->Release();
            m_func = nullptr;
        }
    }

    void reset(handle_type func)
    {
        // Avoid Release-then-AddRef on the same handle,
        if(m_func == func) [[unlikely]]
            return;

        if(m_func)
            (void)m_func->Release();
        m_func = func;
        if(m_func)
            (void)m_func->AddRef();
    }

    void swap(script_function& other) noexcept
    {
        std::swap(m_func, other.m_func);
    }

private:
    handle_type m_func;
};

/**
 * @brief Wrapper of script function
 */
template <typename R, typename... Args>
class script_function<R(Args...)> : public script_function<void>
{
    using my_base = script_function<void>;

public:
    using result_type = script_invoke_result<R>;

    script_function() noexcept = default;
    script_function(const script_function&) = default;
    script_function(script_function&&) noexcept = default;

    explicit script_function(handle_type func)
        : my_base(func) {}

    /**
     * @brief Assign a function object. It @b won't increase the reference count!
     *
     * @warning DON'T use this constructor unless you know what you are doing!
     *          The ownership of the passed function object is transferred to
     *          this wrapper, which will release it on destruction.
     */
    script_function(std::in_place_t, handle_type func) noexcept
        : my_base(std::in_place, func) {}

    script_function& operator=(const script_function&) = default;
    script_function& operator=(script_function&&) noexcept = default;

    script_function(script_function_ref<R(Args...)> rf) noexcept
        : my_base(rf.target()) {}

    result_type operator()(
        context_pointer ctx, Args... args
    ) const
    {
        handle_type func = target();
        if(!func)
            detail::throw_bad_call();

        return script_invoke<R>(ctx, func, std::forward<Args>(args)...);
    }

    void swap(script_function& other) noexcept
    {
        my_base::swap(other);
    }

    operator script_function_ref<R(Args...)>() const noexcept
    {
        return target();
    }
};

/**
 * @brief Wrapper of script method, a.k.a member function
 */
template <typename R, typename... Args>
class script_method<R(Args...)> : public script_function<void>
{
    using my_base = script_function<void>;

public:
    using result_type = script_invoke_result<R>;

    script_method() noexcept = default;
    script_method(const script_method&) = default;
    script_method(script_method&&) noexcept = default;

    explicit script_method(handle_type func)
        : my_base(func) {}

    script_method(script_method_ref<R(Args...)> rf) noexcept
        : my_base(rf.target()) {}

    script_method& operator=(const script_method&) = default;
    script_method& operator=(script_method&&) noexcept = default;

    template <script_object_handle Object>
    result_type operator()(
        context_pointer ctx, Object&& obj, Args... args
    ) const
    {
        handle_type func = target();
        if(!func)
            detail::throw_bad_call();

        return script_invoke<R>(ctx, std::forward<Object>(obj), func, std::forward<Args>(args)...);
    }

    result_type operator()(
        context_pointer ctx, const void* obj, Args... args
    ) const
    {
        handle_type func = target();
        if(!func)
            detail::throw_bad_call();

        return script_invoke<R>(ctx, obj, func, std::forward<Args>(args)...);
    }

    void swap(script_method& other) noexcept
    {
        my_base::swap(other);
    }

    operator script_method_ref<R(Args...)>() const noexcept
    {
        return target();
    }
};
} // namespace asbind20

#endif
