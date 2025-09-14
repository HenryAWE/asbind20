/**
 * @file invoke.hpp
 * @author HenryAWE
 * @brief Invocation tools
 */

#ifndef ASBIND20_INVOKE_HPP
#define ASBIND20_INVOKE_HPP

#pragma once

#include <version>
#include <tuple>
#include <functional>
#include <optional>
#include "detail/include_as.hpp"
#include "utility.hpp"
#include "type_traits.hpp"
#ifdef __cpp_lib_expected
#    define ASBIND20_HAS_EXPECTED __cpp_lib_expected
#    include <expected>
#endif

namespace asbind20
{
namespace detail
{
    template <typename T>
    concept is_script_obj =
        std::same_as<T, AS_NAMESPACE_QUALIFIER asIScriptObject*> ||
        std::same_as<T, AS_NAMESPACE_QUALIFIER asIScriptObject const*>;
} // namespace detail

template <typename T>
requires(!std::is_const_v<T>)
decltype(auto) get_script_return(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx)
{
    assert(ctx->GetState() == (AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED));

    constexpr bool is_customized = requires() {
        { type_traits<T>::get_return(ctx) } -> std::convertible_to<T>;
    };

    if constexpr(is_customized)
    {
        return type_traits<T>::get_return(ctx);
    }
    else if constexpr(detail::is_script_obj<std::remove_cvref_t<T>>)
    {
        AS_NAMESPACE_QUALIFIER asIScriptObject* ptr =
            *(AS_NAMESPACE_QUALIFIER asIScriptObject**)ctx->GetAddressOfReturnValue();
        return T(ptr);
    }
    else if constexpr(std::is_reference_v<T>)
    {
        using ptr_t = std::add_pointer_t<std::remove_reference_t<T>>;
        return *reinterpret_cast<ptr_t>(ctx->GetReturnAddress());
    }
    else if constexpr(std::is_pointer_v<T>)
    {
        return (T)ctx->GetReturnAddress();
    }
    else if constexpr(std::is_class_v<T>)
    {
        using ptr_t = std::add_pointer_t<std::remove_reference_t<T>>;
        return *reinterpret_cast<ptr_t>(ctx->GetReturnObject());
    }
    else
    {
        using primitive_t = typename std::conditional_t<
            std::is_enum_v<std::remove_cvref_t<T>>,
            int,
            std::remove_cvref_t<T>>;
        if constexpr(std::integral<primitive_t>)
        {
            if constexpr(sizeof(primitive_t) == 1)
                return static_cast<T>(ctx->GetReturnByte());
            else if constexpr(sizeof(primitive_t) == 2)
                return static_cast<T>(ctx->GetReturnWord());
            else if constexpr(sizeof(primitive_t) == 4)
                return static_cast<T>(ctx->GetReturnDWord());
            else if constexpr(sizeof(primitive_t) == 8)
                return static_cast<T>(ctx->GetReturnQWord());
            else // Compiler extensions like __int128
                return *(T*)ctx->GetAddressOfReturnValue();
        }
        else if constexpr(std::same_as<primitive_t, float>)
        {
            return ctx->GetReturnFloat();
        }
        else if constexpr(std::same_as<primitive_t, double>)
        {
            return ctx->GetReturnDouble();
        }
        else
            static_assert(!sizeof(T), "Invalid type");
    }
}

class bad_script_invoke_result_access : public std::exception
{
public:
    using error_code_type = AS_NAMESPACE_QUALIFIER asEContextState;

    bad_script_invoke_result_access(error_code_type r) noexcept
        : m_r(r) {}

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
auto get_context_result(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx);

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

    [[nodiscard]]
    auto get_context() const noexcept
        -> AS_NAMESPACE_QUALIFIER asIScriptContext*
    {
        return m_ctx;
    }

    /**
     * @brief Returns the AngelScript error code of context state
     */
    [[nodiscard]]
    auto error() const
        -> AS_NAMESPACE_QUALIFIER asEContextState
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

    /// @}

protected:
    script_invoke_result_base(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx
    ) noexcept
        : m_ctx(ctx)
    {
        assert(m_ctx != nullptr);
    }

    [[noreturn]]
    void throw_bad_access() const
    {
        throw bad_script_invoke_result_access(error());
    }

private:
    AS_NAMESPACE_QUALIFIER asIScriptContext* m_ctx;
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
    friend auto get_context_result(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx);

    script_invoke_result(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx
    ) noexcept
        : script_invoke_result_base(ctx) {}

public:
    using value_type = T;
    using return_type =
        decltype(get_script_return<T>(std::declval<AS_NAMESPACE_QUALIFIER asIScriptContext*>()));
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
        assert(has_value());
        return get_script_return<T>(get_context());
    }

