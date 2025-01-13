#ifndef ASBIND20_INVOKE_HPP
#define ASBIND20_INVOKE_HPP

#pragma once

#include <tuple>
#include <functional>
#include "detail/include_as.hpp"
#include "utility.hpp"

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
        return "bad_script_invoke_result_access";
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

    script_invoke_result(const R& result)
        : m_r(asEXECUTION_FINISHED)
    {
        construct_impl(result);
    }

    script_invoke_result(R&& result)
        : m_r(asEXECUTION_FINISHED)
    {
        construct_impl(std::move(result));
    }

    script_invoke_result(bad_result_t, int r) noexcept
        : m_r(r)
    {
        if(r == asEXECUTION_FINISHED)
            m_r = asEXECUTION_ERROR;
    }

    ~script_invoke_result()
    {
        ptr()->~R();
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

    R&& operator*() const& noexcept
    {
        assert(has_value());
        return *ptr();
    }

    R&& operator*() const&& noexcept
    {
        assert(has_value());
        return std::move(*ptr());
    }

    R& value() &
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

    R&& value() &&
    {
        if(!has_value())
            throw_bad_access();
        return std::move(**this);
    }

    const R& value() const&
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

    const R&& value() const&&
    {
        if(!has_value())
            throw_bad_access();
        return std::move(**this);
    }

    bool has_value() const noexcept
    {
        return error() == asEXECUTION_FINISHED;
    }

    explicit operator bool() const noexcept
    {
        return has_value();
    }

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

    script_invoke_result(R& result)
        : m_r(asEXECUTION_FINISHED), m_ptr(std::addressof(result))
    {}

    script_invoke_result(bad_result_t, int r) noexcept
        : m_r(r)
    {
        if(r == asEXECUTION_FINISHED)
            m_r = asEXECUTION_ERROR;
    }

    ~script_invoke_result() = default;

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

    R& value() &
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

    R&& value() &&
    {
        if(!has_value())
            throw_bad_access();
        return std::move(**this);
    }

    const R& value() const&
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

    const R&& value() const&&
    {
        if(!has_value())
            throw_bad_access();
        return std::move(**this);
    }

    bool has_value() const noexcept
    {
        return error() == asEXECUTION_FINISHED;
    }

    explicit operator bool() const noexcept
    {
        return has_value();
    }

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

    script_invoke_result()
        : m_r(asEXECUTION_FINISHED) {}

    script_invoke_result(bad_result_t, int r)
        : m_r(r) {}

    void operator*() noexcept {}

    void operator*() const noexcept {}

    bool has_value() const noexcept
    {
        return m_r == asEXECUTION_FINISHED;
    }

    explicit operator bool() const noexcept
    {
        return has_value();
    }

    void value() noexcept
    {
        if(!has_value())
            throw_bad_access();
    }

    void value() const noexcept
    {
        if(!has_value())
            throw_bad_access();
    }

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

namespace detail
{
    template <typename T>
    void set_arg(asIScriptContext* ctx, asUINT idx, std::reference_wrapper<T> ref)
    {
        ctx->SetArgAddress(idx, (void*)std::addressof(ref.get()));
    }

    template <std::integral T>
    void set_arg(asIScriptContext* ctx, asUINT idx, T val)
    {
        constexpr std::size_t int_size = sizeof(std::decay_t<T>);

        if constexpr(int_size == sizeof(asBYTE))
            ctx->SetArgByte(idx, val);
        else if constexpr(int_size == sizeof(asWORD))
            ctx->SetArgWord(idx, val);
        else if constexpr(int_size == sizeof(asDWORD))
            ctx->SetArgDWord(idx, val);
        else if constexpr(int_size == sizeof(asQWORD))
            ctx->SetArgQWord(idx, val);
        else
            static_assert(!sizeof(T), "size of integral type is too large");
    }

    template <typename Enum>
    requires std::is_enum_v<std::decay_t<Enum>>
    void set_arg(asIScriptContext* ctx, asUINT idx, Enum val)
    {
        using T = std::remove_cvref_t<Enum>;

        constexpr bool is_customized = requires() {
            { type_traits<T>::set_arg(ctx, idx, val) } -> std::same_as<int>;
        };

        if constexpr(is_customized)
        {
            type_traits<T>::set_arg(ctx, idx, val);
        }
        else
        {
            static_assert(sizeof(T) <= sizeof(int), "underlying type of enum is too large");
            set_arg(ctx, idx, static_cast<int>(val));
        }
    }

    inline void set_arg(asIScriptContext* ctx, asUINT idx, float val)
    {
        ctx->SetArgFloat(idx, val);
    }

    inline void set_arg(asIScriptContext* ctx, asUINT idx, double val)
    {
        ctx->SetArgDouble(idx, val);
    }

    inline void set_arg(asIScriptContext* ctx, asUINT idx, void* obj)
    {
        ctx->SetArgAddress(idx, obj);
    }

    inline void set_arg(asIScriptContext* ctx, asUINT idx, const void* obj)
    {
        ctx->SetArgAddress(idx, const_cast<void*>(obj));
    }

    inline void set_arg(asIScriptContext* ctx, asUINT idx, asIScriptObject* obj)
    {
        ctx->SetArgObject(idx, obj);
    }

    inline void set_arg(asIScriptContext* ctx, asUINT idx, const asIScriptObject* obj)
    {
        ctx->SetArgObject(idx, const_cast<asIScriptObject*>(obj));
    }

    template <typename Class>
    requires std::is_class_v<std::remove_cvref_t<Class>>
    void set_arg(asIScriptContext* ctx, asUINT idx, Class&& obj)
    {
        using T = std::remove_cvref_t<Class>;

        constexpr bool is_customized = requires() {
            { type_traits<T>::set_arg(ctx, idx, obj) } -> std::same_as<int>;
        };

        if constexpr(is_customized)
        {
            type_traits<T>::set_arg(ctx, idx, obj);
        }
        else
        {
            ctx->SetArgObject(idx, (void*)std::addressof(obj));
        }
    }

    template <typename Tuple>
    void set_args(asIScriptContext* ctx, Tuple&& tp)
    {
        [&]<asUINT... Idx>(std::integer_sequence<asUINT, Idx...>)
        {
            (set_arg(ctx, Idx, std::get<Idx>(tp)), ...);
        }(std::make_integer_sequence<asUINT, std::tuple_size_v<Tuple>>());
    }

    template <typename T>
    concept is_script_obj =
        std::is_same_v<T, asIScriptObject*> ||
        std::is_same_v<T, const asIScriptObject*>;

    template <typename T>
    requires(!std::is_const_v<T>)
    T get_ret(asIScriptContext* ctx)
    {
        constexpr bool is_customized = requires() {
            { type_traits<T>::get_return(ctx) } -> std::convertible_to<T>;
        };

        if constexpr(is_customized)
        {
            return type_traits<T>::get_return(ctx);
        }
        else if constexpr(is_script_obj<std::remove_cvref_t<T>>)
        {
            asIScriptObject* ptr = *reinterpret_cast<asIScriptObject**>(ctx->GetAddressOfReturnValue());
            return T(ptr);
        }
        else if constexpr(std::is_reference_v<T>)
        {
            using ptr_t = std::add_pointer_t<std::remove_reference_t<T>>;
            return *reinterpret_cast<ptr_t>(ctx->GetReturnAddress());
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
            else if constexpr(std::is_same_v<primitive_t, float>)
            {
                return ctx->GetReturnFloat();
            }
            else if constexpr(std::is_same_v<primitive_t, double>)
            {
                return ctx->GetReturnDouble();
            }
            else
                static_assert(!sizeof(T), "Invalid type");
        }
    }

    template <typename R>
    auto get_invoke_result(asIScriptContext* ctx)
    {
        int r = ctx->Execute();

        if(r == asEXECUTION_FINISHED)
        {
            if constexpr(std::is_void_v<R>)
                return script_invoke_result<void>();
            else
                return script_invoke_result<R>(detail::get_ret<R>(ctx));
        }
        else
            return script_invoke_result<R>(bad_result, r);
    }
} // namespace detail

/**
 * @brief Call a script function
 */
template <typename R, typename... Args>
script_invoke_result<R> script_invoke(asIScriptContext* ctx, asIScriptFunction* func, Args&&... args)
{
    assert(func != nullptr);
    assert(ctx != nullptr);

    [[maybe_unused]]
    int r = 0;
    r = ctx->Prepare(func);
    assert(r >= 0);

    detail::set_args(ctx, std::forward_as_tuple(args...));

    return detail::get_invoke_result<R>(ctx);
}

template <typename T>
concept script_object_handle =
    std::is_same_v<std::remove_cvref_t<T>, asIScriptObject*> ||
    std::is_same_v<std::remove_cvref_t<T>, const asIScriptObject*> ||
    requires(T&& obj) {
        (const asIScriptObject*)obj;
    };

/**
 * @brief Calling a method on script class
 */
template <typename R, script_object_handle Object, typename... Args>
script_invoke_result<R> script_invoke(asIScriptContext* ctx, Object&& obj, asIScriptFunction* func, Args&&... args)
{
    assert(func != nullptr);
    assert(ctx != nullptr);

    int r = 0;
    r = ctx->Prepare(func);
    assert(r >= 0);
    r = ctx->SetObject(const_cast<asIScriptObject*>((const asIScriptObject*)obj));
    assert(r >= 0);

    detail::set_args(ctx, std::forward_as_tuple(std::forward<Args>(args)...));

    return detail::get_invoke_result<R>(ctx);
}

class script_function_base
{
protected:
    script_function_base() noexcept
        : m_fp(nullptr) {}

    script_function_base(const script_function_base& other)
        : script_function_base(other.target()) {}

    script_function_base(script_function_base&& other)
        : m_fp(std::exchange(other.m_fp, nullptr)) {}

    script_function_base(asIScriptFunction* fp)
        : m_fp(fp)
    {
        if(m_fp)
            m_fp->AddRef();
    }

public:
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

    asIScriptFunction* target() const noexcept
    {
        return m_fp;
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(target());
    }

    explicit operator asIScriptFunction*() const noexcept
    {
        return target();
    }

    int reset(std::nullptr_t = nullptr) noexcept
    {
        int prev_refcount = 0;
        if(m_fp)
        {
            prev_refcount = m_fp->Release();
            m_fp = nullptr;
        }

        return prev_refcount;
    }

    int reset(asIScriptFunction* fp)
    {
        int prev_refcount = reset(nullptr);
        m_fp = fp;
        if(m_fp)
            m_fp->AddRef();

        return prev_refcount;
    }

protected:
    [[noreturn]]
    static void throw_bad_call()
    {
        throw std::bad_function_call();
    }

    void swap(script_function_base& other) noexcept
    {
        std::swap(m_fp, other.m_fp);
    }

private:
    asIScriptFunction* m_fp;
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

    explicit script_function(asIScriptFunction* fp)
        : script_function_base(fp) {}

    script_function& operator=(const script_function&) = default;
    script_function& operator=(script_function&&) noexcept = default;

    result_type operator()(asIScriptContext* ctx, Args&&... args) const
    {
        asIScriptFunction* fp = target();
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

    explicit script_method(asIScriptFunction* fp)
        : script_function_base(fp) {}

    script_method& operator=(const script_method&) = default;
    script_method& operator=(script_method&&) noexcept = default;

    template <script_object_handle Object>
    result_type operator()(asIScriptContext* ctx, Object&& obj, Args&&... args) const
    {
        asIScriptFunction* fp = target();
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
 * @return object Instantiated script object, or empty object if failed
 *
 * @note This function requires the class to be default constructible
 */
[[nodiscard]]
inline script_object instantiate_class(asIScriptContext* ctx, asITypeInfo* class_info)
{
    if(!class_info) [[unlikely]]
        return script_object();

    asIScriptFunction* factory = nullptr;
    if(asQWORD flags = class_info->GetFlags(); flags & asOBJ_REF)
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
