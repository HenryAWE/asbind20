#ifndef ASBIND20_INVOKE_HPP
#define ASBIND20_INVOKE_HPP

#pragma once

#include <tuple>
#include <functional>
#include "detail/include_as.hpp"
#include "utility.hpp"
#include "type_traits.hpp"

namespace asbind20
{
struct bad_result_t
{};

constexpr inline bad_result_t bad_result;

class bad_script_invoke_result_access : public std::exception
{
public:
    bad_script_invoke_result_access(int r) noexcept
        : m_r(r) {}

    const char* what() const noexcept override
    {
        return "bad script invoke result access";
    }

    int error() const noexcept
    {
        return m_r;
    }

private:
    int m_r;
};

/**
 * @brief Script invocation result
 *
 * @tparam R Result type
 */
template <typename R>
class script_invoke_result
{
public:
    using value_type = R;

    script_invoke_result(const script_invoke_result& other) noexcept(std::is_nothrow_copy_constructible_v<R>)
        : m_r(other.m_r)
    {
        if(m_r == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED)
        {
            construct_impl(*other);
        }
    }

    script_invoke_result(script_invoke_result&& other) noexcept(std::is_nothrow_move_constructible_v<R>)
        : m_r(other.m_r)
    {
        if(m_r == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED)
        {
            construct_impl(std::move(*other));
        }
    }

    explicit script_invoke_result(const R& result)
        : m_r(AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED)
    {
        construct_impl(result);
    }

    explicit script_invoke_result(R&& result) noexcept
        : m_r(AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED)
    {
        construct_impl(std::move(result));
    }

    script_invoke_result(bad_result_t, int r) noexcept
        : m_r(r)
    {
        if(r == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED) [[unlikely]]
            m_r = AS_NAMESPACE_QUALIFIER asEXECUTION_ERROR;
    }

    script_invoke_result& operator=(script_invoke_result&& other)
    {
        if(this == &other) [[unlikely]]
            return *this;

        if(!other.has_value())
        {
            destroy_impl();
            m_r = other.m_r;
            return *this;
        }

        if(has_value())
        {
            **this = std::move(*other);
        }
        else
        {
            construct_impl(std::move(*other));
            m_r = AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED;
        }

        return *this;
    }

    script_invoke_result& operator=(const script_invoke_result& other)
    {
        if(this == &other) [[unlikely]]
            return *this;

        if(!other.has_value())
        {
            destroy_impl();
            m_r = other.m_r;
            return *this;
        }

        if(has_value())
        {
            **this = *other;
        }
        else
        {
            construct_impl(*other);
            m_r = AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED;
        }

        return *this;
    }

    ~script_invoke_result()
    {
        destroy_impl();
    }

    R* operator->() noexcept
    {
        assert(has_value());
        return ptr();
    }

    const R* operator->() const noexcept
    {
        assert(has_value());
        return ptr();
    }

    R& operator*() & noexcept
    {
        assert(has_value());
        return *ptr();
    }

    R&& operator*() && noexcept
    {
        assert(has_value());
        return std::move(*ptr());
    }

    const R& operator*() const& noexcept
    {
        assert(has_value());
        return *ptr();
    }

    const R&& operator*() const&& noexcept
    {
        assert(has_value());
        return std::move(*ptr());
    }

    [[nodiscard]]
    R& value() &
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

    [[nodiscard]]
    R&& value() &&
    {
        if(!has_value())
            throw_bad_access();
        return std::move(**this);
    }

    [[nodiscard]]
    const R& value() const&
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

    [[nodiscard]]
    const R&& value() const&&
    {
        if(!has_value())
            throw_bad_access();
        return std::move(**this);
    }

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
    int error() const noexcept
    {
        return m_r;
    }

private:
    alignas(alignof(R)) std::byte m_data[sizeof(R)];
    int m_r;

    R* ptr() noexcept
    {
        return reinterpret_cast<R*>(m_data);
    }

    const R* ptr() const noexcept
    {
        return reinterpret_cast<const R*>(m_data);
    }

    template <typename... Args>
    void construct_impl(Args&&... args) noexcept(std::is_nothrow_constructible_v<R, Args...>)
    {
        new(m_data) R(std::forward<Args>(args)...);
    }

