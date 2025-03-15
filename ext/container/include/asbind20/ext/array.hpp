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

            AS_NAMESPACE_QUALIFIER asITypeInfo* iterator_ti;

            ~array_cache() = default;
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

    private:
        static void find_required_elem_methods(
            array_cache& out, int subtype_id, AS_NAMESPACE_QUALIFIER asITypeInfo* ti
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
            array_cache* cache = static_cast<array_cache*>(
                ti->GetUserData(UserDataID)
            );
            if(cache) [[likely]]
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

            new(cache) array_cache{};
            generate_cache(*cache, subtype_id, ti);

            ti->SetUserData(cache, UserDataID);
        }

        template <AS_NAMESPACE_QUALIFIER asPWORD UserDataID>
        static void cache_cleanup_callback(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            array_cache* mem = get_cache<UserDataID>(ti);
            if(mem)
            {
                mem->~array_cache();
                AS_NAMESPACE_QUALIFIER asFreeMem(mem);
            }
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

    script_array(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, size_type n)
        : m_data(ti)
    {
        setup_cache();
        m_data.emplace_back_n(n);
    }

    script_array(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, size_type n, const void* value)
        : m_data(ti)
    {
        setup_cache();
        m_data.push_back_n(n, value);
    }

    script_array(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, script_init_list_repeat ilist)
        : m_data(ti, ilist)
    {
        setup_cache();
    }

private:
    ~script_array() = default;

public:
    // Proxy class for binding iterator to AngelScript
    class script_array_iterator
    {
        friend script_array;
        friend void register_script_array(AS_NAMESPACE_QUALIFIER asIScriptEngine*, bool, bool);

    private:
        script_array_iterator() = default;

    public:
        using difference_type = std::make_signed_t<size_type>;

        script_array_iterator(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            (void)ti;
        }

        script_array_iterator(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, script_array* arr)
            : m_arr(arr)
        {
            (void)ti;
            assert(ti->GetSubTypeId() == arr->get_type_info()->GetSubTypeId());
            if(arr)
                m_arr->addref();
        }

        script_array_iterator(script_array_iterator&& other)
            : m_arr(std::exchange(other.m_arr, nullptr)), m_offset(other.m_offset)
        {}

        script_array_iterator(const script_array_iterator& other)
            : m_arr(other.m_arr), m_offset(other.m_offset)
        {
            if(m_arr)
                m_arr->addref();
        }

        script_array_iterator(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, const script_array_iterator& other)
            : script_array_iterator(other)
        {
            (void)ti;
            assert(ti->GetSubTypeId() == m_arr->get_type_info()->GetSubTypeId());
        }

        script_array_iterator(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, script_array* arr, size_type offset)
            : m_arr(arr), m_offset(offset)
        {
            (void)ti;
            assert(ti->GetSubTypeId() == m_arr->get_type_info()->GetSubTypeId());
            if(m_arr)
                m_arr->addref();
        }

        ~script_array_iterator()
        {
            if(m_arr)
                m_arr->release();
        }

        bool operator==(const script_array_iterator& rhs) const
        {
            if(m_arr != rhs.m_arr) [[unlikely]]
            {
                raise_incompatible_iterator();
            }

            return m_offset == rhs.m_offset;
        }

        std::weak_ordering operator<=>(const script_array_iterator& rhs) const
        {
            if(m_arr != rhs.m_arr) [[unlikely]]
            {
                raise_incompatible_iterator();
            }

            if(m_arr == nullptr) [[unlikely]]
                return std::weak_ordering::equivalent;
            else
                return m_offset <=> rhs.m_offset;
        }

        script_array_iterator& operator=(const script_array_iterator& rhs)
        {
            if(this == &rhs) [[unlikely]]
                return *this;

            m_offset = rhs.m_offset;
            if(m_arr)
                m_arr->release();
            m_arr = rhs.m_arr;
            if(m_arr)
                m_arr->addref();

            return *this;
        }

        script_array_iterator& operator++() noexcept
        {
            ++m_offset;
            return *this;
        }

        script_array_iterator& operator--() noexcept
        {
            if(m_offset == 0) [[unlikely]]
                return *this;
            --m_offset;
            return *this;
        }

        script_array_iterator operator++(int) noexcept
        {
            script_array_iterator tmp = *this;
            ++m_offset;
            return tmp;
        }

        script_array_iterator operator--(int) noexcept
        {
            script_array_iterator tmp = *this;
            if(m_offset == 0) [[unlikely]]
                return tmp;
            --m_offset;
            return tmp;
        }

        script_array_iterator& operator+=(difference_type diff) noexcept
        {
            if(diff + static_cast<difference_type>(m_offset) < 0) [[unlikely]]
                m_offset = 0;
            else
                m_offset += diff;

            return *this;
        }

        script_array_iterator& operator-=(difference_type diff) noexcept
        {
            *this += -diff;
            return *this;
        }

        difference_type operator-(const script_array_iterator& rhs) const
        {
            if(m_arr != rhs.m_arr) [[unlikely]]
                raise_incompatible_iterator();

            return static_cast<difference_type>(m_offset) -
                   static_cast<difference_type>(rhs.m_offset);
        }

        [[nodiscard]]
        script_array* get_array() const noexcept
        {
            return m_arr;
        }

        [[nodiscard]]
        size_type get_offset() const noexcept
        {
            return m_offset;
        }

        void* value() const
        {
            if(!m_arr) [[unlikely]]
            {
                set_script_exception("array_iterator<T>: empty iterator");
                return nullptr;
            }
            if(m_offset >= m_arr->size())
            {
                raise_invalid_position();
                return nullptr;
            }

            return (*m_arr)[m_offset];
        }

        explicit operator bool() const noexcept
        {
            return m_arr != nullptr;
        }

        void enum_refs(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
        {
            if(!m_arr)
                return;
            engine->GCEnumCallback(m_arr);
        }

        void release_refs(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
        {
            (void)engine;

            if(!m_arr)
                return;
            assert(engine == m_arr->get_engine());
            m_arr->release();
            m_arr = nullptr;
        }

    private:
        script_array* m_arr = nullptr;
        size_type m_offset = 0;

        // Set script exception of invalid position
        static void raise_invalid_position()
        {
            set_script_exception("array_iterator<T>: invalid position");
        }

        static void raise_incompatible_iterator()
        {
            set_script_exception("array_iterator<T>: incompatible iterator");
        }
    };

    auto get_engine() const
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return get_type_info()->GetEngine();
    }

    int element_type_id() const
    {
        return m_data.element_type_id();
    }

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

        int subtype_id = element_type_id();
        if(is_primitive_type(subtype_id))
        {
            return visit_primitive_type(
                []<typename T>(
                    const T* lhs_start, const T* lhs_stop, const T* rhs_start
                ) -> bool
                {
                    return std::equal(
                        lhs_start,
                        lhs_stop,
                        rhs_start
                    );
                },
                subtype_id,
                m_data.data(),
                m_data.data_at(m_data.size()),
                other.m_data.data()
            );
        }
        else
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

            return true;
        }
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

#define ASBIND20_EXT_ARRAY_CHECK_CALLBACK(func_name, ret)                    \
    do {                                                                     \
        if(this->m_within_callback)                                          \
        {                                                                    \
            set_script_exception(                                            \
                "array<T>." #func_name "(): modifying array within callback" \
            );                                                               \
            return ret;                                                      \
        }                                                                    \
    } while(0)

    script_array& operator=(const script_array& other)
    {
        if(this == &other) [[unlikely]]
            return *this;

        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(opAssign, *this);

        m_data.clear();
        m_data.reserve(other.size());
        for(size_type i = 0; i < other.size(); ++i)
        {
            m_data.push_back(other[i]);
        }

        return *this;
    }

    void reserve(size_type new_cap)
    {
        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(reserve, void());

        m_data.reserve(new_cap);
    }

    void shrink_to_fit()
    {
        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(shrink_to_fit, void());

        m_data.shrink_to_fit();
    }

    void resize(size_type new_size)
    {
        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(resize, void());

        m_data.resize(new_size);
    }

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

    /**
     * @brief Remove matched elements
     *
     * @param val Value to remove
     * @param start Start position. Return 0 if start position is out of range
     * @param n Max checked elements
     * @return size_type Removed element count
     */
    size_type remove(const void* val, size_type start = 0, size_type n = -1)
    {
        if(start >= size())
            return 0;

        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(remove, 0);

        size_type removed = 0;

        int subtype_id = m_data.element_type_id();

        n = std::min(size() - start, n);

        if(is_primitive_type(subtype_id))
        {
            visit_primitive_type(
                [this, start, n, &removed]<typename T>(
                    const T* data, const T* val
                )
                {
                    for(size_type i = start; i + removed < n;)
                    {
                        if(data[i] != *val)
                        {
                            ++i;
                            continue;
                        }

                        i = static_cast<size_type>(m_data.remove(i));
                        removed += 1;
                    }
                },
                subtype_id,
                m_data.data(),
                val
            );
        }
        else
        {
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti = get_type_info();

            reuse_active_context ctx(ti->GetEngine());
            array_cache* cache = get_cache();
            for(size_type i = start; i + removed < n;)
            {
                bool eq = elem_opEquals(
                    subtype_id,
                    (*this)[i],
                    val,
                    ctx,
                    cache
                );
                if(!eq)
                {
                    ++i;
                    continue;
                }
                i = static_cast<size_type>(m_data.remove(i));
                removed += 1;
            }
        }

        if(removed > 0)
            m_data.erase(m_data.end() - removed, m_data.end());
        return removed;
    }

    /**
     * @brief Remove matched elements
     *
     * @param pred AngelScript signature: `bool pred(const T&in)`
     * @param start Start position. Return 0 if start position is out of range
     * @param n Max checked elements
     * @return size_type Removed element count
     */
    size_type remove_if(AS_NAMESPACE_QUALIFIER asIScriptFunction* pred, size_type start = 0, size_type n = -1)
    {
        if(start >= size())
            return 0;

        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(remove_if, 0);

        size_type removed = 0;

        n = std::min(size() - start, n);

        AS_NAMESPACE_QUALIFIER asITypeInfo* ti = get_type_info();

        callback_guard guard(this);
        reuse_active_context ctx(ti->GetEngine());
        for(size_type i = start; i + removed < n;)
        {
            auto eq = script_invoke<bool>(ctx, pred, m_data[i]);
            if(!eq.has_value() || !*eq)
            {
                ++i;
                continue;
            }
            i = static_cast<size_type>(m_data.remove(i));
            removed += 1;
        }

        if(removed > 0)
            m_data.erase(m_data.end() - removed, m_data.end());
        return removed;
    }

    size_type count(const void* val, size_type start = 0, size_type n = -1) const
    {
        if(start >= size()) [[unlikely]]
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
            for(size_type i = start; i < start + n; ++i)
            {
                if(elem_opEquals(subtype_id, m_data[i], val, ctx, cache))
                    ++result;
            }

            return result;
        }
    }

    size_type count_if(AS_NAMESPACE_QUALIFIER asIScriptFunction* pred, size_type start, size_type n = -1)
    {
        if(start >= size()) [[unlikely]]
            return 0;
        if(m_within_callback) [[unlikely]]
        {
            set_script_exception("array<T>.count_if: nested callback");
            return 0;
        }

        n = std::min(size() - start, n);

        size_type result = 0;

        callback_guard guard(this);
        reuse_active_context ctx(get_engine(), true);
        for(size_type i = start; i < start + n; ++i)
        {
            auto eq = script_invoke<bool>(ctx, pred, m_data[i]);
            if(eq.has_value() && *eq)
                result += 1;
        }

        return result;
    }

    void sort(size_type start = 0, size_type n = -1, bool asc = true)
    {
        if(start >= size()) [[unlikely]]
        {
            set_script_exception("array<T>.sort(): out of range");
            return;
        }

        n = std::min(size() - start, n);

        int subtype_id = element_type_id();
        if(is_primitive_type(subtype_id))
        {
            visit_primitive_type(
                [asc]<typename T>(T* start, T* stop)
                {
                    if(asc)
                    {
                        std::sort(
                            start, stop, std::less<T>{}
                        );
                    }
                    else
                    {
                        std::sort(
                            start, stop, std::greater<T>{}
                        );
                    }
                },
                subtype_id,
                m_data.data_at(start),
                m_data.data_at(start + n)
            );
        }
        else
        {
            auto helper = [&](auto&& f)
            {
                void** data = static_cast<void**>(m_data.data_at(start));
                std::sort(
                    data, data + n, std::move(f)
                );
            };

            array_cache* cache = get_cache();
            if(!cache) [[unlikely]]
            {
                set_script_exception("array<T>.sort(): internal error");
                return;
            }
            if(!cache->subtype_opCmp) [[unlikely]]
            {
                if(cache->opCmp_status == AS_NAMESPACE_QUALIFIER asMULTIPLE_FUNCTIONS)
                    set_script_exception("array<T>.sort(): multiple opCmp() functions");
                else
                    set_script_exception("array<T>.sort(): opCmp() function not found");
                return;
            }

            reuse_active_context ctx(get_engine(), true);

            auto comp = [cache, &ctx]<bool IsHandle, bool Asc>(
                            std::bool_constant<IsHandle>,
                            std::bool_constant<Asc>,
                            void* lhs,
                            void* rhs
                        ) -> bool
            {
                if(lhs == nullptr || rhs == nullptr)
                {
                    if constexpr(!IsHandle)
                        assert(false && "bad array");

                    if constexpr(Asc)
                        return lhs < rhs;
                    else
                        return lhs > rhs;
                }

                auto cmp = script_invoke<int>(
                    ctx, lhs, cache->subtype_opCmp, rhs
                );
                if(!cmp.has_value())
                    return false;

                if constexpr(Asc)
                    return *cmp < 0;
                else
                    return *cmp > 0;
            };

            if(is_objhandle(subtype_id))
            {
                if(asc)
                {
                    helper(
                        [&comp](void* lhs, void* rhs)
                        { return comp(std::true_type{}, std::true_type{}, lhs, rhs); }
                    );
                }
                else
                {
                    helper(
                        [&comp](void* lhs, void* rhs)
                        { return comp(std::true_type{}, std::false_type{}, lhs, rhs); }
                    );
                }
            }
            else
            {
                if(asc)
                {
                    helper(
                        [&comp](void* lhs, void* rhs)
                        { return comp(std::false_type{}, std::true_type{}, lhs, rhs); }
                    );
                }
                else
                {
                    helper(
                        [&comp](void* lhs, void* rhs)
                        { return comp(std::false_type{}, std::false_type{}, lhs, rhs); }
                    );
                }
            }
        }
    }

    void reverse(size_type start = 0, size_type n = -1)
    {
        m_data.reverse(start, n);
    }

    void reverse(script_array_iterator start)
    {
        if(start.get_array() != this) [[unlikely]]
        {
            start.raise_incompatible_iterator();
            return;
        }

        size_type start_offset = start.get_offset();
        if(start_offset >= size()) [[unlikely]]
        {
            start.raise_invalid_position();
            return;
        }

        m_data.reverse(start_offset);
    }

    void reverse(script_array_iterator start, script_array_iterator stop)
    {
        if(start.get_array() != this || stop.get_array() != this) [[unlikely]]
        {
            start.raise_incompatible_iterator();
            return;
        }

        size_type start_offset = start.get_offset();
        if(start_offset >= size()) [[unlikely]]
        {
            start.raise_invalid_position();
            return;
        }

        if(stop <= start)
            return;

        m_data.reverse(
            start_offset, static_cast<size_type>(stop - start)
        );
    }

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

    script_array_iterator erase(script_array_iterator it)
    {
        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(erase, script_array_iterator());

        if(this != it.m_arr) [[unlikely]]
        {
            set_script_exception("array<T>.erase(): incompatible iterator");
            return script_array_iterator();
        }
        size_type where = it.m_offset;
        if(where >= size()) [[unlikely]]
        {
            it.raise_invalid_position();
            return script_array_iterator();
        }

        m_data.erase(where);
        return it; // The offset should be unchanged
    }

    script_array_iterator insert(script_array_iterator it, const void* value)
    {
        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(insert, script_array_iterator());

        if(this != it.m_arr) [[unlikely]]
        {
            set_script_exception("array<T>.insert(): incompatible iterator");
            return script_array_iterator();
        }
        size_type where = it.m_offset;
        if(where > size()) [[unlikely]]
        {
            it.raise_invalid_position();
            return script_array_iterator();
        }

        m_data.insert(where, value);
        return it; // The offset should be unchanged
    }

    script_array_iterator find(const void* value, size_type start = 0, size_type n = -1) const
    {
        assert(value != nullptr);

        // Initial value is equivalent to the end() iterator
        size_type result = size();
        array_cache* cache = get_cache();
        if(!cache) [[unlikely]]
        {
            set_script_exception("array<T>: internal error");
            return script_array_iterator();
        }

        if(start >= size()) [[unlikely]]
        {
            goto result_found;
        }

        n = std::min(size() - start, n);

        if(int subtype_id = m_data.element_type_id(); is_primitive_type(subtype_id))
        {
            result = visit_primitive_type(
                []<typename T>(const T* start, const T* sentinel, const T* val) -> size_type
                {
                    return static_cast<size_type>(
                        std::find(start, sentinel, *val) - start
                    );
                },
                subtype_id,
                m_data.data_at(start),
                m_data.data_at(start + n),
                value
            );
            result += start;
            goto result_found;
        }
        else
        {
            reuse_active_context ctx(get_engine());
            for(size_type i = start; i < start + n; ++i)
            {
                bool eq = elem_opEquals(
                    subtype_id,
                    (*this)[i],
                    value,
                    ctx,
                    cache
                );

                if(eq)
                {
                    result = i;
                    goto result_found;
                }
            }
        }

result_found:
        return script_array_iterator(
            cache->iterator_ti, const_cast<script_array*>(this), result
        );
    }

