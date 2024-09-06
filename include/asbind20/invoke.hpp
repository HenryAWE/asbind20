#ifndef ASBIND20_INVOKE_HPP
#define ASBIND20_INVOKE_HPP

#pragma once

#include <variant>
#include <tuple>
#include <functional>
#include <angelscript.h>
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

    const void operator*() const noexcept {}

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

    const void value() const noexcept
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
        ctx->SetArgAddress(idx, std::addressof(ref.get()));
    }

    template <std::integral T>
    requires(sizeof(T) <= sizeof(asQWORD))
    void set_arg(asIScriptContext* ctx, asUINT idx, T val)
    {
        using arg_t = std::remove_cvref_t<T>;

        if constexpr(sizeof(arg_t) == sizeof(asBYTE))
            ctx->SetArgByte(idx, val);
        else if constexpr(sizeof(arg_t) == sizeof(asWORD))
            ctx->SetArgWord(idx, val);
        else if constexpr(sizeof(arg_t) == sizeof(asDWORD))
            ctx->SetArgDWord(idx, val);
        else if constexpr(sizeof(arg_t) == sizeof(asQWORD))
            ctx->SetArgQWord(idx, val);
    }

    template <typename Enum>
    requires std::is_enum_v<Enum>
    void set_arg(asIScriptContext* ctx, asUINT idx, Enum val)
    {
        static_assert(sizeof(Enum) <= sizeof(int), "Value out of range");
        set_arg(ctx, idx, static_cast<int>(val));
    }

    inline void set_arg(asIScriptContext* ctx, asUINT idx, float val)
    {
        ctx->SetArgFloat(idx, val);
    }

    inline void set_arg(asIScriptContext* ctx, asUINT idx, double val)
    {
        ctx->SetArgDouble(idx, val);
    }

    inline void set_arg(asIScriptContext* ctx, asUINT idx, asIScriptObject* obj)
    {
        ctx->SetArgAddress(idx, obj);
    }

    template <typename Class>
    requires std::is_class_v<std::remove_cvref_t<Class>>
    void set_arg(asIScriptContext* ctx, asUINT idx, Class&& obj)
    {
        ctx->SetArgAddress(idx, std::addressof(obj));
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
        std::is_same_v<T, object>;

    template <typename T>
    requires(!std::is_const_v<T>)
    T get_ret(asIScriptContext* ctx)
    {
        if constexpr(is_script_obj<std::remove_cvref_t<T>>)
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
            using ret_t = typename std::conditional_t<
                std::is_enum_v<T>,
                std::underlying_type<T>,
                std::remove_cvref<T>>::type;
            if constexpr(std::integral<ret_t>)
            {
                if constexpr(sizeof(ret_t) == 1)
                    return static_cast<T>(ctx->GetReturnByte());
                else if constexpr(sizeof(ret_t) == 2)
                    return static_cast<T>(ctx->GetReturnWord());
                else if constexpr(sizeof(ret_t) == 4)
                    return static_cast<T>(ctx->GetReturnDWord());
                else if constexpr(sizeof(ret_t) == 8)
                    return static_cast<T>(ctx->GetReturnQWord());
            }
            else if constexpr(std::is_same_v<ret_t, float>)
            {
                return ctx->GetReturnFloat();
            }
            else if constexpr(std::is_same_v<ret_t, double>)
            {
                return ctx->GetReturnDouble();
            }
        }
    }

    template <typename R, typename... Args>
    auto execute_impl(asIScriptFunction* func, asIScriptContext* ctx)
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

    ctx->Prepare(func);
    detail::set_args(ctx, std::forward_as_tuple(args...));

    return detail::execute_impl<R>(func, ctx);
}

/**
 * @brief Calling a method on script class
 */
template <typename R, typename... Args>
script_invoke_result<R> script_invoke(asIScriptContext* ctx, asIScriptObject* obj, asIScriptFunction* func, Args&&... args)
{
    assert(func != nullptr);
    assert(ctx != nullptr);
    //assert(obj->GetTypeId() == func->GetObjectType()->GetTypeId());

    ctx->Prepare(func);
    ctx->SetObject(obj);

    detail::set_args(ctx, std::forward_as_tuple(args...));

    return detail::execute_impl<R>(func, ctx);
}

