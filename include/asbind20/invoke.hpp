
#include <variant>
#include <tuple>
#include <angelscript.h>

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

    [[nodiscard]]
    void throw_bad_access() const
    {
        throw bad_script_invoke_result_access(error());
    }
};

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

    [[nodiscard]]
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
            ptr->AddRef();

            if constexpr(std::is_same_v<std::remove_cvref_t<T>, object>)
                return object(ptr, false);
            else
                return ptr;
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
            using ret_t = std::remove_cvref_t<T>;
            if constexpr(std::integral<ret_t>)
            {
                if constexpr(sizeof(ret_t) == 1)
                    return ctx->GetReturnByte();
                else if constexpr(sizeof(ret_t) == 2)
                    return ctx->GetReturnWord();
                else if constexpr(sizeof(ret_t) == 4)
                    return ctx->GetReturnDWord();
                else if constexpr(sizeof(ret_t) == 8)
                    return ctx->GetReturnQWord();
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

template <typename R, typename... Args>
script_invoke_result<R> script_invoke(asIScriptFunction* func, asIScriptContext* ctx, Args&&... args)
{
    assert(func != nullptr);
    assert(ctx != nullptr);

    ctx->Prepare(func);
    detail::set_args(ctx, std::forward_as_tuple(args...));

    return detail::execute_impl<R>(func, ctx);
}

// Calling a method on script class
template <typename R, typename... Args>
script_invoke_result<R> script_invoke(asIScriptObject* obj, asIScriptFunction* func, asIScriptContext* ctx, Args&&... args)
{
    assert(func != nullptr);
    assert(ctx != nullptr);
    assert(obj->GetTypeId() == func->GetObjectType()->GetTypeId());

    ctx->Prepare(func);
    ctx->SetObject(obj);

    detail::set_args(ctx, std::forward_as_tuple(args...));

    return detail::execute_impl<R>(func, ctx);
}

// Calling a method on script class
template <typename R, typename... Args>
script_invoke_result<R> script_invoke(const object& obj, asIScriptFunction* func, asIScriptContext* ctx, Args&&... args)
{
    return script_invoke<R>(obj.get(), func, ctx, std::forward<Args>(args)...);
}
} // namespace asbind20
