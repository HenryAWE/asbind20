#ifndef ASBIND20_EXT_ARRAY_HPP
#define ASBIND20_EXT_ARRAY_HPP

#pragma once

#include <mutex>
#include <cstddef>
#include "../asbind.hpp"

namespace asbind20::ext
{
asPWORD script_array_cache_id();

void register_script_array(asIScriptEngine* engine, bool as_default = true);

class script_array
{
    friend void register_script_array(asIScriptEngine* engine, bool as_default);

    void notify_gc_for_this();

public:
    using size_type = asUINT;

    script_array() = delete;

    script_array(asITypeInfo* ti);

    script_array(const script_array&);

    script_array(asITypeInfo* ti, size_type n);

    script_array(asITypeInfo* ti, size_type n, const void* value);

    script_array(asITypeInfo* ti, void* list_buf);

    ~script_array();

    script_array& operator=(const script_array& other);

    void* operator new(std::size_t bytes);
    void operator delete(void* p);

    void swap(script_array& other) noexcept;

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

    template <typename F>
    void for_each(F&& f) const
    {
        for(size_type i = 0; i < size(); ++i)
            std::invoke(std::forward<F>(f), pointer_to(i));
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

    void sort(size_type idx = 0, size_type n = -1, bool asc = true);

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

    mutable int m_refcount = 1;
    mutable bool m_gc_flag = false;
    asITypeInfo* m_ti = nullptr;
    int m_subtype_id = 0;
    size_type m_elem_size = 0;
    array_data m_data = array_data();
    std::mutex m_mx;

    static void* allocate(std::size_t bytes);
    static void deallocate(void* mem) noexcept;

    void default_construct_n(void* start, size_type n);
    void value_construct_n(void* start, const void* value, size_type n);

    bool mem_resize_to(size_type new_cap);
};
} // namespace asbind20::ext

#endif
