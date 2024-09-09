#ifndef ASBIND20_EXT_ARRAY_HPP
#define ASBIND20_EXT_ARRAY_HPP

#pragma once

#include "../asbind.hpp"

namespace asbind20::ext
{
asPWORD script_array_cache_id();

void register_script_array(asIScriptEngine* engine, bool as_default = true);

class script_array
{
    friend void register_script_array(asIScriptEngine* engine, bool as_default);

public:
    using size_type = asUINT;

    script_array() = delete;

    script_array(asITypeInfo* ti);

    script_array(const script_array&);

    script_array(asITypeInfo* ti, void* list_buf);

    ~script_array();

    script_array& operator=(const script_array& other);

    bool operator==(const script_array& other) const;

    void* operator[](asUINT idx);
    const void* operator[](asUINT idx) const;

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
    void foreach(F&& f)
    {
        for(size_type i = 0; i < size(); ++i)
            std::invoke(std::forward<F>(f), (*this)[i]);
    }

    void reserve(size_type new_cap);

    void push_back(void* ref);

private:
    struct array_data
    {
        size_type capacity = 0;
        size_type size = 0;
        std::byte* ptr = nullptr;
    };

    /**
     * @brief Move data to a newly allocated `array_data`. (INTERNAL)
     */
    void move_data(array_data& src, array_data& dst);

    void assign_impl(void* ptr, void* value);

    struct array_cache
    {
        asIScriptFunction* subtype_opCmp;
        asIScriptFunction* subtype_opEquals;
        int opCmp_status;
        int opEquals_status;
    };

    bool equals_impl(const void* lhs, const void* rhs, asIScriptContext* ctx, const array_cache* cache) const;

    void cache_data();
    static void cache_cleanup_callback(asITypeInfo* ti);

    int get_refcount() const;
    void set_gc_flag();
    bool get_gc_flag() const;
    void enum_refs(asIScriptEngine* engine);
    void release_refs(asIScriptEngine* engine);

    void* ref_at(asUINT idx);

    void* opIndex(asUINT idx);

    void* get_front();
    void* get_back();

    mutable int m_refcount = 1;
    mutable bool m_gc_flag = false;
    asITypeInfo* m_ti = nullptr;
    int m_subtype_id = 0;
    size_type m_elem_size = 0;
    array_data m_data = array_data();

    asQWORD subtype_flags() const;

    static void* allocate(std::size_t bytes);
    static void deallocate(void* ptr) noexcept;

    void alloc_data(size_type cap);

    void construct_elem(void* ptr, size_type n = 1);

    bool mem_grow_to(size_type new_cap);

    void copy_other_impl(script_array& other);
};

} // namespace asbind20::ext

#endif