    /** @note This function is only available if return type is reference or convertible to pointer */
    pointer_type operator->() const requires(
        detail::invoke_result_traits<return_type>::to_pointer_support
    )
    {
        assert(has_value());
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
        else
            return static_cast<T>(std::forward<U>(default_val));
    }

    /// @}

    [[nodiscard]]
    std::optional<T> to_optional() const
    {
        if(!has_value())
            return std::nullopt;
        else
            return std::optional<T>(**this);
    }

    explicit operator std::optional<T>() const
    {
        return to_optional();
    }

#ifdef ASBIND20_HAS_EXPECTED

    [[nodiscard]]
    std::expected<T, error_type> to_expected() const
    {
        if(has_value())
            return std::expected<T, error_type>(**this);
        else
            return std::unexpected<error_type>(error());
    }

    operator std::expected<T, error_type>() const
    {
        return to_expected();
    }

#endif
};

/**
 * @brief Script invocation result for references
 */
template <typename T>
class script_invoke_result<T&> : public script_invoke_result_base
{
    template <typename R>
    friend auto get_context_result(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx);

    script_invoke_result(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx
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
        assert(has_value());
        return get_script_return<T&>(get_context());
    }

    pointer_type operator->() const noexcept
    {
        assert(has_value());
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
    T& value_or(U&& default_val) const
    {
        if(has_value())
            return **this;
        else
            return std::forward<U>(default_val);
    }
};

/**
 * @brief Script invocation result for void type
 */
template <>
class script_invoke_result<void> : public script_invoke_result_base
{
    template <typename R>
    friend auto get_context_result(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx);

    script_invoke_result(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx
    ) noexcept
        : script_invoke_result_base(ctx) {}

public:
    using value_type = void;
    using return_type = void;

    script_invoke_result(
        const script_invoke_result& other
    ) noexcept = default;

    ~script_invoke_result() = default;

    script_invoke_result& operator=(
        const script_invoke_result& other
    ) noexcept = default;

    void operator*() const noexcept
    {
        assert(has_value());
    }

