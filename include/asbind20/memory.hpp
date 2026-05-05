/**
 * @file memory.hpp
 * @author HenryAWE
 * @brief Tools for memory management
 */

#ifndef ASBIND20_MEMORY_HPP
#define ASBIND20_MEMORY_HPP

#pragma once

#include <cassert>
#include <cstddef>
#include <new>
#include <utility>
#include <type_traits>
#include <string>
#include "detail/err_handler.hpp"
#include "detail/include_as.hpp"
#include "utility.hpp"
#include "debugging/gc_statistics.hpp"

namespace asbind20
{
/**
 * @brief Wrap `asAllocMem()` and `asFreeMem()` as a C++ allocator
 */
template <typename T>
class script_allocator
{
public:
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    constexpr script_allocator() noexcept = default;

    template <typename U>
    constexpr script_allocator(const script_allocator<U>&) noexcept
    {}

    constexpr script_allocator& operator=(const script_allocator&) noexcept = default;

    constexpr ~script_allocator() = default;

    [[nodiscard]]
    static constexpr pointer allocate(size_type n)
    {
        if(std::is_constant_evaluated())
        {
            std::allocator<T> tmp;
            return tmp.allocate(n);
        }
        else
        {
            check_length(n);

            void* mem = AS_NAMESPACE_QUALIFIER asAllocMem(n * sizeof(T));
            if(!mem) [[unlikely]]
                asbind20::detail::throw_<std::bad_alloc>();
            return pointer(mem);
        }
    }

    static constexpr void deallocate(pointer mem, size_type n) noexcept
    {
        if(std::is_constant_evaluated())
        {
            std::allocator<T> tmp;
            tmp.deallocate(mem, n);
        }
        else
        {
            (void)n; // unused
            AS_NAMESPACE_QUALIFIER asFreeMem(static_cast<void*>(mem));
        }
    }

private:
    static void check_length(size_type n)
    {
        if(std::numeric_limits<size_type>::max() / sizeof(T) < n) [[unlikely]]
            detail::throw_<std::bad_array_new_length>();
    }
};

/**
 * @brief Smart pointer for script object
 */
class script_object
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asIScriptObject*;

    script_object() noexcept = default;

    script_object(script_object&& other) noexcept
        : m_obj(std::exchange(other.m_obj, nullptr)) {}

    script_object(const script_object&) = delete;

    explicit script_object(handle_type obj)
        : m_obj(obj)
    {
        if(m_obj)
            m_obj->AddRef();
    }

    ~script_object()
    {
        reset();
    }

    [[nodiscard]]
    handle_type get() const noexcept
    {
        return m_obj;
    }

    explicit operator handle_type() const noexcept
    {
        return get();
    }

    explicit operator bool() const noexcept
    {
        return get() != nullptr;
    }

    handle_type operator->() const noexcept
    {
        return get();
    }

    /**
     * @brief Release without decreasing reference count
     *
     * @warning USE WITH CAUTION!
     *
     * @return Previously stored object
     */
    [[nodiscard]]
    handle_type release() noexcept
    {
        return std::exchange(m_obj, nullptr);
    }

    /**
     * @brief Reset object the null pointer
     */
    void reset(std::nullptr_t = nullptr) noexcept
    {
        if(m_obj)
        {
            m_obj->Release();
            m_obj = nullptr;
        }
    }

    /**
     * @brief Reset object
     *
     * @param obj New object to store
     */
    void reset(handle_type obj)
    {
        if(m_obj)
            m_obj->Release();
        m_obj = obj;
        if(obj)
            obj->AddRef();
    }

private:
    handle_type m_obj = nullptr;
};

/**
 * @brief RAII helper for reusing active script context.
 *
 * It will fallback to request context from the engine.
 */
