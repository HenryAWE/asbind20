#ifndef ASBIND20_EXT_CONTAINER_ARRAY_HPP
#define ASBIND20_EXT_CONTAINER_ARRAY_HPP

#pragma once

#include <mutex>
#include <cstddef>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/vocabulary.hpp>

namespace asbind20::ext
{
asPWORD script_array_cache_id();

namespace detail
{
    template <bool UseGeneric>
    void register_script_array_impl(asIScriptEngine* engine, bool as_default);
} // namespace detail

void register_script_array(asIScriptEngine* engine, bool as_default = true, bool generic = has_max_portability());

class script_array
{
    template <bool UseGeneric>
    friend void detail::register_script_array_impl(asIScriptEngine* engine, bool as_default);

    void notify_gc_for_this();

public:
    using size_type = asUINT;

    script_array() = delete;

    script_array(asITypeInfo* ti);

    script_array(const script_array&);

    script_array(asITypeInfo* ti, size_type n);

    script_array(asITypeInfo* ti, size_type n, const void* value);

    script_array(asITypeInfo* ti, void* list_buf);

private:
    ~script_array();

public:
    script_array& operator=(const script_array& other);

    void* operator new(std::size_t bytes);
    void operator delete(void* p);

    bool operator==(const script_array& other) const;

    void* operator[](size_type idx);
    const void* operator[](size_type idx) const;

    void* data() noexcept
    {
        return m_data.ptr;
    }

    const void* data() const noexcept
    {
        return m_data.ptr;
    }

    bool element_indirect() const;

    void* pointer_to(size_type idx);
    const void* pointer_to(size_type idx) const;

    void addref();
    void release();

    size_type size() const noexcept
    {
        return m_data.size;
    }

    size_type capacity() const noexcept
    {
        return m_data.capacity;
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }

    void reserve(size_type new_cap);
    void shrink_to_fit();

    void clear();

    void push_back(const void* value);

    void pop_back();

    void append_range(const script_array& rng, size_type n = -1);

    void insert(size_type idx, void* value);

    void insert_range(size_type idx, const script_array& rng, size_type n = -1);

    void erase(size_type idx, size_type n = 1);

    size_type erase_value(const void* val, size_type idx = 0, size_type n = -1);

    void sort(size_type idx = 0, size_type n = -1, bool asc = true);

    void reverse(size_type idx = 0, size_type n = -1);

    size_type find(const void* value, size_type idx = 0) const;

    bool contains(const void* value, size_type idx = 0) const;

    size_type count(const void* value, size_type idx = 0, size_type n = -1) const;

    script_optional* find_optional(const void* val, size_type idx = 0);

    [[nodiscard]]
    asITypeInfo* script_type_info() const noexcept
    {
        return m_ti;
    }

    asQWORD subtype_flags() const;

    void lock()
    {
        m_mx.lock();
    }

    void unlock() noexcept
    {
        m_mx.unlock();
    }

private:
    struct array_data
    {
        size_type capacity = 0;
        size_type size = 0;
        std::byte* ptr = nullptr;
    };

    void copy_construct_range(void* start, const void* rng_start, size_type n);
    void move_construct_range(void* start, void* rng_start, size_type n);
    void move_construct_range_backward(void* start, void* rng_start, size_type n);
    void copy_assign_range_backward(void* start, const void* rng_start, size_type n);

    void insert_range_impl(size_type idx, const void* src, size_type n);

    void copy_assign_at(void* ptr, void* value);

    void destroy_n(void* start, size_type n);

    struct array_cache
    {
        asIScriptFunction* subtype_opCmp;
        asIScriptFunction* subtype_opEquals;
        int opCmp_status;
        int opEquals_status;
    };

    bool elem_opEquals(const void* lhs, const void* rhs, asIScriptContext* ctx, const array_cache* cache) const;

    void cache_data();
    static void cache_cleanup_callback(asITypeInfo* ti);

    int get_refcount() const;
    void set_gc_flag();
    bool get_gc_flag() const;
    void enum_refs(asIScriptEngine* engine);
    void release_refs(asIScriptEngine* engine);

    void* opIndex(size_type idx);

    void* get_front();
    void* get_back();

    void set_front(void* value);
    void set_back(void* value);

    array_data m_data = array_data();
    asITypeInfo* m_ti = nullptr;
    int m_refcount = 1;
    int m_subtype_id = 0;
    size_type m_elem_size = 0;
    bool m_gc_flag = false;
    mutable bool m_within_callback = false;
    std::mutex m_mx;

    class callback_guard
    {
    public:
        callback_guard(const script_array& this_) noexcept;

        ~callback_guard();

    private:
        const script_array& m_this;
    };

    static void* allocate(std::size_t bytes);
    static void deallocate(void* mem) noexcept;

    void default_construct_n(void* start, size_type n);
    void value_construct_n(void* start, const void* value, size_type n);

    bool mem_resize_to(size_type new_cap);

    void erase_impl(size_type idx, size_type n);

    size_type script_find_if(asIScriptFunction* fn, size_type idx = 0) const;

    size_type script_erase_if(asIScriptFunction* fn, size_type idx, size_type n);

    void script_for_each(asIScriptFunction* fn, size_type idx = 0, size_type n = -1) const;

    void script_sort_by(asIScriptFunction* fn, size_type idx = 0, size_type n = -1, bool stable = true);

    size_type script_count_if(asIScriptFunction* fn, size_type idx = 0, size_type n = -1) const;
};
} // namespace asbind20::ext

#endif