private:
    script_array_iterator script_begin()
    {
        array_cache* cache = get_cache();
        if(!cache) [[unlikely]]
        {
            set_script_exception("array<T>: internal error");
            return script_array_iterator();
        }
        return script_array_iterator(
            cache->iterator_ti, this, 0
        );
    }

    script_array_iterator script_end()
    {
        array_cache* cache = get_cache();
        if(!cache) [[unlikely]]
        {
            set_script_exception("array<T>: internal error");
            return script_array_iterator();
        }
        return script_array_iterator(
            cache->iterator_ti, this, size()
        );
    }

    void* opIndex(size_type idx)
    {
        if(idx >= size())
        {
            set_script_exception("array<T>.opIndex(): out or range");
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
    using iter_t = script_array::script_array_iterator;
    using size_type = array_t::size_type;

#define ASBIND_EXT_ARRAY_MFN(name, ret, args) \
    fp<static_cast<ret(array_t::*) args>(&array_t::name)>

    auto helper = [engine, as_default]<bool UseGeneric>(std::bool_constant<UseGeneric>)
    {
        template_ref_class<array_t, UseGeneric> c(
            engine,
            "array<T>",
            AS_NAMESPACE_QUALIFIER asOBJ_GC
        );

        constexpr AS_NAMESPACE_QUALIFIER asQWORD iterator_flags =
            AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_CDAK |
            AS_NAMESPACE_QUALIFIER asOBJ_GC;

        template_value_class<iter_t, UseGeneric> it(
            engine,
            "array_iterator<T>",
            iterator_flags
        );
        template_value_class<iter_t, UseGeneric> cit(
            engine,
            "const_array_iterator<T>",
            iterator_flags
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
            .template factory<size_type>("uint n", use_explicit, use_policy<policies::notify_gc>)
            .template factory<size_type, const void*>("uint n, const T&in value", use_policy<policies::notify_gc>)
            .list_factory("repeat T", use_policy<policies::repeat_list_proxy, policies::notify_gc>)
            .opAssign()
            .opEquals()
            .method("uint get_size() const property", fp<&array_t::size>)
            .method("void set_size(uint) property", fp<&array_t::resize>)
            .method("uint resize(uint new_size)", fp<&array_t::resize>)
            .method("uint get_capacity() const property", fp<&array_t::capacity>)
            .method("void set_capacity(uint) property", fp<&array_t::reserve>)
            .method("void reserve(uint new_cap)", fp<&array_t::reserve>)
            .method("void shrink_to_fit()", fp<&array_t::shrink_to_fit>)
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
            .method("void sort(uint start=0, uint n=uint(-1), bool asc=true)", fp<&array_t::sort>)
            .method("void reverse(uint start=0, uint n=uint(-1))", ASBIND_EXT_ARRAY_MFN(reverse, void, (size_type, size_type)))
            .method("void reverse(const_array_iterator<T> start)", ASBIND_EXT_ARRAY_MFN(reverse, void, (iter_t)))
            .method("void reverse(const_array_iterator<T> start, const_array_iterator<T> stop)", ASBIND_EXT_ARRAY_MFN(reverse, void, (iter_t, iter_t)))
            .method("uint remove(const T&in, uint start=0, uint n=uint(-1)) const", fp<&array_t::remove>)
            .funcdef("bool remove_if_callback(const T&in)")
            .method("uint remove_if(const remove_if_callback&in, uint start=0, uint n=uint(-1)) const", fp<&array_t::remove_if>)
            .method("uint count(const T&in, uint start=0, uint n=uint(-1)) const", fp<&array_t::count>)
            .funcdef("bool count_if_callback(const T&in)")
            .method("uint count_if(const count_if_callback&in, uint start=0, uint n=uint(-1)) const", fp<&array_t::count_if>)
            .method("array_iterator<T> begin()", fp<&array_t::script_begin>)
            .method("array_iterator<T> end()", fp<&array_t::script_end>)
            .method("const_array_iterator<T> begin() const", fp<&array_t::script_begin>)
            .method("const_array_iterator<T> end() const", fp<&array_t::script_end>)
            .method("const_array_iterator<T> cbegin() const", fp<&array_t::script_begin>)
            .method("const_array_iterator<T> cend() const", fp<&array_t::script_end>)
            .method("array_iterator<T> erase(array_iterator<T> where)", ASBIND_EXT_ARRAY_MFN(erase, iter_t, (iter_t)))
            .method("const_array_iterator<T> erase(const_array_iterator<T> where)", ASBIND_EXT_ARRAY_MFN(erase, iter_t, (iter_t)))
            .method("array_iterator<T> find(const T&in, uint start=0, uint n=uint(-1))", fp<&array_t::find>)
            .method("const_array_iterator<T> find(const T&in, uint start=0, uint n=uint(-1)) const", fp<&array_t::find>)
            .method("array_iterator<T> insert(array_iterator<T> where, const T&in)", ASBIND_EXT_ARRAY_MFN(insert, iter_t, (iter_t, const void*)))
            .method("const_array_iterator<T> insert(const_array_iterator<T> where, const T&in)", ASBIND_EXT_ARRAY_MFN(insert, iter_t, (iter_t, const void*)));

        auto iterator_common = [](auto& r)
        {
            r
                .template_callback(fp<&array_t::template_callback>) // Reuse the template callback
                .default_constructor()
                .copy_constructor()
                .opAssign()
                .destructor()
                .opEquals()
                .opCmp()
                .opPreInc()
                .opPreDec()
                .opPostInc()
                .opPostDec()
                .method("array<T>@+ get_arr() const property", fp<&iter_t::get_array>)
                .property("const uint offset", &iter_t::m_offset)
                .template opConv<bool>()
                .enum_refs(fp<&iter_t::enum_refs>)
                .release_refs(fp<&iter_t::release_refs>);
        };

        iterator_common(it);
        it
            .method("T& get_value() const property", fp<&iter_t::value>)
            .template opImplConv<iter_t>("const_array_iterator<T>");

        iterator_common(cit);
        cit.method("const T& get_value() const property", fp<&iter_t::value>);

        if(as_default)
            c.as_array();

#undef ASBIND20_EXT_ARRAY_MFN
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
