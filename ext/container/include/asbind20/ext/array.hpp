#ifndef ASBIND20_EXT_CONTAINER_ARRAY_HPP
#define ASBIND20_EXT_CONTAINER_ARRAY_HPP

#pragma once

#include <cstddef>
#include <asbind20/asbind.hpp>
#include <asbind20/container/small_vector.hpp>

namespace asbind20::ext
{
consteval auto default_script_array_user_id() noexcept
    -> AS_NAMESPACE_QUALIFIER asPWORD
{
    return 2000;
}

namespace detail
{
    class script_array_base
    {
    public:
        void* operator new(std::size_t bytes);
        void operator delete(void* p);

    protected:
        struct array_cache
        {
            AS_NAMESPACE_QUALIFIER asIScriptFunction* subtype_opCmp;
            AS_NAMESPACE_QUALIFIER asIScriptFunction* subtype_opEquals;
            int opCmp_status;
            int opEquals_status;
        };

        static bool elem_opEquals(
            int subtype_id,
            const void* lhs,
            const void* rhs,
            AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
            const array_cache* cache
        );

    public:
        static bool template_callback(
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti, bool&
        );

    protected:
        static void generate_cache(
            array_cache& out, int subtype_id, AS_NAMESPACE_QUALIFIER asITypeInfo* ti
        );

        // Setups cached methods for script array.
        // This function must be called whenever a new script array has been constructed!
        // According to the author of AngelScript, the template callback is not meant for caching data.
        // See: https://www.gamedev.net/forums/topic/717709-about-caching-required-methods-in-template-callback/
        template <AS_NAMESPACE_QUALIFIER asPWORD UserDataID>
        static void setup_cache(
            int subtype_id, AS_NAMESPACE_QUALIFIER asITypeInfo* ti
        )
        {
            if(is_primitive_type(subtype_id))
                return;

            array_cache* cache = static_cast<array_cache*>(
                ti->GetUserData(UserDataID)
            );
            if(cache)
                return;

            std::lock_guard guard(as_exclusive_lock);

            // Double-check to prevent cache from being created by another thread
            cache = static_cast<array_cache*>(ti->GetUserData(UserDataID));
            if(cache) [[unlikely]]
                return;

            cache = reinterpret_cast<array_cache*>(
                AS_NAMESPACE_QUALIFIER asAllocMem(sizeof(array_cache))
            );
            if(!cache) [[unlikely]]
            {
                set_script_exception("out of memory");
                return;
            }

            std::memset(cache, 0, sizeof(array_cache));
            generate_cache(*cache, subtype_id, ti);

            ti->SetUserData(cache, UserDataID);
        }

        template <AS_NAMESPACE_QUALIFIER asPWORD UserDataID>
        static void cache_cleanup_callback(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            void* mem = ti->GetUserData(UserDataID);
            if(mem)
                AS_NAMESPACE_QUALIFIER asFreeMem(mem);
        }

        template <AS_NAMESPACE_QUALIFIER asPWORD UserDataID>
        static array_cache* get_cache(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            assert(ti != nullptr);
            return static_cast<array_cache*>(
                ti->GetUserData(UserDataID)
            );
        }
    };
} // namespace detail

class script_array : private detail::script_array_base
{
    friend void register_script_array(AS_NAMESPACE_QUALIFIER asIScriptEngine*, bool, bool);

    using my_base = detail::script_array_base;

    static constexpr AS_NAMESPACE_QUALIFIER asPWORD user_id = default_script_array_user_id();

    void setup_cache()
    {
        AS_NAMESPACE_QUALIFIER asITypeInfo* ti = get_type_info();
        my_base::setup_cache<user_id>(ti->GetSubTypeId(), ti);
    }

    array_cache* get_cache() const
    {
        return my_base::get_cache<user_id>(get_type_info());
    }

public:
    using size_type = AS_NAMESPACE_QUALIFIER asUINT;
    using index_type = std::make_signed_t<size_type>;

    using my_base::operator new;
    using my_base::operator delete;

    script_array() = delete;

    script_array(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        : m_data(ti)
    {
        setup_cache();
    }

    script_array(const script_array& other)
        : m_data(other.m_data)
    {
        setup_cache();
    }

    script_array(asITypeInfo* ti, size_type n);

    script_array(asITypeInfo* ti, size_type n, const void* value);

    script_array(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, script_init_list_repeat ilist)
        : m_data(ti, ilist)
    {
        setup_cache();
    }

private:
    ~script_array() = default;

public:
    auto get_engine() const
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return get_type_info()->GetEngine();
    }

    script_array& operator=(const script_array& other);

