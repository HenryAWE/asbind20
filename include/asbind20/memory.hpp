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
#include <ostream>
#include <utility>
#include <type_traits>
#include <string>
#include "detail/err_handler.hpp"
#include "detail/include_as.hpp"
#include "utility.hpp"

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
    static constexpr auto allocate(size_type n) -> pointer;

    static constexpr void deallocate(pointer mem, size_type n) noexcept;

private:
    static void check_length(size_type n);
};

template <typename Pointer>
concept shared_script_object_pointer =
    std::is_pointer_v<Pointer> &&
    requires(Pointer p) {
        p->AddRef();
        p->Release();
    };

/**
 * @brief Tag type of `adopt_object`
 *
 * @sa adopt_object
 */
struct adopt_object_t
{};

/**
 * @brief Tag for taking over a reference which is already owned
 *
 * The constructors and `reset()` of the RAII helpers increase the reference
 * count of the passed object by default. Passing this tag instead tells the
 * helper that the caller already owns a reference, so that the helper takes it
 * over without increasing it. It is meant for the entities which come with
 * ownership, such as the one returned by a function documented as returning a
 * new reference.
 *
 * @sa shared_script_object_interface
 */
inline constexpr adopt_object_t adopt_object{};

/**
 * @brief Base class of the RAII helpers for reference counted script entities
 *
 * It implements the reference counting operations shared by the RAII helpers
 * derived from it: copying and moving (which share and transfer the ownership),
 * `reset()`, `release()`, comparison with another helper, with a pointer and
 * with a reference of the underlying entity, hashing and output to a stream.
 *
 * @tparam SharedObjectPointer Pointer to a reference counted script entity,
 *         such as `asIScriptObject*`
 */
template <shared_script_object_pointer SharedObjectPointer>
class shared_script_object_interface
{
public:
    using pointer = SharedObjectPointer;
    // for compatibility with asbind20 v1.x
    using handle_type = pointer;
    using element_type = std::remove_pointer_t<SharedObjectPointer>;
    using reference = element_type&;
    using const_reference = const element_type&;

    constexpr shared_script_object_interface() noexcept = default;

    explicit shared_script_object_interface(pointer obj)
        : m_obj(obj)
    {
        increase_ref_count();
    }

    explicit shared_script_object_interface(reference obj)
        : shared_script_object_interface(std::addressof(obj))
    {}

    shared_script_object_interface(adopt_object_t, pointer obj) noexcept
        : m_obj(obj)
    {}

    shared_script_object_interface(adopt_object_t, reference obj) noexcept
        : m_obj(std::addressof(obj))
    {}

    shared_script_object_interface(adopt_object_t, shared_script_object_interface&) = delete;
    shared_script_object_interface(adopt_object_t, shared_script_object_interface&&) = delete;

protected:
    ~shared_script_object_interface()
    {
        decrease_ref_count();
    }

public:
    constexpr bool operator==(
        const shared_script_object_interface& rhs
    ) const noexcept
    {
        return m_obj == rhs.m_obj;
    }

    friend constexpr bool operator==(
        pointer lhs, const shared_script_object_interface& rhs
    ) noexcept
    {
        return lhs == rhs.get();
    }

    friend constexpr bool operator==(
        const shared_script_object_interface& lhs, pointer rhs
    ) noexcept
    {
        return lhs.get() == rhs;
    }

    friend constexpr bool operator==(
        const_reference lhs, const shared_script_object_interface& rhs
    ) noexcept
    {
        return std::addressof(lhs) == rhs.get();
    }

    friend constexpr bool operator==(
        const shared_script_object_interface& lhs, const_reference rhs
    ) noexcept
    {
        return lhs.get() == std::addressof(rhs);
    }

    [[nodiscard]]
    pointer get() const noexcept
    {
        return m_obj;
    }

    explicit operator pointer() const noexcept
    {
        return get();
    }

    explicit operator bool() const noexcept
    {
        return get() != nullptr;
    }

    reference operator*() const noexcept
    {
        return *get();
    }

    pointer operator->() const noexcept
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
     * @brief Reset object to the null pointer
     */
    void reset(std::nullptr_t = nullptr) noexcept
    {
        decrease_ref_count();
        m_obj = nullptr;
    }

