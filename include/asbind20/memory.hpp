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
#include <utility>
#include <type_traits>
#include <string>
#include "detail/include_as.hpp"
#include "utility.hpp"

namespace asbind20
{
/**
 * @brief Wrap `asAllocMem()` and `asFreeMem()` as a C++ allocator
 */
template <typename T>
class as_allocator
{
public:
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    as_allocator() noexcept = default;

    template <typename U>
    as_allocator(const as_allocator<U>&) noexcept
    {}

    as_allocator& operator=(const as_allocator&) = default;

    ~as_allocator() = default;

    static pointer allocate(size_type n)
    {
        size_type size = n * sizeof(value_type);
        return (pointer)(AS_NAMESPACE_QUALIFIER asAllocMem(size));
    }

    static void deallocate(pointer mem, size_type n) noexcept
    {
        (void)n; // unused
        AS_NAMESPACE_QUALIFIER asFreeMem(static_cast<void*>(mem));
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

    explicit reuse_active_context(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, bool propagate_error = true
    )
        : m_engine(engine), m_propagate_error(propagate_error)
    {
        assert(m_engine != nullptr);

        m_ctx = current_context();
        if(m_ctx)
        {
            if(m_ctx->GetEngine() == engine && m_ctx->PushState() >= 0)
                m_is_nested = true;
            else
                m_ctx = nullptr;
        }

        if(!m_ctx)
        {
            m_ctx = engine->RequestContext();
        }
    }

    ~reuse_active_context()
    {
        if(m_ctx)
        {
            if(m_is_nested)
            {
                if(m_propagate_error)
                {
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
                else
                    m_ctx->PopState();
            }
            else
                m_engine->ReturnContext(m_ctx);
        }
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

    explicit request_context(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
        : m_engine(engine)
    {
        assert(m_engine != nullptr);
        m_ctx = m_engine->RequestContext();
    }

    ~request_context()
    {
        if(m_ctx)
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
    void lock()
    {
        assert(*this);
        m_bool->Lock();
    }

    /**
     * @brief Unlock the flag
     */
    void unlock() noexcept
    {
        assert(*this);
        m_bool->Unlock();
    }

    [[nodiscard]]
    bool get_flag() const
    {
        assert(*this);
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
                assert(ptr == nullptr && "reference not released");
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
            assert(!is_void_type(type_id));

            if(is_primitive_type(type_id))
                return data.primitive;
            else if(is_objhandle(type_id))
                return &data.handle;
            else
                return data.ptr;
        }

        static const void* data_address(const data_type& data, int type_id)
        {
            assert(!is_void_type(type_id));

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
         * @param engine Script engine
         * @param type_id Type ID. Must @b NOT be void (`asTYPEID_VOID`)
         *
         * @return True if successful
         */
        static bool construct(data_type& data, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id)
        {
            assert(!is_void_type(type_id));

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
            assert(!is_void_type(type_id));

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
            assert(!is_void_type(type_id));

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
            assert(!is_void_type(type_id));
            assert(out != nullptr);

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
         * @param engine Script engine
         * @param type_id Type ID. Must @b NOT be void (`asTYPEID_VOID`)
         */
        static void destroy(data_type& data, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id)
        {
            if(is_primitive_type(type_id))
            {
                // Suppressing assertion in destructor
                assert((data.ptr = nullptr, true));
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
    name##_type& name()& noexcept                             \
    {                                                         \
        return m_##name;                                      \
    }                                                         \
    const name##_type& name() const& noexcept                 \
    {                                                         \
        return m_##name;                                      \
    }                                                         \
    name##_type&& name()&& noexcept                           \
    {                                                         \
        return std::move(m_##name);                           \
    }                                                         \
    const name##_type&& name() const&& noexcept               \
    {                                                         \
        return std::move(m_##name);                           \
    }

#define ASBIND20_DETAIL_COMPRESSED_PAIR_MEMBER_COMPRESSED(name) \
    name##_type& name()& noexcept                               \
    {                                                           \
        return *this;                                           \
    }                                                           \
    const name##_type& name() const& noexcept                   \
    {                                                           \
        return *this;                                           \
    }                                                           \
    name##_type&& name()&& noexcept                             \
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
            : T2(std::get<Indices1>(tuple1)...), m_first(std::get<Indices2>(tuple2)...)
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
} // namespace asbind20

template <typename T1, typename T2>
struct std::tuple_element<0, asbind20::compressed_pair<T1, T2>>
{
    using type = T1;
};

template <typename T1, typename T2>
struct std::tuple_element<1, asbind20::compressed_pair<T1, T2>>
{
    using type = T2;
};

template <typename T1, typename T2>
struct std::tuple_size<asbind20::compressed_pair<T1, T2>> :
    integral_constant<size_t, 2>
{};

#endif