    bool operator==(const script_array& other) const
    {
        if(get_type_info() != other.get_type_info()) [[unlikely]]
        {
            assert(false && "comparing different arrays with different element types");
            return false;
        }

        if(size() != other.size())
            return false;
        if(empty())
            return true;

        int subtype_id = m_data.element_type_id();
        if(!is_primitive_type(m_data.element_type_id()))
        {
            array_cache* cache = get_cache();

            reuse_active_context ctx(get_engine());
            for(size_type i = 0; i < size(); ++i)
            {
                bool eq = elem_opEquals(
                    subtype_id,
                    (*this)[i],
                    other[i],
                    ctx,
                    cache
                );
                if(!eq)
                    return false;
            }
        }
        else // Primitive types
        {
            assert(is_primitive_type(subtype_id));
            for(size_type i = 0; i < size(); ++i)
            {
                bool eq = elem_opEquals(
                    subtype_id,
                    (*this)[i],
                    other[i],
                    nullptr,
                    nullptr
                );
                if(!eq)
                    return false;
            }
        }

        return true;
    }

    void* operator[](size_type idx)
    {
        return m_data[idx];
    }

    const void* operator[](size_type idx) const
    {
        return m_data[idx];
    }

    void* data() noexcept
    {
        return m_data.data();
    }

    const void* data() const noexcept
    {
        return m_data.data();
    }

    void addref()
    {
        m_gc_flag = false;
        ++m_counter;
    }

    void release() noexcept
    {
        m_gc_flag = false;
        m_counter.dec_and_try_destroy(
            [](auto* p)
            { delete p; },
            this
        );
    }

    size_type size() const noexcept
    {
        return static_cast<size_type>(m_data.size());
    }

    size_type capacity() const noexcept
    {
        return static_cast<size_type>(m_data.capacity());
    }

    bool empty() const noexcept
    {
        return m_data.empty();
    }

    void reserve(size_type new_cap)
    {
        m_data.reserve(new_cap);
    }

    void shrink_to_fit()
    {
        // TODO: shrinking
    }

#define ASBIND20_EXT_ARRAY_CHECK_CALLBACK(func_name, ret)        \
    do {                                                         \
        if(this->m_within_callback)                              \
        {                                                        \
            set_script_exception(                                \
                #func_name "(): modifying array within callback" \
            );                                                   \
            return ret;                                          \
        }                                                        \
    } while(0)

    void clear() noexcept
    {
        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(clear, void());

        m_data.clear();
    }

    void push_back(const void* value)
    {
        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(push_back, void());

        m_data.push_back(value);
    }

    void emplace_back()
    {
        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(emplace_back, void());

        m_data.emplace_back();
    }

    void pop_back()
    {
        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(pop_back, void());

        m_data.pop_back();
    }

    void append_range(const script_array& rng, size_type n = -1);

    void insert(size_type idx, void* value)
    {
        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(insert, void());

        m_data.insert(idx, value);
    }

    void insert_range(size_type idx, const script_array& rng, size_type n = -1);

    void erase(size_type idx, size_type n = 1)
    {
        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(erase, void());

        m_data.erase(idx, n);
    }

    size_type count(const void* val, size_type start, size_type n = -1) const
    {
        if(start >= size())
            return 0;

        n = std::min(size() - start, n);

        int subtype_id = m_data.element_type_id();
        if(is_primitive_type(subtype_id))
        {
            return visit_primitive_type(
                []<typename T>(const T* start, const T* stop, const T* val)
                {
                    return static_cast<size_type>(std::count(
                        start,
                        stop,
                        *val
                    ));
                },
                subtype_id,
                m_data.data_at(start),
                m_data.data_at(start + n),
                val
            );
        }
        else
        {
            array_cache* cache = get_cache();
            reuse_active_context ctx(get_engine(), true);

            size_type result = 0;
            void** elems = (void**)m_data.data();
            for(size_type i = 0; i < size(); ++i)
            {
                const void* elem = elems[i];
                if(elem_opEquals(subtype_id, elem, val, ctx, cache))
                    ++result;
            }

            return result;
        }
    }

    size_type erase_value(const void* val, size_type idx = 0, size_type n = -1);

    void sort(size_type idx = 0, size_type n = -1, bool asc = true);

    void reverse(size_type idx = 0, size_type n = -1);

    size_type find(const void* value, size_type idx = 0) const;

    bool contains(const void* value, size_type idx = 0) const;

    [[nodiscard]]
    auto get_type_info() const noexcept
        -> AS_NAMESPACE_QUALIFIER asITypeInfo*
    {
        return m_data.get_type_info();
    }

private:
    using container_type = container::small_vector<
        container::typeinfo_subtype<0>,
        4 * sizeof(void*),
        as_allocator<void>>;