    void destroy_impl() noexcept
    {
        if(!has_value()) [[unlikely]]
            return;
        ptr()->~R();
    }

    [[noreturn]]
    void throw_bad_access() const
    {
        throw bad_script_invoke_result_access(error());
    }
};

/**
 * @brief Script invocation result for references
 */
template <typename R>
class script_invoke_result<R&>
{
public:
    using value_type = R&;

    script_invoke_result(const script_invoke_result& other) noexcept
        : m_r(other.m_r), m_ptr(other.m_ptr) {}

    explicit script_invoke_result(R& result) noexcept
        : m_ptr(std::addressof(result)),
          m_r(AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED)
    {}

    script_invoke_result(bad_result_t, int r) noexcept
        : m_r(r)
    {
        if(r == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED) [[unlikely]]
            m_r = AS_NAMESPACE_QUALIFIER asEXECUTION_ERROR;
    }

    ~script_invoke_result() = default;

    script_invoke_result& operator=(const script_invoke_result& other) noexcept
    {
        m_r = other.m_r;
        m_ptr = other.m_ptr;

        return *this;
    }

    R* operator->() noexcept
    {
        assert(has_value());
        return m_ptr;
    }

    const R* operator->() const noexcept
    {
        assert(has_value());
        return m_ptr;
    }

    R& operator*() & noexcept
    {
        assert(has_value());
        return *m_ptr;
    }

    R&& operator*() && noexcept
    {
        assert(has_value());
        return std::move(*m_ptr);
    }

    R&& operator*() const& noexcept
    {
        assert(has_value());
        return *m_ptr;
    }

    R&& operator*() const&& noexcept
    {
        assert(has_value());
        return std::move(*m_ptr);
    }

    [[nodiscard]]
    R& value() &
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

    [[nodiscard]]
    R&& value() &&
    {
        if(!has_value())
            throw_bad_access();
        return std::move(**this);
    }

    [[nodiscard]]
    const R& value() const&
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

    [[nodiscard]]
    const R&& value() const&&
    {
        if(!has_value())
            throw_bad_access();
        return std::move(**this);
    }

    [[nodiscard]]
    bool has_value() const noexcept
    {
        return error() == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED;
    }

    [[nodiscard]]
    explicit operator bool() const noexcept
    {
        return has_value();
    }

    [[nodiscard]]
    int error() const noexcept
    {
        return m_r;
    }

private:
    R* m_ptr;
    int m_r;

    [[noreturn]]
    void throw_bad_access()
    {
        throw bad_script_invoke_result_access(m_r);
    }
};

/**
 * @brief Script invocation result for void type
 */
template <>
class script_invoke_result<void>
{
public:
    using value_type = void;

    script_invoke_result() noexcept
        : m_r(AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED) {}

    script_invoke_result(const script_invoke_result& other) noexcept
        : m_r(other.m_r) {}

    script_invoke_result(bad_result_t, int r)
        : m_r(r)
    {
        if(r == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED) [[unlikely]]
            m_r = AS_NAMESPACE_QUALIFIER asEXECUTION_ERROR;
    }

    ~script_invoke_result() = default;

    script_invoke_result& operator=(const script_invoke_result& other) noexcept
    {
        m_r = other.m_r;

        return *this;
    }

    void operator*() noexcept {}

    void operator*() const noexcept {}

    [[nodiscard]]
    bool has_value() const noexcept
    {
        return m_r == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED;
    }

    explicit operator bool() const noexcept
    {
        return has_value();
    }

    void value()
    {
        if(!has_value())
            throw_bad_access();
    }

    void value() const
    {
        if(!has_value())
            throw_bad_access();
    }

    [[nodiscard]]
    int error() const noexcept
    {
        return m_r;
    }

private:
    int m_r;

