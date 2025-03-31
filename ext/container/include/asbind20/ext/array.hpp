#ifndef ASBIND20_EXT_CONTAINER_ARRAY_HPP
#define ASBIND20_EXT_CONTAINER_ARRAY_HPP

#pragma once

#include <cstddef>
#include <asbind20/asbind.hpp>
#include <asbind20/container/small_vector.hpp>
#include <asbind20/operators.hpp>

namespace asbind20::ext
{
consteval auto default_script_array_user_id() noexcept
    -> AS_NAMESPACE_QUALIFIER asPWORD
{
#ifdef ASBIND20_EXT_SCRIPT_ARRAY_USER_ID
    return ASBIND20_EXT_SCRIPT_ARRAY_USER_ID;
#else
    return 2000;
#endif
}

// Implementation Note:
// Keep the code related to script array in header file,
// in order to support user customizing user data ID by defining macro.
// It may cause ODR-violation to put code in a separated source file.

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
    // Negative value means reverse index
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

        friend script_array_iterator operator+(const script_array_iterator& lhs, difference_type rhs) noexcept
        {
            script_array_iterator result = lhs;
            return result += rhs;
        }

        friend script_array_iterator operator+(difference_type lhs, const script_array_iterator& rhs) noexcept
        {
            script_array_iterator result = rhs;
            return result += lhs;
        }

        script_array_iterator& operator-=(difference_type diff) noexcept
        {
            *this += -diff;
            return *this;
        }

        friend script_array_iterator operator-(const script_array_iterator& lhs, difference_type rhs) noexcept
        {
            script_array_iterator result = lhs;
            return result -= rhs;
        }

        // Calculate distance between two iterators
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