class [[nodiscard]] reuse_active_context
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asIScriptContext*;

    reuse_active_context() = delete;
    reuse_active_context(const reuse_active_context&) = delete;

    reuse_active_context& operator=(const reuse_active_context&) = delete;

    explicit reuse_active_context(std::nullptr_t) = delete;

    explicit reuse_active_context(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, bool propagate_error = true
    )
        : m_engine(engine), m_propagate_error(propagate_error)
    {
        ASBIND20_ASSERT(m_engine != nullptr);

        m_ctx = current_context();
        if(m_ctx) [[likely]]
        {
            ASBIND20_ASSERT(m_ctx->GetEngine() == engine);
            if(m_ctx->PushState() >= 0)
            {
                m_is_nested = true;
                return;
            }
        }

        // Fallback
        m_ctx = engine->RequestContext();
    }

    ~reuse_active_context()
    {
        if(!m_ctx) [[unlikely]]
            return;

        if(!m_is_nested)
        {
            m_engine->ReturnContext(m_ctx);
            return;
        }

        pop_state_impl();
    }

    [[nodiscard]]
    handle_type get() const noexcept
    {
        return m_ctx;
    }

    [[nodiscard]]
    auto get_engine() const noexcept
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine;
    }

    operator handle_type() const noexcept
    {
        return get();
    }

    handle_type operator->() const noexcept
    {
        return get();
    }

    /**
     * @brief Returns true if current context is reused.
     */
    [[nodiscard]]
    bool is_nested() const noexcept
    {
        return m_is_nested;
    }

    [[nodiscard]]
    bool will_propagate_error() const noexcept
    {
        return m_propagate_error;
    }

private:
    AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine = nullptr;
    handle_type m_ctx = nullptr;
    bool m_is_nested = false;
    bool m_propagate_error = true;

    void pop_state_impl() const
    {
        ASBIND20_ASSERT(m_is_nested);

        if(!m_propagate_error)
        {
            m_ctx->PopState();
            return;
        }

        // Propagating error
        std::string ex;
        AS_NAMESPACE_QUALIFIER asEContextState state =
            m_ctx->GetState();
        if(state == AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION)
            ex = m_ctx->GetExceptionString();

        m_ctx->PopState();

        switch(state)
        {
        case AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION:
            m_ctx->SetException(ex.c_str());
            break;

        case AS_NAMESPACE_QUALIFIER asEXECUTION_ABORTED:
            m_ctx->Abort();
            break;

        default:
            break;
        }
    }
};

/**
 * @brief RAII helper for requesting script context from the engine
 */
class [[nodiscard]] request_context
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asIScriptContext*;

    request_context() = delete;
    request_context(const request_context&) = delete;

    request_context& operator=(const request_context&) = delete;

    explicit request_context(std::nullptr_t) = delete;

    explicit request_context(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
        : m_engine(engine)
    {
        ASBIND20_ASSERT(m_engine != nullptr);
        m_ctx = m_engine->RequestContext();
    }

    ~request_context()
    {
        if(m_ctx) [[likely]]
            m_engine->ReturnContext(m_ctx);
    }

    [[nodiscard]]
    handle_type get() const noexcept
    {
        return m_ctx;
    }

    [[nodiscard]]
    auto get_engine() const noexcept
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine;
    }

    operator handle_type() const noexcept
    {
        return get();
    }

    handle_type operator->() const noexcept
    {
        return get();
    }

private:
    AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine = nullptr;
    handle_type m_ctx = nullptr;
};

/**
 * @brief RAII helper for script context
 */
class script_context
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asIScriptContext*;

    script_context() noexcept = default;

    script_context(const script_context& other)
    {
        reset(other.get());
    }

    script_context(script_context&& other) noexcept
        : m_ctx(std::exchange(other.m_ctx, nullptr))
    {}

    /**
     * @brief Create context from the script engine.
     *
     * @param engine Script engine. If it is null, the context will not be created.
     */
    explicit script_context(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
    )
    {
        if(!engine) [[unlikely]]
            return;
        reset(std::in_place, engine->CreateContext());
    }

    ~script_context()
    {
        reset(nullptr);
    }

    script_context& operator=(const script_context& rhs)
    {
        if(this == &rhs)
            return *this;
        reset(rhs.get());
        return *this;
    }

    script_context& operator=(script_context&& rhs) noexcept
    {
        if(this == &rhs)
            return *this;
        reset(nullptr);
        m_ctx = std::exchange(rhs.m_ctx, nullptr);
        return *this;
    }

    [[nodiscard]]
    handle_type get() const noexcept
    {
        return m_ctx;
    }

    operator handle_type() const noexcept
    {
        return get();
    }

    explicit operator bool() const noexcept
    {
        return m_ctx != nullptr;
    }

    [[nodiscard]]
    handle_type release() noexcept
    {
        return std::exchange(m_ctx, nullptr);
    }

    void reset(std::nullptr_t) noexcept
    {
        if(m_ctx)
            m_ctx->Release();
        m_ctx = nullptr;
    }

    void reset(std::in_place_t, handle_type ctx)
    {
        if(m_ctx)
            m_ctx->Release();
        m_ctx = ctx;
    }

    void reset(handle_type ctx = nullptr)
    {
        reset(std::in_place, ctx);
        if(m_ctx)
            m_ctx->AddRef();
    }

    void swap(script_context& other) noexcept
    {
        std::swap(m_ctx, other.m_ctx);
    }