    [[noreturn]]
    void throw_bad_access() const
    {
        throw bad_script_invoke_result_access(m_r);
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

namespace detail
{
    template <typename Tuple>
    void set_args(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, Tuple&& tp)
    {
        [&]<asUINT... Idx>(std::integer_sequence<asUINT, Idx...>)
        {
            (set_script_arg(ctx, Idx, std::get<Idx>(tp)), ...);
        }(std::make_integer_sequence<asUINT, std::tuple_size_v<Tuple>>());
    }

    template <typename T>
    concept is_script_obj =
        std::same_as<T, AS_NAMESPACE_QUALIFIER asIScriptObject*> ||
        std::same_as<T, AS_NAMESPACE_QUALIFIER asIScriptObject const*>;
} // namespace detail

template <typename T>
requires(!std::is_const_v<T>)
T get_script_return(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx)
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

namespace detail
{
    template <typename R>
    auto execute_context(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx)
    {
        int r = ctx->Execute();

        if(r == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED)
        {
            if constexpr(std::is_void_v<R>)
                return script_invoke_result<void>();
            else
                return script_invoke_result<R>(get_script_return<R>(ctx));
        }
        else
            return script_invoke_result<R>(bad_result, r);
    }
} // namespace detail

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

    detail::set_args(ctx, std::forward_as_tuple(args...));

    return detail::execute_context<R>(ctx);
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
    return ctx->SetObject(const_cast<AS_NAMESPACE_QUALIFIER asIScriptObject*>(
        (AS_NAMESPACE_QUALIFIER asIScriptObject const*)obj
    ));
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

    detail::set_args(ctx, std::forward_as_tuple(std::forward<Args>(args)...));

    return detail::execute_context<R>(ctx);
}

class script_function_base
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asIScriptFunction*;

    script_function_base() noexcept
        : m_fp(nullptr) {}

    script_function_base(const script_function_base& other)
        : script_function_base(other.target()) {}

    script_function_base(script_function_base&& other) noexcept
        : m_fp(std::exchange(other.m_fp, nullptr)) {}

    script_function_base(handle_type fp)
        : m_fp(fp)
    {
        if(m_fp)
            m_fp->AddRef();
    }

    ~script_function_base()
    {
        reset();
    }

    script_function_base& operator=(const script_function_base& other)
    {
        if(this == &other)
            return *this;

        reset(other.target());

        return *this;
    }

    script_function_base& operator=(script_function_base&& other) noexcept
    {
        script_function_base(std::move(other)).swap(*this);
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

    void swap(script_function_base& other) noexcept
    {
        std::swap(m_fp, other.m_fp);
    }

protected:
    [[noreturn]]
    static void throw_bad_call()
    {
        throw std::bad_function_call();
    }

private:
    handle_type m_fp;
};

template <typename... Ts>
class script_function;

/**
 * @brief Wrapper of script function
 */
template <typename R, typename... Args>
class script_function<R(Args...)> : public script_function_base
{
public:
    using result_type = script_invoke_result<R>;

    script_function() noexcept = default;
    script_function(const script_function&) = default;
    script_function(script_function&&) noexcept = default;

    explicit script_function(handle_type fp)
        : script_function_base(fp) {}

    script_function& operator=(const script_function&) = default;
    script_function& operator=(script_function&&) noexcept = default;

    result_type operator()(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, Args&&... args
    ) const
    {
        handle_type fp = target();
        if(!fp)
            throw_bad_call();

        return script_invoke<R>(ctx, fp, std::forward<Args>(args)...);
    }

    void swap(script_function& other) noexcept
    {
        script_function_base::swap(other);
    }
};

template <typename... Ts>
class script_method;

/**
 * @brief Wrapper of script method, a.k.a member function
 */
template <typename R, typename... Args>
class script_method<R(Args...)> : public script_function_base
{
public:
    using result_type = script_invoke_result<R>;

    script_method() noexcept = default;
    script_method(const script_method&) = default;
    script_method(script_method&&) noexcept = default;

    explicit script_method(handle_type fp)
        : script_function_base(fp) {}

    script_method& operator=(const script_method&) = default;
    script_method& operator=(script_method&&) noexcept = default;

    template <script_object_handle Object>
    result_type operator()(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, Object&& obj, Args&&... args
    ) const
    {
        handle_type fp = target();
        if(!fp)
            throw_bad_call();

        return script_invoke<R>(ctx, std::forward<Object>(obj), fp, std::forward<Args>(args)...);
    }

    void swap(script_method& other) noexcept
    {
        script_function_base::swap(other);
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
    AS_NAMESPACE_QUALIFIER asITypeInfo* class_info
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
    return result.has_value() ? std::move(*result) : script_object();
}
} // namespace asbind20

#endif