    void value() const
    {
        if(!has_value())
            throw_bad_access();
    }
};

template <typename T>
int set_script_arg(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
    AS_NAMESPACE_QUALIFIER asUINT idx,
    std::reference_wrapper<T> ref
)
{
    return ctx->SetArgAddress(idx, (void*)std::addressof(ref.get()));
}

template <std::integral T>
int set_script_arg(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
    AS_NAMESPACE_QUALIFIER asUINT idx,
    T val
)
{
    constexpr std::size_t int_size = sizeof(std::decay_t<T>);

    if constexpr(int_size == sizeof(AS_NAMESPACE_QUALIFIER asBYTE))
        return ctx->SetArgByte(idx, val);
    else if constexpr(int_size == sizeof(AS_NAMESPACE_QUALIFIER asWORD))
        return ctx->SetArgWord(idx, val);
    else if constexpr(int_size == sizeof(AS_NAMESPACE_QUALIFIER asDWORD))
        return ctx->SetArgDWord(idx, val);
    else if constexpr(int_size == sizeof(AS_NAMESPACE_QUALIFIER asQWORD))
        return ctx->SetArgQWord(idx, val);
    else
        static_assert(!sizeof(T), "size of integral type is too large");
}

template <typename Enum>
requires std::is_enum_v<Enum>
int set_script_arg(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
    AS_NAMESPACE_QUALIFIER asUINT idx,
    Enum val
)
{
    using type = std::remove_cv_t<Enum>;

    constexpr bool is_customized = requires() {
        { type_traits<type>::set_arg(ctx, idx, val) } -> std::same_as<int>;
    };

    if constexpr(is_customized)
    {
        return type_traits<type>::set_arg(ctx, idx, val);
    }
    else
    {
        return set_script_arg(ctx, idx, static_cast<int>(val));
    }
}

template <std::floating_point T>
inline int set_script_arg(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
    AS_NAMESPACE_QUALIFIER asUINT idx,
    T val
)
{
    using type = std::remove_cv_t<T>;

    if constexpr(std::same_as<type, float>)
        return ctx->SetArgFloat(idx, val);
    else if constexpr(std::same_as<type, double>)
        return ctx->SetArgDouble(idx, val);
    else
        static_assert(!sizeof(T), "Invalid floating point");
}

inline int set_script_arg(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
    AS_NAMESPACE_QUALIFIER asUINT idx,
    void* obj
)
{
    return ctx->SetArgAddress(idx, obj);
}

inline int set_script_arg(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
    AS_NAMESPACE_QUALIFIER asUINT idx,
    const void* obj
)
{
    return ctx->SetArgAddress(idx, const_cast<void*>(obj));
}

inline int set_script_arg(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
    AS_NAMESPACE_QUALIFIER asUINT idx,
    AS_NAMESPACE_QUALIFIER asIScriptObject* obj
)
{
    return ctx->SetArgObject(idx, obj);
}

inline int set_script_arg(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
    AS_NAMESPACE_QUALIFIER asUINT idx,
    AS_NAMESPACE_QUALIFIER asIScriptObject const* obj
)
{
    return ctx->SetArgObject(idx, const_cast<AS_NAMESPACE_QUALIFIER asIScriptObject*>(obj));
}

template <typename Class>
requires std::is_class_v<std::remove_cvref_t<Class>>
int set_script_arg(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
    AS_NAMESPACE_QUALIFIER asUINT idx,
    Class&& obj
)
{
    using type = std::remove_cvref_t<Class>;

    constexpr bool is_customized = requires() {
        { type_traits<type>::set_script_arg(ctx, idx, obj) } -> std::same_as<int>;
    };

    if constexpr(is_customized)
    {
        return type_traits<type>::set_script_arg(ctx, idx, obj);
    }
    else
    {
        return ctx->SetArgObject(idx, (void*)std::addressof(obj));
    }
}

/**
 * @brief Apply a tuple to script context as arguments
 *
 * @param ctx Script context. The script function must be prepared.
 * @param tp Tuple of arguments
 */
template <typename Tuple>
void apply_script_args(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, Tuple&& tp)
{
    [&]<AS_NAMESPACE_QUALIFIER asUINT... Idx>(std::integer_sequence<AS_NAMESPACE_QUALIFIER asUINT, Idx...>)
    {
        (set_script_arg(ctx, Idx, std::get<Idx>(tp)), ...);
    }(std::make_integer_sequence<AS_NAMESPACE_QUALIFIER asUINT, std::tuple_size_v<Tuple>>());
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
auto get_context_result(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx)
{
    return script_invoke_result<R>(ctx);
}

/**
 * @brief Call a script function
 */
template <typename R, typename... Args>
script_invoke_result<R> script_invoke(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
    AS_NAMESPACE_QUALIFIER asIScriptFunction* func,
    Args&&... args
)
{
    assert(func != nullptr);
    assert(ctx != nullptr);

    [[maybe_unused]]
    int r = 0;
    r = ctx->Prepare(func);
    assert(r >= 0);

    apply_script_args(ctx, std::forward_as_tuple(args...));

    ctx->Execute();
    return get_context_result<R>(ctx);
}

template <typename T>
concept script_object_handle =
    std::same_as<std::remove_cvref_t<T>, AS_NAMESPACE_QUALIFIER asIScriptObject*> ||
    std::same_as<std::remove_cvref_t<T>, AS_NAMESPACE_QUALIFIER asIScriptObject const*> ||
    requires(T&& obj) {
        (AS_NAMESPACE_QUALIFIER asIScriptObject const*)obj;
    };

template <script_object_handle Object>
int set_script_object(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, Object&& obj)
{
    return ctx->SetObject(
        const_cast<AS_NAMESPACE_QUALIFIER asIScriptObject*>((AS_NAMESPACE_QUALIFIER asIScriptObject const*)obj)
    );
}

/**
 * @brief Call a method on script object
 */
template <typename R, script_object_handle Object, typename... Args>
script_invoke_result<R> script_invoke(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
    Object&& obj,
    AS_NAMESPACE_QUALIFIER asIScriptFunction* func,
    Args&&... args
)
{
    assert(func != nullptr);
    assert(ctx != nullptr);

    [[maybe_unused]]
    int r = 0;
    r = ctx->Prepare(func);
    assert(r >= 0);
    r = set_script_object(ctx, std::forward<Object>(obj));
    assert(r >= 0);

    apply_script_args(ctx, std::forward_as_tuple(std::forward<Args>(args)...));

    ctx->Execute();
    return get_context_result<R>(ctx);
}

namespace detail
{
    [[noreturn]]
    inline void throw_bad_call()
    {
        throw std::bad_function_call();
    }
} // namespace detail

template <typename T>
class script_function_ref;

/**
 * @brief Script function wrapper without ownership, i.e., no reference counting support
 */
template <typename R, typename... Args>
class script_function_ref<R(Args...)>
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asIScriptFunction*;
    using result_type = script_invoke_result<R>;

    script_function_ref() noexcept
        : script_function_ref(nullptr) {}

    script_function_ref(handle_type fp) noexcept
        : m_fp(fp) {}

    void reset(std::nullptr_t = nullptr) noexcept
    {
        m_fp = nullptr;
    }

    void reset(handle_type fp) noexcept
    {
        m_fp = fp;
    }

    handle_type target() const noexcept
    {
        return m_fp;
    }

    result_type operator()(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, Args&&... args
    ) const
    {
        handle_type fp = target();
        if(!fp) [[unlikely]]
            detail::throw_bad_call();

        return script_invoke<R>(ctx, fp, std::forward<Args>(args)...);
    }

private:
    handle_type m_fp;
};

template <typename T>
class script_method_ref;

/**
 * @brief Script method wrapper without ownership, i.e., no reference counting support
 */
template <typename R, typename... Args>
class script_method_ref<R(Args...)>
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asIScriptFunction*;
    using result_type = script_invoke_result<R>;