private:
    handle_type m_ctx = nullptr;
};

/**
 * @brief Script engine manager
 */
class script_engine
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asIScriptEngine*;

    script_engine() noexcept
        : m_engine(nullptr) {}

    script_engine(const script_engine&) = delete;

    script_engine(script_engine&&) noexcept
        : m_engine(std::exchange(m_engine, nullptr)) {}

    explicit script_engine(handle_type engine) noexcept
        : m_engine(engine) {}

    script_engine& operator=(const script_engine&) = delete;

    script_engine& operator=(script_engine&& other) noexcept
    {
        if(this != &other)
            reset(other.release());
        return *this;
    }

    ~script_engine()
    {
        reset();
    }

    [[nodiscard]]
    handle_type get() const noexcept
    {
        return m_engine;
    }

    operator handle_type() const noexcept
    {
        return get();
    }

    handle_type operator->() const noexcept
    {
        return get();
    }

    [[nodiscard]]
    handle_type release() noexcept
    {
        return std::exchange(m_engine, nullptr);
    }

    void reset(handle_type engine = nullptr) noexcept
    {
        if(m_engine)
            m_engine->ShutDownAndRelease();
        m_engine = engine;
    }

private:
    handle_type m_engine;
};

/**
 * @brief Create an AngelScript engine
 */
[[nodiscard]]
inline script_engine make_script_engine(
    AS_NAMESPACE_QUALIFIER asDWORD version = ANGELSCRIPT_VERSION
)
{
    return script_engine(
        AS_NAMESPACE_QUALIFIER asCreateScriptEngine(version)
    );
}

/**
 * @brief Helper for `asILockableSharedBool*`
 *
 * This class can be helpful for implementing weak reference support.
 */
class lockable_shared_bool
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asILockableSharedBool*;

    lockable_shared_bool() noexcept = default;

    explicit lockable_shared_bool(handle_type bool_)
    {
        reset(bool_);
    }

    lockable_shared_bool(std::in_place_t, handle_type bool_) noexcept
    {
        reset(std::in_place, bool_);
    }

    lockable_shared_bool(const lockable_shared_bool& other)
    {
        reset(other.m_bool);
    }

    lockable_shared_bool(lockable_shared_bool&& other) noexcept
        : m_bool(std::exchange(other.m_bool, nullptr)) {}

    ~lockable_shared_bool()
    {
        reset(nullptr);
    }

    bool operator==(const lockable_shared_bool& other) const = default;

    void reset(std::nullptr_t = nullptr) noexcept
    {
        if(m_bool)
        {
            m_bool->Release();
            m_bool = nullptr;
        }
    }

    void reset(handle_type bool_) noexcept
    {
        if(m_bool)
            m_bool->Release();
        m_bool = bool_;
        if(m_bool)
            m_bool->AddRef();
    }

    /**
      * @brief Connect to the weak reference flag of object
      *
      * @param obj Object to connect
      * @param ti Type information
      *
      * @note If it failed to connect, this helper will be reset to nullptr.
      */
    void connect_object(void* obj, AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
    {
        if(!ti) [[unlikely]]
        {
            reset();
            return;
        }

        reset(ti->GetEngine()->GetWeakRefFlagOfScriptObject(obj, ti));
    }

    /**
     * @warning If you get the lockable shared bool by `GetWeakRefFlagOfScriptObject()`,
     *          you should @b not use this function! Because it won't increase the reference count.
     *
     * @sa connect_object
     */
    void reset(std::in_place_t, handle_type bool_) noexcept
    {
        if(m_bool)
            m_bool->Release();
        m_bool = bool_;
    }

    lockable_shared_bool& operator=(const lockable_shared_bool& other)
    {
        if(this == &other)
            return *this;

        reset(nullptr);
        if(other.m_bool)
        {
            other.m_bool->AddRef();
            m_bool = other.m_bool;
        }

        return *this;
    }

    lockable_shared_bool& operator=(lockable_shared_bool&& other)
    {
        if(this == &other)
            return *this;

        lockable_shared_bool(std::move(other)).swap(*this);
        return *this;
    }

    /**
     * @brief Lock the flag
     */
    void lock() const
    {
        ASBIND20_ASSERT(*this);
        m_bool->Lock();
    }

    /**
     * @brief Unlock the flag
     */
    void unlock() const noexcept
    {
        ASBIND20_ASSERT(*this);
        m_bool->Unlock();
    }

    [[nodiscard]]
    bool get_flag() const
    {
        ASBIND20_ASSERT(*this);
        return m_bool->Get();
    }

    void set_flag(bool value = true)
    {
        m_bool->Set(value);
    }

    [[nodiscard]]
    handle_type get() const noexcept
    {
        return m_bool;
    }

    handle_type operator->() const noexcept
    {
        return m_bool;
    }

    operator handle_type() const noexcept
    {
        return get();
    }

    explicit operator bool() const noexcept
    {
        return m_bool != nullptr;
    }

    void swap(lockable_shared_bool& other) noexcept
    {
        std::swap(m_bool, other.m_bool);
    }

private:
    handle_type m_bool = nullptr;
};