    /**
     * @brief Reset object
     *
     * @param obj New object to store
     */
    void reset(pointer obj)
    {
        // Avoid Release-then-AddRef on the same handle,
        if(m_obj == obj) [[unlikely]]
            return;

        decrease_ref_count();
        m_obj = obj;
        increase_ref_count();
    }

    void reset(reference obj)
    {
        reset(std::addressof(obj));
    }

    /**
     * @brief Reset object by taking over the ownership of a reference
     *
     * @note If the stored object is the same as `obj`, this function does
     *       nothing, i.e., the caller keeps the ownership of the passed
     *       reference. Use `reset(std::move(other))` to transfer the ownership
     *       between helpers.
     */
    void reset(adopt_object_t, pointer obj)
    {
        // Avoid Release-then-AddRef on the same handle,
        if(m_obj == obj) [[unlikely]]
            return;

        decrease_ref_count();
        m_obj = obj;
        // Don't increase the reference count here
    }

    void reset(adopt_object_t, reference obj)
    {
        this->reset(adopt_object, std::addressof(obj));
    }

    void reset(shared_script_object_interface& obj)
    {
        if(this == &obj) [[unlikely]]
            return;
        this->reset(obj.get());
    }

    void reset(shared_script_object_interface&& obj) noexcept
    {
        if(this == &obj) [[unlikely]]
            return;
        take_over(obj.release());
    }

    void reset(adopt_object_t, shared_script_object_interface& obj) = delete;
    void reset(adopt_object_t, shared_script_object_interface&& obj) = delete;

    // For consistency with standard smart pointers,
    // outputs the underlying pointer
    friend std::ostream& operator<<(std::ostream& os, const shared_script_object_interface& obj)
    {
        os << obj.get();
        return os;
    }

protected:
    void increase_ref_count() const
    {
        if(!m_obj)
            return;
        (void)m_obj->AddRef();
    }

    void decrease_ref_count() const
    {
        if(!m_obj)
            return;
        (void)m_obj->Release();
    }

    void take_over(pointer obj) noexcept
    {
        decrease_ref_count();
        m_obj = obj;
    }

    shared_script_object_interface(const shared_script_object_interface& other)
        : m_obj(other.m_obj)
    {
        increase_ref_count();
    }

    shared_script_object_interface(shared_script_object_interface&& other) noexcept
        : m_obj(other.release())
    {}

    shared_script_object_interface& operator=(
        shared_script_object_interface&& other
    ) noexcept
    {
        if(this == &other) [[unlikely]]
            return *this;
        take_over(other.release());
        return *this;
    }

    shared_script_object_interface& operator=(
        const shared_script_object_interface& other
    ) noexcept
    {
        if(this == &other) [[unlikely]]
            return *this;
        reset(other.get());
        return *this;
    }

    void swap(shared_script_object_interface& other) noexcept
    {
        using std::swap;
        swap(m_obj, other.m_obj);
    }

private:
    pointer m_obj = nullptr;
};
} // namespace asbind20

template <asbind20::shared_script_object_pointer SharedObjectPointer>
struct std::hash<asbind20::shared_script_object_interface<SharedObjectPointer>>
{
    constexpr std::size_t operator()(
        const asbind20::shared_script_object_interface<SharedObjectPointer>& obj
    ) const
    {
        return std::hash<SharedObjectPointer>{}(obj.get());
    }
};

namespace asbind20
{
/**
 * @brief Smart pointer for script object
 */
class script_object : public shared_script_object_interface<object_pointer>
{
    using my_base = shared_script_object_interface<object_pointer>;

public:
    using my_base::my_base;

    void swap(script_object& other) noexcept
    {
        my_base::swap(other);
    }
};

inline void swap(script_object& lhs, script_object& rhs) noexcept
{
    lhs.swap(rhs);
}

/**
 * @brief RAII helper for reusing active script context.
 *
 * It will fallback to request context from the engine.
 */
class [[nodiscard]] reuse_active_context
{
public:
    using element_type = AS_NAMESPACE_QUALIFIER asIScriptContext;
    using handle_type = context_pointer;

    reuse_active_context() = delete;
    reuse_active_context(const reuse_active_context&) = delete;

    reuse_active_context& operator=(const reuse_active_context&) = delete;

    explicit reuse_active_context(std::nullptr_t) = delete;