/**
 * @brief Calling a method on script class
 */
template <typename R, typename... Args>
script_invoke_result<R> script_invoke(asIScriptContext* ctx, const object& obj, asIScriptFunction* func, Args&&... args)
{
    return script_invoke<R>(ctx, obj.get(), func, std::forward<Args>(args)...);
}

namespace detail
{
    class script_function_base
    {
    public:
        script_function_base() noexcept
            : m_fp(nullptr) {}

        script_function_base(const script_function_base&) noexcept = default;

        script_function_base(asIScriptFunction* fp)
            : m_fp(fp) {}

        script_function_base& operator=(const script_function_base&) noexcept = default;

        asIScriptFunction* target() const noexcept
        {
            return m_fp;
        }

        explicit operator bool() const noexcept
        {
            return static_cast<bool>(m_fp);
        }

    protected:
        [[noreturn]]
        static void throw_bad_call()
        {
            throw std::bad_function_call();
        }

        void set_target(asIScriptFunction* fp) noexcept
        {
            m_fp = fp;
        }

    private:
        asIScriptFunction* m_fp;
    };

    template <bool IsMethod, typename R, typename... Args>
    class script_function_impl;

    template <typename R, typename... Args>
    class script_function_impl<false, R, Args...> : public script_function_base
    {
        using my_base = script_function_base;

    public:
        using my_base::my_base;

        auto operator()(asIScriptContext* ctx, Args&&... args) const
        {
            asIScriptFunction* fp = target();
            if(!fp)
                throw_bad_call();

            auto result = script_invoke<R>(ctx, target(), std::forward<Args>(args)...);

            return std::move(result).value();
        }
    };

    template <typename R, typename... Args>
    class script_function_impl<true, R, Args...> : public script_function_base
    {
        using my_base = script_function_base;

    public:
        using my_base::my_base;

        auto operator()(asIScriptContext* ctx, asIScriptObject* obj, Args&&... args) const
        {
            asIScriptFunction* fp = target();
            if(!fp)
                throw_bad_call();

            auto result = script_invoke<R>(ctx, obj, target(), std::forward<Args>(args)...);

            return std::move(result).value();
        }

        auto operator()(asIScriptContext* ctx, const object& obj, Args&&... args) const
        {
            return (*this)(ctx, obj.get(), std::forward<Args>(args)...);
        }
    };
} // namespace detail


template <typename... Ts>
class script_function;

/**
 * @brief Wrapper of script function
 */
template <typename R, typename... Args>
class script_function<R(Args...)> : public detail::script_function_impl<false, R, Args...>
{
    using my_base = detail::script_function_impl<false, R, Args...>;

public:
    using my_base::my_base;

    script_function& operator=(const script_function&) noexcept = default;

    script_function& operator=(asIScriptFunction* fp) noexcept
    {
        this->set_target(fp);
        return *this;
    }
};

template <typename... Ts>
class script_method;

/**
 * @brief Wrapper of script method, a.k.a member function
 */
template <typename R, typename... Args>
class script_method<R(Args...)> : public detail::script_function_impl<true, R, Args...>
{
    using my_base = detail::script_function_impl<true, R, Args...>;

public:
    using my_base::my_base;

    script_method& operator=(const script_method&) noexcept = default;

    script_method& operator=(asIScriptFunction* fp) noexcept
    {
        this->set_target(fp);
        return *this;
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
inline object instantiate_class(asIScriptContext* ctx, asITypeInfo* class_info)
{
    if(!class_info) [[unlikely]]
        return object();

    asIScriptFunction* factory = nullptr;
    if(int flags = class_info->GetFlags(); flags & asOBJ_REF)
    {
        for(asUINT i = 0; i < class_info->GetFactoryCount(); ++i)
        {
            asIScriptFunction* fp = class_info->GetFactoryByIndex(i);
            if(fp->GetParamCount() == 0) // Default factory
            {
                factory = fp;
                break;
            }
        }
    }

    if(!factory) [[unlikely]]
        return object();

    auto result = script_invoke<object>(ctx, factory);
    return result.has_value() ? std::move(*result) : object();
}

} // namespace asbind20

#endif