/**
 * @brief Create a lockable shared bool for implementing weak reference
 *
 * @note Lock the exclusive lock in multithreading enviornment
 */
[[nodiscard]]
inline lockable_shared_bool make_lockable_shared_bool()
{
    return lockable_shared_bool(
        std::in_place, AS_NAMESPACE_QUALIFIER asCreateLockableSharedBool()
    );
}

/**
 * @brief RAII helper for `asITypeInfo*`.
 */
class script_typeinfo
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asITypeInfo*;

    script_typeinfo() noexcept = default;

    /**
     * @brief Assign a type info object. It @b won't increase the reference count!
     * @sa script_typeinfo(inplace_addref_t, handle_type)
     *
     * @warning DON'T use this constructor unless you know what you are doing!
     *
     * @note Generally, the AngelScript APIs for getting type info won't increase reference count,
     *       such as being the hidden first argument of template class constructor/factory.
     */
    explicit script_typeinfo(std::in_place_t, handle_type ti) noexcept
        : m_ti(ti) {}

    /**
     * @brief Assign a type info object, and increase reference count
     */
    script_typeinfo(handle_type ti) noexcept
        : m_ti(ti)
    {
        if(m_ti)
            m_ti->AddRef();
    }

    script_typeinfo(const script_typeinfo& other) noexcept
        : m_ti(other.m_ti)
    {
        if(m_ti)
            m_ti->AddRef();
    }

    script_typeinfo(script_typeinfo&& other) noexcept
        : m_ti(std::exchange(other.m_ti, nullptr)) {}

    ~script_typeinfo()
    {
        reset();
    }

    script_typeinfo& operator=(const script_typeinfo& other) noexcept
    {
        if(this != &other)
            reset(other.m_ti);
        return *this;
    }

    script_typeinfo& operator=(script_typeinfo&& other) noexcept
    {
        if(this != &other)
            reset(other.release());
        return *this;
    }

    [[nodiscard]]
    handle_type get() const noexcept
    {
        return m_ti;
    }

    handle_type operator->() const noexcept
    {
        return get();
    }

    operator handle_type() const noexcept
    {
        return get();
    }

    explicit operator bool() const noexcept
    {
        return m_ti != nullptr;
    }

    [[nodiscard]]
    handle_type release() noexcept
    {
        return std::exchange(m_ti, nullptr);
    }

    void reset(std::nullptr_t = nullptr) noexcept
    {
        if(m_ti)
        {
            m_ti->Release();
            m_ti = nullptr;
        }
    }

    void reset(handle_type ti)
    {
        if(m_ti)
            m_ti->Release();
        m_ti = ti;
        if(m_ti)
            m_ti->AddRef();
    }

    void reset(std::in_place_t, handle_type ti)
    {
        if(m_ti)
            m_ti->Release();
        m_ti = ti;
    }

    [[nodiscard]]
    int type_id() const
    {
        if(!m_ti) [[unlikely]]
            return AS_NAMESPACE_QUALIFIER asINVALID_ARG;

        return m_ti->GetTypeId();
    }

    [[nodiscard]]
    int subtype_id(AS_NAMESPACE_QUALIFIER asUINT idx = 0) const
    {
        if(!m_ti) [[unlikely]]
            return AS_NAMESPACE_QUALIFIER asINVALID_ARG;

        return m_ti->GetSubTypeId(idx);
    }

    [[nodiscard]]
    auto subtype(AS_NAMESPACE_QUALIFIER asUINT idx = 0) const
        -> AS_NAMESPACE_QUALIFIER asITypeInfo*
    {
        if(!m_ti) [[unlikely]]
            return nullptr;

        return m_ti->GetSubType(idx);
    }