        void* operator[](difference_type off) const
        {
            return (*this + off).value();
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

    /**
     * @brief Convert index to offset in array
     *
     * @param idx Index value. Negative value means reverse index.
     * @return size_type Offset in array, or size_type(-1) for invalid index
     */
    size_type index_to_offset(index_type idx) const noexcept
    {
        size_type sz = size();
        if(idx < 0)
        {
            size_type abs_idx = static_cast<size_type>(-idx);
            if(abs_idx > sz) [[unlikely]]
                return size_type(-1);
            else
                return sz - abs_idx;
        }
        else
        {
            if(static_cast<size_type>(idx) >= sz) [[unlikely]]
                return size_type(-1);
            return static_cast<size_type>(idx);
        }
    }

    void* operator[](size_type off)
    {
        return m_data[off];
    }

    const void* operator[](size_type off) const
    {
        return m_data[off];
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
        ++m_refcount;
    }

    void release() noexcept
    {
        m_gc_flag = false;
        m_refcount.dec_and_try_destroy(
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

    /**
     * @brief Remove matched elements
     *
     * @param val Value to remove
     * @param start Start position. Return 0 if start position is out of range
     * @param n Max checked elements
     * @return size_type Removed element count
     */
    size_type remove(const void* val, index_type start = 0, size_type n = -1)
    {
        size_type off = index_to_offset(start);
        if(off == size_type(-1))
        {
            return 0;
        }

        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(remove, 0);

        size_type removed = 0;

        int subtype_id = m_data.element_type_id();

        n = std::min(size() - off, n);

        if(is_primitive_type(subtype_id))
        {
            visit_primitive_type(
                [this, off, n, &removed]<typename T>(
                    const T* data, const T* val
                )
                {
                    for(size_type i = off; i + removed < n;)
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
            if(!cache) [[unlikely]]
                return 0;
            for(size_type i = off; i + removed < n;)
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
    size_type remove_if(AS_NAMESPACE_QUALIFIER asIScriptFunction* pred, index_type start = 0, size_type n = -1)
    {
        size_type off = index_to_offset(start);
        if(off == size_type(-1)) [[unlikely]]
            return 0;

        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(remove_if, 0);

        size_type removed = 0;

        n = std::min(size() - off, n);

        AS_NAMESPACE_QUALIFIER asITypeInfo* ti = get_type_info();

        callback_guard guard(this);
        reuse_active_context ctx(ti->GetEngine());
        for(size_type i = off; i + removed < n;)
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

    size_type count(const void* val, index_type start = 0, size_type n = -1) const
    {
        size_type off = index_to_offset(start);
        if(off >= size()) [[unlikely]]
            return 0;

        n = std::min(size() - off, n);

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
                m_data.data_at(off),
                m_data.data_at(off + n),
                val
            );
        }
        else
        {
            array_cache* cache = get_cache();
            reuse_active_context ctx(get_engine(), true);

            size_type result = 0;
            for(size_type i = off; i < off + n; ++i)
            {
                if(elem_opEquals(subtype_id, m_data[i], val, ctx, cache))
                    ++result;
            }

            return result;
        }
    }

    size_type count_if(AS_NAMESPACE_QUALIFIER asIScriptFunction* pred, index_type start = 0, size_type n = -1) const
    {
        size_type off = index_to_offset(start);
        if(off == size_type(-1)) [[unlikely]]
            return 0;
        if(m_within_callback) [[unlikely]]
        {
            set_script_exception("array<T>.count_if(): nested callback");
            return 0;
        }

        n = std::min(size() - off, n);

        size_type result = 0;

        callback_guard guard(this);
        reuse_active_context ctx(get_engine(), true);
        for(size_type i = off; i < off + n; ++i)
        {
            auto eq = script_invoke<bool>(ctx, pred, m_data[i]);
            if(eq.has_value() && *eq)
                result += 1;
        }

        return result;
    }

private:
    template <bool Stable, typename Compare>
    void sort_by_impl(Compare&& comp, size_type off, size_type n)
    {
        try
        {
            void** data = static_cast<void**>(m_data.data_at(off));
            if constexpr(Stable)
            {
                std::stable_sort(
                    data, data + n, std::forward<Compare>(comp)
                );
            }
            else
            {
                std::sort(
                    data, data + n, std::forward<Compare>(comp)
                );
            }
        }
        catch(const bad_script_invoke_result_access&)
        {
            // Exception caused by comparator.
            // Exception should have already been set to script context
            // by reuse_active_context
        }
    }

    template <bool IsHandle, bool Ascending, bool IsMethod>
    struct script_compare
    {
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx;
        AS_NAMESPACE_QUALIFIER asIScriptFunction* func;

        bool operator()(void* lhs, void* rhs) const
        {
            if(lhs == nullptr || rhs == nullptr)
            {
                if constexpr(!IsHandle)
                    assert(false && "bad array");

                if constexpr(Ascending)
                    return lhs < rhs;
                else
                    return lhs > rhs;
            }

            if constexpr(IsMethod) // opCmp of subtype
            {
                auto result = script_invoke<int>(
                    ctx, lhs, func, rhs
                );
                // Exception will be caught by the outer function
                int cmp = result.value();

                if constexpr(Ascending)
                    return cmp < 0;
                else
                    return cmp > 0;
            }
            else // callback function, whose signature is bool(const T&in, const T&in)
            {
                auto result = script_invoke<bool>(
                    ctx, func, lhs, rhs
                );
                // Exception will be caught by the outer function
                bool cmp = result.value();
                if constexpr(Ascending)
                    return cmp;
                else
                    return !cmp;
            }
        }
    };

    template <typename T>
    struct script_compare_primitive
    {
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx;
        AS_NAMESPACE_QUALIFIER asIScriptFunction* func;

        bool operator()(const T& lhs, const T& rhs) const
        {
            auto result = script_invoke<bool>(
                ctx, func, std::cref(lhs), std::cref(rhs)
            );
            // Exception will be caught by the outer function
            return result.value();
        }
    };

    template <bool IsMethod, bool Stable>
    void sort_by_script_compare_impl(
        AS_NAMESPACE_QUALIFIER asIScriptFunction* func,
        bool is_handle,
        bool asc,
        size_type off,
        size_type n
    )
    {
        reuse_active_context ctx(get_engine());
        if(is_handle)
        {
            if(asc)
            {
                sort_by_impl<Stable>(
                    script_compare<true, true, IsMethod>{ctx, func},
                    off,
                    n
                );
            }
            else
            {
                sort_by_impl<Stable>(
                    script_compare<true, false, IsMethod>{ctx, func},
                    off,
                    n
                );
            }
        }
        else
        {
            if(asc)
            {
                sort_by_impl<Stable>(
                    script_compare<false, true, IsMethod>{ctx, func},
                    off,
                    n
                );
            }
            else
            {
                sort_by_impl<Stable>(
                    script_compare<false, false, IsMethod>{ctx, func},
                    off,
                    n
                );
            }
        }
    }

    template <bool IsMethod>
    void sort_by_script_compare(
        AS_NAMESPACE_QUALIFIER asIScriptFunction* func,
        bool is_handle,
        bool asc,
        bool stable,
        size_type off,
        size_type n
    )
    {
        if(stable)
        {
            sort_by_script_compare_impl<IsMethod, true>(
                func, is_handle, asc, off, n
            );
        }
        else
        {
            sort_by_script_compare_impl<IsMethod, false>(
                func, is_handle, asc, off, n
            );
        }
    }

public:
    void sort(
        index_type start = 0, size_type n = -1, bool asc = true, bool stable = false
    )
    {
        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(sort, void());
        callback_guard guard(this);

        size_type off = index_to_offset(start);
        if(off == size_type(-1)) [[unlikely]]
        {
            set_script_exception("array<T>.sort(): out of range");
            return;
        }

        n = std::min(size() - off, n);

        int subtype_id = element_type_id();
        if(is_primitive_type(subtype_id))
        {
            visit_primitive_type(
                // Ignore `stable` for primitive types
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
                m_data.data_at(off),
                m_data.data_at(off + n)
            );
        }
        else
        {
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

            sort_by_script_compare<true>(
                cache->subtype_opCmp,
                is_objhandle(subtype_id),
                asc,
                off,
                stable,
                n
            );
        }
    }

    void sort_by(
        AS_NAMESPACE_QUALIFIER asIScriptFunction* func,
        index_type start = 0,
        size_type n = -1,
        bool stable = false
    )
    {
        assert(func != nullptr);

        ASBIND20_EXT_ARRAY_CHECK_CALLBACK(sort_by, void());
        callback_guard guard(this);

        size_type off = index_to_offset(start);
        if(off == size_type(-1)) [[unlikely]]
        {
            set_script_exception("array<T>.sort_by(): out of range");
            return;
        }

        n = std::min(size() - off, n);

        int subtype_id = element_type_id();
        if(is_primitive_type(subtype_id))
        {
            reuse_active_context ctx(get_engine());
            visit_primitive_type(
                [&ctx, func, stable]<typename T>(T* start, T* stop)
                {
                    if(stable)
                    {
                        std::stable_sort(
                            start,
                            stop,
                            script_compare_primitive<T>{ctx, func}
                        );
                    }
                    else
                    {
                        std::sort(
                            start,
                            stop,
                            script_compare_primitive<T>{ctx, func}
                        );
                    }
                },
                subtype_id,
                m_data.data_at(off),
                m_data.data_at(off + n)
            );
        }
        else
        {
            sort_by_script_compare<false>(
                func,
                is_objhandle(subtype_id),
                true,
                stable,
                off,
                n
            );
        }
    }

    void reverse(index_type start = 0, size_type n = -1)
    {
        size_type off = index_to_offset(start);
        if(off == size_type(-1))
        {
            set_script_exception("array<T>.reverse(): out of range");
            return;
        }
        m_data.reverse(off, n);
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
        return m_refcount;
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

private:
    // Returns size() if not found
    size_type find_impl(
        const void* value, size_type start_offset, size_type n, array_cache* cache
    ) const
    {
        assert(start_offset < size());

        n = std::min(size() - start_offset, n);

        if(int subtype_id = element_type_id(); is_primitive_type(subtype_id))
        {
            return visit_primitive_type(
                [this, start_offset]<typename T>(
                    const T* start, const T* sentinel, const T* val
                ) -> size_type
                {
                    const T* found = std::find(start, sentinel, *val);

                    if(found == sentinel)
                        return size();
                    else
                        return static_cast<size_type>(found - start) +
                               start_offset;
                },
                subtype_id,
                m_data.data_at(start_offset),
                m_data.data_at(start_offset + n),
                value
            );
        }
        else
        {
            reuse_active_context ctx(get_engine());
            for(size_type i = start_offset; i < start_offset + n; ++i)
            {
                bool eq = elem_opEquals(
                    subtype_id,
                    (*this)[i],
                    value,
                    ctx,
                    cache
                );

                if(eq)
                    return i;
            }

            return size();
        }
    }

public:
    script_array_iterator find(const void* value, index_type start = 0, size_type n = -1) const
    {
        assert(value != nullptr);

        // Initial value is equivalent to the end() iterator
        size_type result = size();
        array_cache* cache = get_cache();
        if(!cache || !cache->iterator_ti) [[unlikely]]
        {
            set_script_exception("array<T>: internal error");
            return script_array_iterator();
        }

        size_type off = index_to_offset(start);
        if(off == size_type(-1)) [[unlikely]]
        {
            goto result_found;
        }

        result = find_impl(value, off, n, cache);

result_found:
        return script_array_iterator(
            cache->iterator_ti, const_cast<script_array*>(this), result
        );
    }

    bool contains(const void* value, index_type start, size_type n = -1) const
    {
        assert(value != nullptr);

        array_cache* cache = get_cache();
        if(!cache) [[unlikely]]
        {
            set_script_exception("array<T>: internal error");
            return false;
        }

        size_type off = index_to_offset(start);
        if(off == size_type(-1)) [[unlikely]]
            return false;

        size_type found = find_impl(value, off, n, cache);
        return found != size();
    }

private:
    script_array_iterator script_begin()
    {
        array_cache* cache = get_cache();
        if(!cache || !cache->iterator_ti) [[unlikely]]
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
        if(!cache || !cache->iterator_ti) [[unlikely]]
        {
            set_script_exception("array<T>: internal error");
            return script_array_iterator();
        }
        return script_array_iterator(
            cache->iterator_ti, this, size()
        );
    }

public:
    void* opIndex(index_type idx)
    {
        size_type off = index_to_offset(idx);
        if(off == size_type(-1)) [[unlikely]]
        {
            set_script_exception("array<T>.opIndex(): out or range");
            return nullptr;
        }
        return (*this)[off];
    }

private:
    atomic_counter m_refcount;
    bool m_gc_flag = false;
    // Prevents array from begin modified within a callback of functions like find_if
    mutable bool m_within_callback = false;

    // TODO: Atomic flags for multithreading

    class callback_guard
    {
    public:
        callback_guard(const callback_guard&) = delete;

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
    using index_type = array_t::index_type;

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
            .method("void sort(int start=0, uint n=uint(-1), bool asc=true, bool stable=false)", fp<&array_t::sort>)
            .funcdef("bool sort_by_callback(const T&in, const T&in)")
            .method("void sort_by(const sort_by_callback&in, int start=0, uint n=uint(-1), bool asc=true, bool stable=false)", fp<&array_t::sort_by>)
            .method("void reverse(int start=0, uint n=uint(-1))", fp<overload_cast<index_type, size_type>(&array_t::reverse)>)
            .method("void reverse(const_array_iterator<T> start)", fp<overload_cast<iter_t>(&array_t::reverse)>)
            .method("void reverse(const_array_iterator<T> start, const_array_iterator<T> stop)", fp<overload_cast<iter_t, iter_t>(&array_t::reverse)>)
            .method("uint remove(const T&in, int start=0, uint n=uint(-1)) const", fp<&array_t::remove>)
            .funcdef("bool remove_if_callback(const T&in)")
            .method("uint remove_if(const remove_if_callback&in, int start=0, uint n=uint(-1)) const", fp<&array_t::remove_if>)
            .method("uint count(const T&in, int start=0, uint n=uint(-1)) const", fp<&array_t::count>)
            .funcdef("bool count_if_callback(const T&in)")
            .method("uint count_if(const count_if_callback&in, int start=0, uint n=uint(-1)) const", fp<&array_t::count_if>)
            .method("array_iterator<T> begin()", fp<&array_t::script_begin>)
            .method("array_iterator<T> end()", fp<&array_t::script_end>)
            .method("const_array_iterator<T> begin() const", fp<&array_t::script_begin>)
            .method("const_array_iterator<T> end() const", fp<&array_t::script_end>)
            .method("const_array_iterator<T> cbegin() const", fp<&array_t::script_begin>)
            .method("const_array_iterator<T> cend() const", fp<&array_t::script_end>)
            .method("array_iterator<T> erase(array_iterator<T> where)", fp<&array_t::erase>)
            .method("const_array_iterator<T> erase(const_array_iterator<T> where)", fp<&array_t::erase>)
            .method("array_iterator<T> find(const T&in, int start=0, uint n=uint(-1))", fp<&array_t::find>)
            .method("const_array_iterator<T> find(const T&in, int start=0, uint n=uint(-1)) const", fp<&array_t::find>)
            .method("bool contains(const T&in, int start=0, uint n=uint(-1)) const", fp<&array_t::contains>)
            .method("array_iterator<T> insert(array_iterator<T> where, const T&in)", fp<&array_t::insert>)
            .method("const_array_iterator<T> insert(const_array_iterator<T> where, const T&in)", fp<&array_t::insert>);

        using difference_type = iter_t::difference_type;
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
                .use(const_this + param<difference_type>)
                .use(param<difference_type> + const_this)
                .use(const_this - param<int>)
                .use(const_this - const_this)
                .use(_this += param<difference_type>)
                .use(_this -= param<difference_type>)
                .method("array<T>@+ get_arr() const property", fp<&iter_t::get_array>)
                .property("const uint offset", &iter_t::m_offset)
                .template opConv<bool>()
                .enum_refs(fp<&iter_t::enum_refs>)
                .release_refs(fp<&iter_t::release_refs>);
        };

        iterator_common(it);
        it
            .method("T& get_value() const property", fp<&iter_t::value>)
            .use(const_this[param<difference_type>]->template return_<void*>("T&"))
            .template opImplConv<iter_t>("const_array_iterator<T>");

        iterator_common(cit);
        cit
            .use(const_this[param<difference_type>]->template return_<void*>("const T&"))
            .method("const T& get_value() const property", fp<&iter_t::value>);

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

template <std::size_t Size>
script_array* new_script_array(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    meta::fixed_string<Size> subtype_decl
)
{
    using meta::fixed_string;

    auto full_decl = fixed_string("array<") + subtype_decl + fixed_string(">");
    AS_NAMESPACE_QUALIFIER asITypeInfo* ti = engine->GetTypeInfoByDecl(
        full_decl.c_str()
    );
    if(!ti) [[unlikely]]
        return nullptr;
    script_array* ptr = new script_array(ti);
    if(ti->GetFlags() & AS_NAMESPACE_QUALIFIER asOBJ_GC)
    {
        engine->NotifyGarbageCollectorOfNewObject(ptr, ti);
    }
    return ptr;
}
} // namespace asbind20::ext

#endif