    explicit reuse_active_context(
        engine_pointer engine, bool propagate_error = true
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

    template <script_engine_pointer_like Engine>
    explicit reuse_active_context(
        const Engine& engine, bool propagate_error = true
    )
        : reuse_active_context(engine.get(), propagate_error)
    {}

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
    engine_pointer get_engine() const noexcept
    {
        return m_engine;
    }

    operator handle_type() const noexcept
    {
        return get();
    }

    context_reference operator*() const noexcept
    {
        return *get();
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
    engine_pointer m_engine = nullptr;
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
    using element_type = AS_NAMESPACE_QUALIFIER asIScriptContext;
    using handle_type = context_pointer;

    request_context() = delete;
    request_context(const request_context&) = delete;

    request_context& operator=(const request_context&) = delete;

    explicit request_context(std::nullptr_t) = delete;

    explicit request_context(engine_pointer engine)
        : m_engine(engine)
    {
        ASBIND20_ASSERT(m_engine != nullptr);
        m_ctx = m_engine->RequestContext();
    }

    explicit request_context(engine_reference engine)
        : request_context(std::addressof(engine))
    {}

    template <script_engine_pointer_like Engine>
    explicit request_context(const Engine& engine)
        : request_context(engine.get())
    {}

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
    engine_pointer get_engine() const noexcept
    {
        return m_engine;
    }

    operator handle_type() const noexcept
    {
        return get();
    }

    context_reference operator*() const noexcept
    {
        return *get();
    }

    handle_type operator->() const noexcept
    {
        return get();
    }

private:
    engine_pointer m_engine = nullptr;
    handle_type m_ctx = nullptr;
};

/**
 * @brief RAII helper for script context
 *
 * @note Use `adopt_object` for taking over a context which already owns a reference,
 *       such as the one returned by `asIScriptEngine::CreateContext()`.
 */
class script_context : public shared_script_object_interface<context_pointer>
{
    using my_base = shared_script_object_interface<context_pointer>;

public:
    using my_base::my_base;

    script_context() noexcept = default;

    /**
     * @brief Create context from the script engine.
     *
     * @param engine Script engine. If it is null, the context will not be created.
     */
    explicit script_context(
        engine_pointer engine
    )
    {
        if(!engine) [[unlikely]]
            return;
        reset(adopt_object, engine->CreateContext());
    }

    template <script_engine_pointer_like Engine>
    explicit script_context(
        const Engine& engine
    )
        : script_context(engine.get())
    {}

    void swap(script_context& other) noexcept
    {
        my_base::swap(other);
    }
};

inline void swap(script_context& lhs, script_context& rhs) noexcept
{
    lhs.swap(rhs);
}

/**
 * @brief Script engine manager
 */
class script_engine
{
public:
    using element_type = AS_NAMESPACE_QUALIFIER asIScriptEngine;
    using handle_type = engine_pointer;
    using pointer = engine_pointer;

    script_engine() noexcept
        : m_engine(nullptr) {}

    script_engine(const script_engine&) = delete;

    script_engine(script_engine&& other) noexcept
        : m_engine(std::exchange(other.m_engine, nullptr)) {}

    explicit script_engine(handle_type engine) noexcept
        : m_engine(engine) {}

    explicit script_engine(engine_reference engine) noexcept
        : script_engine(std::addressof(engine)) {}

    script_engine& operator=(const script_engine&) = delete;

    script_engine& operator=(script_engine&& other) noexcept
    {
        if(this == &other)
            return *this;
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

    bool operator==(const script_engine& rhs) const noexcept
    {
        return m_engine == rhs.m_engine;
    }

    friend bool operator==(
        const script_engine& lhs, pointer rhs
    ) noexcept
    {
        return lhs.get() == rhs;
    }

    friend bool operator==(
        pointer lhs, const script_engine& rhs
    ) noexcept
    {
        return lhs == rhs.get();
    }

    explicit operator handle_type() const noexcept
    {
        return get();
    }

    explicit operator bool() const noexcept
    {
        return get() != nullptr;
    }

    engine_reference operator*() const noexcept
    {
        return *get();
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
            (void)m_engine->ShutDownAndRelease();
        m_engine = engine;
    }

    void reset(engine_reference engine) noexcept
    {
        reset(std::addressof(engine));
    }

    void swap(script_engine& other) noexcept
    {
        using std::swap;
        swap(m_engine, other.m_engine);
    }

    // For consistency with standard smart pointers,
    // outputs the underlying pointer
    friend std::ostream& operator<<(std::ostream& os, const script_engine& engine)
    {
        os << engine.get();
        return os;
    }

private:
    handle_type m_engine;
};

inline void swap(script_engine& lhs, script_engine& rhs) noexcept
{
    lhs.swap(rhs);
}

using script_version_type = AS_NAMESPACE_QUALIFIER asDWORD;

inline constexpr script_version_type default_script_version = ANGELSCRIPT_VERSION;

/**
 * @brief Create an AngelScript engine
 *
 * @note The returned engine is a raw pointer which owns a reference.
 *       Use `make_script_engine()` for owning it exclusively, or
 *       `make_shared_script_engine()` for sharing the ownership.
 */
[[nodiscard]]
inline engine_pointer create_script_engine(
    script_version_type version = default_script_version
)
{
    return AS_NAMESPACE_QUALIFIER asCreateScriptEngine(version);
}

/**
 * @brief Create an AngelScript engine
 */
[[nodiscard]]
inline script_engine make_script_engine(
    script_version_type version = default_script_version
)
{
    return script_engine(
        create_script_engine(version)
    );
}

/**
 * @brief RAII helper for shared script engine
 *
 * @note Use `adopt_object` for taking over an engine which already owns a reference,
 *       such as the one returned by `create_script_engine()`.
 */
class shared_script_engine : public shared_script_object_interface<engine_pointer>
{
    using my_base = shared_script_object_interface<engine_pointer>;

public:
    using my_base::my_base;

    shared_script_engine() noexcept = default;

    shared_script_engine(const unique_script_engine&) = delete;

    shared_script_engine(unique_script_engine&& other) noexcept
        : my_base(adopt_object, other.release()) {}

    shared_script_engine& operator=(unique_script_engine&& other) noexcept
    {
        reset(std::move(other));
        return *this;
    }

    using my_base::reset;

    void reset(const unique_script_engine&) = delete;

    void reset(unique_script_engine&& other)
    {
        // The engine may be the same one, so the reference owned by this
        // helper must be dropped before taking over the one of `other`.
        take_over(other.release());
    }

    void swap(shared_script_engine& other) noexcept
    {
        my_base::swap(other);
    }
};

inline void swap(shared_script_engine& lhs, shared_script_engine& rhs) noexcept
{
    lhs.swap(rhs);
}

/**
 * @brief Create a shared AngelScript engine
 */
[[nodiscard]]
inline shared_script_engine make_shared_script_engine(
    script_version_type version = default_script_version
)
{
    return {adopt_object, create_script_engine(version)};
}

/**
 * @brief Helper for `asILockableSharedBool*`
 *
 * This class can be helpful for implementing weak reference support.
 */
class lockable_shared_bool :
    public shared_script_object_interface<AS_NAMESPACE_QUALIFIER asILockableSharedBool*>
{
    using my_base = shared_script_object_interface<AS_NAMESPACE_QUALIFIER asILockableSharedBool*>;

public:
    using my_base::my_base;

    /**
      * @brief Connect to the weak reference flag of object
      *
      * @param obj Object to connect
      * @param ti Type information
      *
      * @note If it failed to connect, this helper will be reset to nullptr.
      */
    void connect_object(void* obj, const_typeinfo_reference ti)
    {
        reset(
            ti.GetEngine()->GetWeakRefFlagOfScriptObject(
                obj, std::addressof(ti)
            )
        );
    }

    void connect_object(void* obj, const_typeinfo_pointer ti)
    {
        if(!ti) [[unlikely]]
        {
            reset();
            return;
        }

        connect_object(obj, *ti);
    }

    lockable_shared_bool(const lockable_shared_bool& other) = default;
    lockable_shared_bool(lockable_shared_bool&& other) noexcept = default;

    lockable_shared_bool& operator=(const lockable_shared_bool& other) = default;

    lockable_shared_bool& operator=(lockable_shared_bool&& other) noexcept = default;

    /**
     * @brief Lock the flag
     */
    void lock() const
    {
        ASBIND20_ASSERT(*this);
        get()->Lock();
    }

    /**
     * @brief Unlock the flag
     */
    void unlock() const noexcept
    {
        ASBIND20_ASSERT(*this);
        get()->Unlock();
    }

    [[nodiscard]]
    bool get_flag() const
    {
        ASBIND20_ASSERT(*this);
        return get()->Get();
    }

    void set_flag(bool value = true) const
    {
        get()->Set(value);
    }

    void swap(lockable_shared_bool& other) noexcept
    {
        my_base::swap(other);
    }
};

inline void swap(lockable_shared_bool& lhs, lockable_shared_bool& rhs) noexcept
{
    lhs.swap(rhs);
}

/**
 * @brief Create a lockable shared bool for implementing weak reference
 *
 * @note Lock the exclusive lock in multithreading environment
 */
[[nodiscard]]
inline lockable_shared_bool make_lockable_shared_bool()
{
    return {adopt_object, AS_NAMESPACE_QUALIFIER asCreateLockableSharedBool()};
}

/**
 * @brief RAII helper for `asITypeInfo*`.
 *
 * @note Assigning a type info object will increase its reference count.
 *       Use `adopt_object` for taking over a reference you already own,
 *       because the AngelScript APIs for getting type info generally don't
 *       increase the reference count, such as being the hidden first argument
 *       of template class constructor/factory.
 */
class script_typeinfo : public shared_script_object_interface<typeinfo_pointer>
{
    using my_base = shared_script_object_interface<typeinfo_pointer>;

public:
    using my_base::my_base;

    /**
     * @brief Get the type ID of the stored type info
     *
     * @return Type ID, or `asINVALID_ARG` if no type info is stored
     */
    [[nodiscard]]
    int type_id() const
    {
        if(!get()) [[unlikely]]
            return AS_NAMESPACE_QUALIFIER asINVALID_ARG;

        return get()->GetTypeId();
    }

    /**
     * @brief Get the type ID of the subtype
     *
     * @param idx Index of the subtype
     * @return Type ID of the subtype, or `asINVALID_ARG` if no type info is stored
     */
    [[nodiscard]]
    int subtype_id(subtype_index_type idx = 0) const
    {
        if(!get()) [[unlikely]]
            return AS_NAMESPACE_QUALIFIER asINVALID_ARG;

        return get()->GetSubTypeId(idx);
    }

    /**
     * @brief Get the type info of the subtype
     *
     * @warning It @b won't increase the reference count of the returned type info!
     *
     * @param idx Index of the subtype
     * @return Type info of the subtype, or `nullptr` if no type info is stored
     */
    [[nodiscard]]
    typeinfo_pointer subtype(subtype_index_type idx = 0) const
    {
        if(!get()) [[unlikely]]
            return nullptr;

        return get()->GetSubType(idx);
    }

    void swap(script_typeinfo& other) noexcept
    {
        my_base::swap(other);
    }
};

inline void swap(script_typeinfo& lhs, script_typeinfo& rhs) noexcept
{
    lhs.swap(rhs);
}
} // namespace asbind20

template <>
struct std::hash<asbind20::script_object> : std::hash<asbind20::shared_script_object_interface<asbind20::object_pointer>>
{};

template <>
struct std::hash<asbind20::script_context> : std::hash<asbind20::shared_script_object_interface<asbind20::context_pointer>>
{};

template <>
struct std::hash<asbind20::shared_script_engine> : std::hash<asbind20::shared_script_object_interface<asbind20::engine_pointer>>
{};

template <>
struct std::hash<asbind20::lockable_shared_bool> : std::hash<asbind20::shared_script_object_interface<AS_NAMESPACE_QUALIFIER asILockableSharedBool*>>
{};

template <>
struct std::hash<asbind20::script_typeinfo> : std::hash<asbind20::shared_script_object_interface<asbind20::typeinfo_pointer>>
{};

namespace asbind20
{
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
                // Cast to void* to avoid clangd warning
                std::memcpy(
                    static_cast<void*>(this), static_cast<void*>(&other), sizeof(data_type)
                );
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

                // Cast to void* to avoid clangd warning
                std::memcpy(
                    static_cast<void*>(this), static_cast<void*>(&other), sizeof(data_type)
                );
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
        static bool construct(data_type& data, engine_pointer engine, int type_id)
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
        static bool copy_construct(data_type& data, engine_pointer engine, int type_id, const void* ref)
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
        static bool copy_assign_from(data_type& data, engine_pointer engine, int type_id, const void* ref)
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
        static bool copy_assign_to(const data_type& data, engine_pointer engine, int type_id, void* out)
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
        static void destroy(data_type& data, engine_pointer engine, int type_id)
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
        static void enum_refs(data_type& data, typeinfo_pointer ti)
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

#include "memory.inl"

#endif