private:
    handle_type m_ti = nullptr;
};

namespace container
{
    /**
     * @brief A set of helper for storing a single script object
     */
    class single
    {
    public:
        /**
         * @brief Helper for storing data
         *
         * @note This helper needs an external type ID for correctly handle the stored data,
         *       so it is recommended to use this helper as a member of container class, together with a member for storing type ID.
         */
        union data_type
        {
            /** primitive value */
            std::byte primitive[8];
            /** script handle */
            void* handle;
            /** script object */
            void* ptr;

            data_type()
                : ptr(nullptr) {}

            data_type(const data_type&) = delete;

            data_type(data_type&& other) noexcept
            {
                std::memcpy((void*)this, &other, sizeof(data_type));
                other.ptr = nullptr;
            }

            /**
             * @warning Due to limitations of the AngelScript interface, it won't properly release the stored object.
             *          Remember to manually clear the stored object before destroying the helper!
             */
            ~data_type()
            {
                ASBIND20_ASSERT(ptr == nullptr && "reference not released");
            }

            data_type& operator=(data_type&& other) noexcept
            {
                if(this == &other) [[unlikely]]
                    return *this;

                std::memcpy((void*)this, &other, sizeof(data_type));
                other.ptr = nullptr;

                return *this;
            }
        };

        /**
         * @name Get the address of the data
         *
         * This can be used to implemented a function that return reference of data to script
         */
        /// @{

        static void* data_address(data_type& data, int type_id)
        {
            ASBIND20_ASSERT(!is_void_type(type_id));

            if(is_primitive_type(type_id))
                return data.primitive;
            else if(is_objhandle(type_id))
                return &data.handle;
            else
                return data.ptr;
        }

        static const void* data_address(const data_type& data, int type_id)
        {
            ASBIND20_ASSERT(!is_void_type(type_id));

            if(is_primitive_type(type_id))
                return data.primitive;
            else if(is_objhandle(type_id))
                return &data.handle;
            else
                return data.ptr;
        }

        /// @}

        /**
         * @brief Get the referenced object
         *
         * This allows direct interaction with the stored object, whether it's an object handle or not
         *
         * @note Only valid if the type of stored data is @b NOT a primitive value
         */
        [[nodiscard]]
        static void* object_ref(const data_type& data) noexcept
        {
            return data.ptr;
        }

        // TODO: API receiving asITypeInfo*

        /**
         * @brief Construct the stored value using its default constructor
         *
         * @param data Stored value
         * @param engine Script engine
         * @param type_id Type ID. Must @b NOT be void (`asTYPEID_VOID`)
         *
         * @return True if successful
         */
        static bool construct(data_type& data, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id)
        {
            ASBIND20_ASSERT(!is_void_type(type_id));

            if(is_primitive_type(type_id))
            {
                std::memset(data.primitive, 0, 8);
            }
            else if(is_objhandle(type_id))
            {
                data.handle = nullptr;
            }
            else
            {
                void* ptr = engine->CreateScriptObject(
                    engine->GetTypeInfoById(type_id)
                );
                if(!ptr) [[unlikely]]
                    return false;
                data.ptr = ptr;
            }

            return true;
        }

        /**
         * @brief Copy construct the stored value from another value
         *
         * @param data Stored value
         * @param engine Script engine
         * @param type_id Type ID. Must @b NOT be void (`asTYPEID_VOID`)
         * @param ref Address of the value. Must @b NOT be `nullptr`
         *
         * @return True if successful
         *
         * @note Make sure this helper doesn't contain a constructed object previously!
         */
        static bool copy_construct(data_type& data, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id, const void* ref)
        {
            ASBIND20_ASSERT(!is_void_type(type_id));

            if(is_primitive_type(type_id))
            {
                copy_primitive_value(data.primitive, ref, type_id);
            }
            else if(is_objhandle(type_id))
            {
                void* handle = *static_cast<void* const*>(ref);
                data.handle = handle;
                if(handle)
                {
                    engine->AddRefScriptObject(
                        handle,
                        engine->GetTypeInfoById(type_id)
                    );
                }
            }
            else
            {
                void* ptr = engine->CreateScriptObjectCopy(
                    const_cast<void*>(ref),
                    engine->GetTypeInfoById(type_id)
                );
                if(!ptr) [[unlikely]]
                    return false;
                data.ptr = ptr;
            }

            return true;
        }