    script_method_ref() noexcept
        : script_method_ref(nullptr) {}

    script_method_ref(handle_type fp) noexcept
        : m_fp(fp) {}

    void reset(std::nullptr_t = nullptr) noexcept
    {
        m_fp = nullptr;
    }

    void reset(handle_type fp) noexcept
    {
        m_fp = fp;
    }

    handle_type target() const noexcept
    {
        return m_fp;
    }

    template <script_object_handle Object>
    result_type operator()(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, Object&& obj, Args&&... args
    ) const
    {
        handle_type fp = target();
        if(!fp)
            detail::throw_bad_call();

        return script_invoke<R>(ctx, std::forward<Object>(obj), fp, std::forward<Args>(args)...);
    }

private:
    handle_type m_fp;
};

template <typename T>
class script_function;

template <>
class script_function<void>
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asIScriptFunction*;

    script_function() noexcept
        : m_fp(nullptr) {}

    script_function(const script_function& other)
        : script_function(other.target()) {}

    script_function(script_function&& other) noexcept
        : m_fp(std::exchange(other.m_fp, nullptr)) {}

    script_function(handle_type fp)
        : m_fp(fp)
    {
        if(m_fp)
            m_fp->AddRef();
    }

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

    handle_type target() const noexcept
    {
        return m_fp;
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
        if(m_fp)
        {
            m_fp->Release();
            m_fp = nullptr;
        }
    }

    void reset(handle_type fp)
    {
        if(m_fp)
            m_fp->Release();
        m_fp = fp;
        if(m_fp)
            m_fp->AddRef();
    }

    void swap(script_function& other) noexcept
    {
        std::swap(m_fp, other.m_fp);
    }

private:
    handle_type m_fp;
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

    explicit script_function(handle_type fp)
        : my_base(fp) {}

    script_function& operator=(const script_function&) = default;
    script_function& operator=(script_function&&) noexcept = default;

    script_function(script_function_ref<R(Args...)> rf) noexcept
        : my_base(rf.target()) {}

    result_type operator()(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, Args&&... args
    ) const
    {
        handle_type fp = target();
        if(!fp)
            detail::throw_bad_call();

        return script_invoke<R>(ctx, fp, std::forward<Args>(args)...);
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

template <typename T>
class script_method;

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

    explicit script_method(handle_type fp)
        : my_base(fp) {}

    script_method(script_method_ref<R(Args...)> rf) noexcept
        : my_base(rf.target()) {}

    script_method& operator=(const script_method&) = default;
    script_method& operator=(script_method&&) noexcept = default;

    template <script_object_handle Object>
    result_type operator()(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, Object&& obj, Args&&... args
    ) const
    {
        handle_type fp = target();
        if(!fp)
            detail::throw_bad_call();

        return script_invoke<R>(ctx, std::forward<Object>(obj), fp, std::forward<Args>(args)...);
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

/**
 * @brief Instantiate a script class using its default factory function
 *
 * @param ctx Script context
 * @param class_info Script class type information
 *
 * @return Instantiated script object, or empty object if failed
 *
 * @note This function requires the class to be default constructible
 */
[[nodiscard]]
inline script_object instantiate_class(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
    const AS_NAMESPACE_QUALIFIER asITypeInfo* class_info
)
{
    if(!class_info) [[unlikely]]
        return script_object();

    AS_NAMESPACE_QUALIFIER asIScriptFunction* factory = nullptr;
    if(AS_NAMESPACE_QUALIFIER asQWORD flags = class_info->GetFlags();
       flags & (AS_NAMESPACE_QUALIFIER asOBJ_SCRIPT_OBJECT))
    {
        factory = get_default_factory(class_info);
    }

    if(!factory) [[unlikely]]
        return script_object();

    auto result = script_invoke<script_object>(ctx, factory);
    return result.has_value() ? *result : script_object();
}
} // namespace asbind20

#endif