    container_type m_data;

    void set_gc_flag()
    {
        m_gc_flag = true;
    }

    bool get_gc_flag() const
    {
        return m_gc_flag;
    }

public:
    int get_refcount() const
    {
        return m_counter;
    }

    void enum_refs(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
    {
        (void)engine;
        assert(m_data.get_type_info()->GetEngine() == engine);
        m_data.enum_refs();
    }

    void release_refs(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
    {
        (void)engine;
        assert(m_data.get_type_info()->GetEngine() == engine);
        clear();
    }

    void* get_front()
    {
        if(empty())
        {
            set_script_exception("get_front(): empty array");
            return nullptr;
        }

        return m_data[0];
    }

    void* get_back()
    {
        if(empty())
        {
            set_script_exception("get_back(): empty array");
            return nullptr;
        }

        return m_data[m_data.size() - 1];
    }

    void set_front(void* value)
    {
        if(empty())
        {
            ASBIND20_EXT_ARRAY_CHECK_CALLBACK(set_front, void());
            m_data.insert(m_data.begin(), value);
        }
        else
        {
            m_data.assign(0, value);
        }
    }

    void set_back(void* value)
    {
        if(empty())
        {
            ASBIND20_EXT_ARRAY_CHECK_CALLBACK(set_back, void());
            m_data.push_back(value);
        }
        else
        {
            m_data.assign(m_data.size() - 1, value);
        }
    }

private:
    void* opIndex(size_type idx)
    {
        if(idx >= size())
        {
            set_script_exception("out or range");
            return nullptr;
        }
        return (*this)[idx];
    }

    atomic_counter m_counter;
    bool m_gc_flag = false;
    mutable bool m_within_callback = false;

    class callback_guard
    {
    public:
        callback_guard(const script_array* this_) noexcept
            : m_guard(this_->m_within_callback)
        {
            assert(!m_guard);
            m_guard = true;
        }

        ~callback_guard()
        {
            assert(m_guard);
            m_guard = false;
        }

    private:
        bool& m_guard;
    };

#undef ASBIND20_EXT_ARRAY_CHECK_CALLBACK
};

inline void register_script_array(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool as_default = true,
    bool generic = has_max_portability()
)
{
    using array_t = script_array;

#define ASBIND_EXT_ARRAY_MFN(name, ret, args) \
    static_cast<ret(array_t::*) args>(&array_t::name)

    auto helper = [engine, as_default]<bool UseGeneric>(std::bool_constant<UseGeneric>)
    {
        template_ref_class<array_t> c(
            engine,
            "array<T>",
            AS_NAMESPACE_QUALIFIER asOBJ_GC
        );
        c
            .template_callback(fp<&array_t::template_callback>)
            .addref(fp<&array_t::addref>)
            .release(fp<&array_t::release>)
            .get_refcount(fp<&array_t::get_refcount>)
            .get_gc_flag(fp<&array_t::get_gc_flag>)
            .set_gc_flag(fp<&array_t::set_gc_flag>)
            .enum_refs(fp<&array_t::enum_refs>)
            .release_refs(fp<&array_t::release_refs>)
            .default_factory(use_policy<policies::notify_gc>)
            .list_factory("repeat T", use_policy<policies::repeat_list_proxy, policies::notify_gc>)
            .opEquals()
            .method("uint get_size() const property", fp<&array_t::size>)
            .method("bool empty() const", fp<&array_t::empty>)
            .method("T& opIndex(uint)", fp<&array_t::opIndex>)
            .method("const T& opIndex(uint) const", fp<&array_t::opIndex>)
            .method("void push_back(const T&in)", fp<&array_t::push_back>)
            .method("void emplace_back()", fp<&array_t::emplace_back>)
            .method("void pop_back()", fp<&array_t::pop_back>)
            .method("void set_front(const T&in) property", fp<&array_t::set_front>)
            .method("void set_back(const T&in) property", fp<&array_t::set_back>)
            .method("T& get_front() property", fp<&array_t::get_front>)
            .method("T& get_back() property", fp<&array_t::get_back>)
            .method("const T& get_front() const property", fp<&array_t::get_front>)
            .method("const T& get_back() const property", fp<&array_t::get_back>)
            .method("uint count(const T&in, uint start=0,uint n=uint(-1)) const", fp<&array_t::count>);

        if(as_default)
            c.as_array();
    };

    if(generic)
        helper(std::true_type{});
    else
        helper(std::false_type{});

    engine->SetTypeInfoUserDataCleanupCallback(
        &array_t::template cache_cleanup_callback<array_t::user_id>,
        array_t::user_id
    );
}
} // namespace asbind20::ext

#endif