        /**
         * @brief Copy assign the stored value from another value
         *
         * @param data Stored value
         * @param engine Script engine
         * @param type_id Type ID. Must @b NOT be void (`asTYPEID_VOID`)
         * @param ref Address of the value. Must @b NOT be `nullptr`
         *
         * @return True if successful
         *
         * @note Make sure the stored value is valid!
         */
        static bool copy_assign_from(data_type& data, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id, const void* ref)
        {
            ASBIND20_ASSERT(!is_void_type(type_id));

            if(is_primitive_type(type_id))
            {
                copy_primitive_value(data.primitive, ref, type_id);
            }
            else if(is_objhandle(type_id))
            {
                auto* ti = engine->GetTypeInfoById(type_id);
                if(data.handle)
                    engine->ReleaseScriptObject(data.handle, ti);
                void* handle = *static_cast<void* const*>(ref);
                data.handle = handle;
                if(handle)
                {
                    engine->AddRefScriptObject(
                        handle, ti
                    );
                }
            }
            else
            {
                int r = engine->AssignScriptObject(
                    data.ptr,
                    const_cast<void*>(ref),
                    engine->GetTypeInfoById(type_id)
                );
                return r >= 0;
            }

            return true;
        }

        /**
         * @brief Copy assign the stored value to destination
         *
         * @param data Stored value
         * @param engine Script engine
         * @param type_id Type ID. Must @b NOT be void (`asTYPEID_VOID`)
         * @param out Address of the destination. Must @b NOT be `nullptr`
         *
         * @return True if successful
         *
         * @note Make sure the stored value is valid!
         */
        static bool copy_assign_to(const data_type& data, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id, void* out)
        {
            ASBIND20_ASSERT(!is_void_type(type_id));
            ASBIND20_ASSERT(out != nullptr);

            if(is_primitive_type(type_id))
            {
                copy_primitive_value(out, data.primitive, type_id);
            }
            else if(is_objhandle(type_id))
            {
                void** out_handle = static_cast<void**>(out);

                auto* ti = engine->GetTypeInfoById(type_id);
                if(*out_handle)
                    engine->ReleaseScriptObject(*out_handle, ti);
                *out_handle = data.handle;
                if(data.handle)
                {
                    engine->AddRefScriptObject(
                        data.handle, ti
                    );
                }
            }
            else
            {
                int r = engine->AssignScriptObject(
                    out,
                    data.ptr,
                    engine->GetTypeInfoById(type_id)
                );

                return r >= 0;
            }

            return true;
        }

        /**
         * @brief Destroy the stored object
         *
         * @param data Stored value
         * @param engine Script engine
         * @param type_id Type ID. Must @b NOT be void (`asTYPEID_VOID`)
         */
        static void destroy(data_type& data, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id)
        {
            if(is_primitive_type(type_id))
            {
                // Suppressing assertion in destructor
                ASBIND20_ASSERT((data.ptr = nullptr, true));
                return;
            }

            if(!data.ptr)
                return;
            engine->ReleaseScriptObject(
                data.ptr,
                engine->GetTypeInfoById(type_id)
            );
            data.ptr = nullptr;
        }

        /**
         * @brief Enumerate references of stored object for GC
         *
         * @details This function has no effect for non-garbage collected types
         *
         * @param data Stored value
         * @param ti Type information
         */
        static void enum_refs(data_type& data, AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            if(!ti) [[unlikely]]
                return;

            auto flags = ti->GetFlags();
            if(!(flags & AS_NAMESPACE_QUALIFIER asOBJ_GC)) [[unlikely]]
                return;

            if(flags & AS_NAMESPACE_QUALIFIER asOBJ_REF)
            {
                ti->GetEngine()->GCEnumCallback(object_ref(data));
            }
            else if(flags & AS_NAMESPACE_QUALIFIER asOBJ_VALUE)
            {
                ti->GetEngine()->ForwardGCEnumReferences(
                    object_ref(data), ti
                );
            }
        }
    };
} // namespace container
} // namespace asbind20

#endif
