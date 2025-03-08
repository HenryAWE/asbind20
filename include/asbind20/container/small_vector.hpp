/**
 * @file small_vector.hpp
 * @author HenryAWE
 * @brief Vector with small size optimization (SSO) for AngelScript object
 */

#ifndef ASBIND20_CONTAINER_SMALL_VECTOR_HPP
#define ASBIND20_CONTAINER_SMALL_VECTOR_HPP

#pragma once

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include "../utility.hpp"
#include "helper.hpp"

namespace asbind20::container
{
namespace detail
{
    constexpr std::size_t accommodate(std::size_t new_cap, std::size_t current_cap) noexcept
    {
        // If the new capacity is only slightly larger than the current capacity,
        // make the new capacity two times of the current capacity.
        if(new_cap < current_cap * 2)
            new_cap = current_cap * 2;

        return new_cap;
    }

    class small_vector_impl
    {
    protected:
        [[noreturn]]
        static void throw_out_of_range()
        {
            throw std::out_of_range("small vector out of range");
        }
    };
} // namespace detail

template <
    typeinfo_policy TypeInfoPolicy,
    std::size_t StaticCapacityBytes = 4 * sizeof(void*),
    typename Allocator = as_allocator<void>>
requires(StaticCapacityBytes > 0)
class small_vector : detail::small_vector_impl
{
public:
    static_assert(
        StaticCapacityBytes % sizeof(void*) == 0,
        "static storage size must be aligned with size of pointer"
    );

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type = Allocator;
    using pointer = void*;
    using const_pointer = const void*;

private:
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    if __GNUC__ >= 14
#        pragma GCC diagnostic ignored "-Warray-bounds="
#    endif
#endif

    class impl_interface
    {
    public:
        impl_interface(int type_id, AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
            : m_type_id(type_id), m_ti(ti) {}

        ~impl_interface() = default;

        auto get_engine() const
            -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
        {
            assert(m_ti != nullptr);
            return m_ti->GetEngine();
        }

        auto get_type_info() const noexcept
            -> AS_NAMESPACE_QUALIFIER asITypeInfo*
        {
            return m_ti;
        }

        int elem_type_id() const
        {
            return m_type_id;
        }

        auto elem_type_info() const
            -> AS_NAMESPACE_QUALIFIER asITypeInfo*
        {
            return TypeInfoPolicy::get_type_info(m_ti);
        }

        // The iterator interface only stores an unsigned offset value,
        // so an invalid iterator from script can be reported by host,
        // instead of crashing the application by accessing invalid memory address.
        class iterator_interface
        {
        public:
            iterator_interface() noexcept = default;

            iterator_interface(const iterator_interface&) noexcept = default;

            iterator_interface(size_type off) noexcept
                : m_off(off) {}

            void advance(difference_type diff) noexcept
            {
                if(diff < 0 && difference_type(m_off) + diff < 0) [[unlikely]]
                    m_off = 0; // guard underflow
                else
                    m_off += diff;
            }

            void inc() noexcept
            {
                advance(1);
            }

            void dec() noexcept
            {
                if(m_off == 0) [[unlikely]]
                    return;
                --m_off;
            }

            size_type get_offset() const noexcept
            {
                return m_off;
            }

        private:
            size_type m_off = 0;
        };

    private:
        // TODO: Optimize the memory layout if the element type is subtype.
        // Note that GetTypeInfo APIs may ignore the handle bit, i.e. GetTypeInfoById(type_id)->GetTypeId() may not equal to type_id,
        // so we need to store the type ID separately for typeinfo_identity.
        // See: https://www.gamedev.net/forums/topic/718032-inconsistent-result-of-asiscriptmodule-gettypeinfobydecl-and-gettypeidbydecl/
        int m_type_id;
        // Although having a type info pointer is useless for a primitive element type,
        // it can keep the interface consistent with other types.
        script_typeinfo m_ti;
    };

    template <int TypeId>
    class impl_primitive_base : public impl_interface
    {
    public:
        using impl_interface::impl_interface;

        int elem_type_id() const noexcept
        {
            assert(impl_interface::elem_type_id() == TypeId);
            return TypeId;
        }
    };

    // Element storage
    template <typename ValueType, typename Base>
    class impl_storage : public Base
    {
    public:
        using value_type = ValueType;
        using pointer = value_type*;
        using const_pointer = const value_type*;

        using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<value_type>;

        template <typename... Args>
        impl_storage(Args&&... args)
            : Base(std::forward<Args>(args)...)
        {
            m_p_begin = get_static_storage();
            m_p_end = m_p_begin;
            m_p_capacity = m_p_begin + max_static_size();
        }

        ~impl_storage()
        {
            if(m_p_begin != get_static_storage())
            {
                std::allocator_traits<allocator_type>::deallocate(
                    m_alloc.second(), m_p_begin, size()
                );
            }
        }

        static consteval size_type max_static_size()
        {
            return StaticCapacityBytes / sizeof(value_type);
        }

        size_type static_capacity() const
        {
            return max_static_size();
        }

        size_type capacity() const
        {
            return m_p_capacity - m_p_begin;
        }

        size_type size() const
        {
            return m_p_end - m_p_begin;
        }

        void* value_ref_at(size_type idx) const
        {
            if(idx >= size())
                return nullptr;
            return &m_p_begin[idx];
        }

        void* data_at(size_type idx) const noexcept
        {
            return m_p_begin + idx;
        }

        void* data() const noexcept
        {
            return m_p_begin;
        }

        void reserve(size_type new_cap)
        {
            if(new_cap <= capacity())
                return;

            assert(new_cap > static_capacity());
            new_cap = detail::accommodate(new_cap, capacity());
            size_type current_size = size();
            pointer tmp = std::allocator_traits<allocator_type>::allocate(
                m_alloc.second(), new_cap
            );
            std::memcpy(tmp, m_p_begin, current_size * sizeof(value_type));

            if(m_p_begin != get_static_storage())
            {
                std::allocator_traits<allocator_type>::deallocate(
                    m_alloc.second(), m_p_begin, capacity()
                );
            }

            m_p_begin = tmp;
            m_p_end = m_p_begin + current_size;
            m_p_capacity = m_p_begin + new_cap;
        }

        void clear() noexcept
        {
            m_p_end = m_p_begin;
        }

        void push_back(const void* ref)
        {
            reserve(size() + 1);
            emplace_back_impl(*static_cast<const value_type*>(ref));
        }

        void emplace_back()
        {
            reserve(size() + 1);
            emplace_back_impl(value_type());
        }

        void pop_back() noexcept
        {
            if(this->size() == 0)
                return;
            --m_p_end;
        }

        void insert_one(size_type where, const void* ref)
        {
            if(size_type current_size = size(); where > current_size)
                throw_out_of_range();
            else if(where == current_size)
            {
                reserve(size() + 1);
                emplace_back_impl(*static_cast<const value_type*>(ref));
                return;
            }

            if(size() + 1 <= capacity())
            {
                pointer p_where = m_p_begin + where;
                size_type elem_to_move = m_p_end - p_where;
                std::memmove(
                    p_where + 1,
                    p_where,
                    elem_to_move * sizeof(value_type)
                );
                ++m_p_end;
                *p_where = *static_cast<const value_type*>(ref);
            }
            else
            {
                size_type new_cap = detail::accommodate(size() + 1, capacity());
                size_type current_size = size();
                pointer tmp = std::allocator_traits<allocator_type>::allocate(
                    m_alloc.second(), new_cap
                );

                std::memcpy(
                    tmp, m_p_begin, where * sizeof(value_type)
                );
                *(tmp + where) = *static_cast<const value_type*>(ref);
                std::memcpy(
                    tmp + where + 1,
                    m_p_begin + where,
                    (current_size - where) * sizeof(value_type)
                );

                if(m_p_begin != get_static_storage())
                {
                    std::allocator_traits<allocator_type>::deallocate(
                        m_alloc.second(), m_p_begin, capacity()
                    );
                }

                m_p_begin = tmp;
                m_p_end = m_p_begin + current_size + 1;
                m_p_capacity = m_p_begin + new_cap;
            }
        }

        void erase_n(size_type start, size_type n)
        {
            if(start >= size())
                throw_out_of_range();

            n = std::min(size() - start, n);
            if(n == 0) [[unlikely]]
                return;
            else if(n == size())
            {
                assert(start == 0);
                clear();
                return;
            }

            pointer p_start = m_p_begin + start;
            size_type elem_to_move = size() - start - n;
            std::memmove(
                p_start,
                p_start + n,
                elem_to_move * sizeof(value_type)
            );
            m_p_end -= n;
        }

    protected:
        pointer get_static_storage() noexcept
        {
            return reinterpret_cast<pointer>(m_alloc.first());
        }

        const_pointer get_static_storage() const noexcept
        {
            return reinterpret_cast<pointer>(m_alloc.first());
        }

        void emplace_back_impl(value_type val) noexcept
        {
            assert(capacity() - size() >= 1);
            *m_p_end = val;
            ++m_p_end;
        }

        pointer m_p_begin;
        pointer m_p_end;
        pointer m_p_capacity;

        alignas(value_type) meta::compressed_pair<std::byte[StaticCapacityBytes], allocator_type> m_alloc;
    };

    template <int TypeId>
    using impl_primitive = impl_storage<primitive_type_of_t<TypeId>, impl_primitive_base<TypeId>>;
    using impl_enum = impl_storage<int, impl_interface>;

    template <bool IsHandle>
    class impl_object :
        public impl_storage<void*, impl_interface>
    {
        using my_base = impl_storage<void*, impl_interface>;

    public:
        using value_type = void*;
        using pointer = void**;
        using const_pointer = void* const*;
        using allocator_type = my_base::allocator_type;

        using my_base::my_base;

        ~impl_object()
        {
            clear();
        }

        void* value_ref_at(size_type idx) const
        {
            if(idx >= this->size())
                return nullptr;
            if constexpr(IsHandle)
                return &this->m_p_begin[idx];
            else
                return this->m_p_begin[idx];
        }

        void clear() noexcept
        {
            release_obj_n(this->m_p_begin, this->size());
            this->m_p_end = this->m_p_end;
        }

        void push_back(const void* ref)
        {
            this->reserve(this->size() + 1);

            void* obj = copy_obj(ref_to_obj(ref));
            *this->m_p_end = obj;

            ++this->m_p_end;
        }

        void emplace_back()
        {
            if constexpr(IsHandle)
            {
                my_base::emplace_back();
            }
            else
            {
                this->reserve(this->size() + 1);
                AS_NAMESPACE_QUALIFIER asITypeInfo* ti = this->elem_type_info();
                assert(ti != nullptr);

                *this->m_p_end = ti->GetEngine()->CreateScriptObject(ti);
                ++this->m_p_end;
            }
        }

        void pop_back() noexcept
        {
            if(this->size() == 0)
                return;

            AS_NAMESPACE_QUALIFIER asITypeInfo* ti = this->elem_type_info();
            assert(ti != nullptr);

            void* obj = *(this->m_p_end - 1);
            if constexpr(IsHandle)
                assert(obj != nullptr);
            if(obj)
                ti->GetEngine()->ReleaseScriptObject(obj, ti);

            --this->m_p_end;
        }

        void insert_one(size_type where, const void* ref)
        {
            if(size_type current_size = this->size(); where > current_size)
                throw_out_of_range();
            else if(where == current_size)
            {
                this->impl_object::push_back(ref);
                return;
            }

            if(this->size() + 1 <= this->capacity())
            {
                void* obj = copy_obj(ref_to_obj(ref));

                pointer p_where = this->m_p_begin + where;
                size_type elem_to_move = this->m_p_end - p_where;
                std::memmove(
                    p_where + 1,
                    p_where,
                    elem_to_move * sizeof(value_type)
                );
                ++this->m_p_end;
                *p_where = obj;
            }
            else
            {
                void* obj = copy_obj(ref_to_obj(ref));
                if(!obj) [[unlikely]] // exception in copy constructor
                    return;

                size_type new_cap = detail::accommodate(this->size() + 1, this->capacity());
                size_type current_size = this->size();
                pointer tmp = std::allocator_traits<allocator_type>::allocate(
                    this->m_alloc.second(), new_cap
                );

                std::memcpy(
                    tmp, this->m_p_begin, where * sizeof(value_type)
                );
                *(tmp + where) = obj;
                std::memcpy(
                    tmp + where + 1,
                    this->m_p_begin + where,
                    (current_size - where) * sizeof(value_type)
                );

                if(this->m_p_begin != this->get_static_storage())
                {
                    std::allocator_traits<allocator_type>::deallocate(
                        this->m_alloc.second(), this->m_p_begin, this->capacity()
                    );
                }

                this->m_p_begin = tmp;
                this->m_p_end = this->m_p_begin + current_size + 1;
                this->m_p_capacity = this->m_p_begin + new_cap;
            }
        }

        void erase_n(size_type start, size_type n)
        {
            if(start >= this->size()) [[unlikely]]
                throw_out_of_range();

            n = std::min(this->size() - start, n);
            if(n == 0) [[unlikely]]
                return;
            else if(n == this->size())
            {
                assert(start == 0);
                clear();
                return;
            }

            void** p_start = this->m_p_begin + start;
            release_obj_n(p_start, n);
            size_type elem_to_move = this->size() - start - n;
            std::memmove(
                p_start,
                p_start + n,
                elem_to_move * sizeof(void*)
            );
            this->m_p_end -= n;
        }

    private:
        static void* ref_to_obj(const void* ref) noexcept
        {
            if constexpr(IsHandle)
                return *static_cast<void* const*>(ref);
            else
                return const_cast<void*>(ref);
        }

        // NOTE: Call ref_to_obj to convert the pointer at first!
        void* copy_obj(void* obj) const
        {
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti = this->elem_type_info();
            assert(ti != nullptr);

            if constexpr(IsHandle)
            {
                if(!obj)
                    return nullptr;
                ti->GetEngine()->AddRefScriptObject(obj, ti);
                return obj;
            }
            else
            {
                assert(obj != nullptr);
                return ti->GetEngine()->CreateScriptObjectCopy(obj, ti);
            }
        }

        void release_obj_n(void** objs, size_type n) const noexcept
        {
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti = this->elem_type_info();
            AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = ti->GetEngine();
            for(size_type i = 0; i < n; ++i)
            {
                void* obj = objs[i];
                if(!obj)
                    continue;
                engine->ReleaseScriptObject(obj, ti);
            }
        }
    };

#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

    static constexpr std::size_t impl_storage_size =
        std::max(sizeof(impl_enum), sizeof(impl_object<false>));

    std::byte m_impl_data[impl_storage_size];

    impl_interface& impl() noexcept
    {
        return *reinterpret_cast<impl_interface*>(m_impl_data);
    }

    const impl_interface& impl() const noexcept
    {
        return *reinterpret_cast<const impl_interface*>(m_impl_data);
    }

    template <typename Visitor>
    decltype(auto) visit_impl(Visitor&& vis)
    {
        int type_id = element_type_id();
        auto& base = impl();

        if(is_primitive_type(type_id))
        {
            assert(!is_void_type(type_id));

            switch(type_id)
            {
#define ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(as_type_id)                \
case AS_NAMESPACE_QUALIFIER as_type_id:                                       \
    return vis(                                                               \
        static_cast<impl_primitive<AS_NAMESPACE_QUALIFIER as_type_id>&>(base) \
    )

                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_BOOL);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_INT8);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_INT16);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_INT32);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_INT64);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_UINT8);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_UINT16);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_UINT32);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_UINT64);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_FLOAT);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_DOUBLE);

#undef ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE

            default:
                assert(is_enum_type(type_id));
                return vis(
                    static_cast<impl_enum&>(base)
                );
            }
        }
        else
        {
            if(is_objhandle(type_id))
            {
                return vis(
                    static_cast<impl_object<true>&>(base)
                );
            }
            else
            {
                return vis(
                    static_cast<impl_object<false>&>(base)
                );
            }
        }
    }

    template <typename Visitor>
    decltype(auto) visit_impl(Visitor&& vis) const
    {
        int type_id = element_type_id();
        const auto& base = impl();

        if(is_primitive_type(type_id))
        {
            assert(!is_void_type(type_id));

            switch(type_id)
            {
#define ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(as_type_id)                      \
case AS_NAMESPACE_QUALIFIER as_type_id:                                             \
    return vis(                                                                     \
        static_cast<const impl_primitive<AS_NAMESPACE_QUALIFIER as_type_id>&>(base) \
    )

                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_BOOL);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_INT8);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_INT16);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_INT32);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_INT64);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_UINT8);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_UINT16);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_UINT32);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_UINT64);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_FLOAT);
                ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE(asTYPEID_DOUBLE);

#undef ASBIND20_CONTAINER_SMALL_VECTOR_VISIT_CASE

            default:
                assert(is_enum_type(type_id));
                return vis(
                    static_cast<const impl_enum&>(base)
                );
            }
        }
        else
        {
            if(is_objhandle(type_id))
            {
                return vis(
                    static_cast<const impl_object<true>&>(base)
                );
            }
            else
            {
                return vis(
                    static_cast<const impl_object<false>&>(base)
                );
            }
        }
    }

    template <typename... Args>
    void init_impl(int type_id, AS_NAMESPACE_QUALIFIER asITypeInfo* ti, Args&&... args)
    {
        assert(!is_void_type(type_id));

        if(!is_primitive_type(type_id))
        {
            if(is_objhandle(type_id))
                new(m_impl_data) impl_object<true>(type_id, ti, std::forward<Args>(args)...);
            else
                new(m_impl_data) impl_object<false>(type_id, ti, std::forward<Args>(args)...);
            return;
        }

        switch(type_id)
        {
#define ASBIND20_SMALL_VECTOR_IMPL_CASE(as_type_id) \
case AS_NAMESPACE_QUALIFIER as_type_id:             \
    new(m_impl_data) impl_primitive<as_type_id>(    \
        type_id, ti, std::forward<Args>(args)...    \
    );                                              \
    break

            ASBIND20_SMALL_VECTOR_IMPL_CASE(asTYPEID_BOOL);
            ASBIND20_SMALL_VECTOR_IMPL_CASE(asTYPEID_INT8);
            ASBIND20_SMALL_VECTOR_IMPL_CASE(asTYPEID_INT16);
            ASBIND20_SMALL_VECTOR_IMPL_CASE(asTYPEID_INT32);
            ASBIND20_SMALL_VECTOR_IMPL_CASE(asTYPEID_INT64);
            ASBIND20_SMALL_VECTOR_IMPL_CASE(asTYPEID_UINT8);
            ASBIND20_SMALL_VECTOR_IMPL_CASE(asTYPEID_UINT16);
            ASBIND20_SMALL_VECTOR_IMPL_CASE(asTYPEID_UINT32);
            ASBIND20_SMALL_VECTOR_IMPL_CASE(asTYPEID_UINT64);
            ASBIND20_SMALL_VECTOR_IMPL_CASE(asTYPEID_FLOAT);
            ASBIND20_SMALL_VECTOR_IMPL_CASE(asTYPEID_DOUBLE);

        default:
            assert(is_enum_type(type_id));
            new(m_impl_data) impl_enum(type_id, ti, std::forward<Args>(args)...);
            break;

#undef ASBIND20_SMALL_VECTOR_IMPL_CASE
        }
    }

public:
    small_vector() = delete;

    ~small_vector()
    {
        visit_impl(
            []<typename ImplType>(ImplType& impl)
            {
                impl.~ImplType();
            }
        );
    }

    explicit small_vector(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
    {
        init_impl(
            TypeInfoPolicy::get_type_id(ti), ti
        );
    }

    explicit small_vector(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id)
    {
        if(is_primitive_type(type_id) && !is_enum_type(type_id))
            init_impl(type_id, nullptr);
        else
        {
            assert(engine != nullptr);
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti = engine->GetTypeInfoById(type_id);
            assert(ti != nullptr);
            init_impl(type_id, ti);
        }
    }

    auto get_type_info() const
        -> AS_NAMESPACE_QUALIFIER asITypeInfo*
    {
        return impl().get_type_info();
    }

    auto element_type_info() const
        -> AS_NAMESPACE_QUALIFIER asITypeInfo*
    {
        return TypeInfoPolicy::get_type_info(get_type_info());
    }

    int element_type_id() const
    {
        return impl().elem_type_id();
    }

    size_type static_capacity() const noexcept
    {
        return visit_impl(
            [](auto& impl)
            { return impl.static_capacity(); }
        );
    }

    size_type capacity() const noexcept
    {
        return visit_impl(
            [](auto& impl)
            { return impl.capacity(); }
        );
    }

    size_type size() const noexcept
    {
        return visit_impl(
            [](auto& impl)
            { return impl.size(); }
        );
    }

    void clear() noexcept
    {
        return visit_impl(
            [](auto& impl)
            { return impl.clear(); }
        );
    }

    bool empty() const noexcept
    {
        return size() == 0;
    }

    void* data() noexcept
    {
        return visit_impl(
            [](auto& impl)
            { return impl.data(); }
        );
    }

    const void* data() const noexcept
    {
        return visit_impl(
            [](auto& impl)
            { return impl.data(); }
        );
    }

    void* operator[](size_type idx) noexcept
    {
        return visit_impl(
            [idx](auto& impl)
            { return impl.value_ref_at(idx); }
        );
    }

    const void* operator[](size_type idx) const noexcept
    {
        return visit_impl(
            [idx](auto& impl)
            { return impl.value_ref_at(idx); }
        );
    }

    void push_back(const void* ref)
    {
        return visit_impl(
            [ref](auto& impl)
            { return impl.push_back(ref); }
        );
    }

    void emplace_back()
    {
        return visit_impl(
            [](auto& impl)
            { return impl.emplace_back(); }
        );
    }

    void pop_back() noexcept
    {
        return visit_impl(
            [](auto& impl)
            { return impl.pop_back(); }
        );
    }

    class iterator;

    class const_iterator : private impl_interface::iterator_interface
    {
        friend iterator;
        friend small_vector;

    public:
        using value_type = const void*;
        using iterator_category = std::random_access_iterator_tag;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = const void*;
        using reference = const void*;
        using const_reference = const void*;

        const_iterator() noexcept = default;

        const_iterator(const const_iterator&) noexcept = default;

    private:
        const_iterator(const small_vector* p_vec, size_type off) noexcept
            : impl_interface::iterator_interface(off), m_p_vec(p_vec) {}

    public:
        reference operator*() const noexcept
        {
            if(!m_p_vec) [[unlikely]]
                return nullptr;
            return (*m_p_vec)[this->get_offset()];
        }

        bool operator==(const const_iterator& rhs) const noexcept
        {
            assert(this->get_container() == rhs.get_container());
            return this->get_offset() == rhs.get_offset();
        }

        std::strong_ordering operator<=>(const const_iterator& rhs) const noexcept
        {
            assert(this->get_container() == rhs.get_container());
            return this->get_offset() <=> rhs.get_offset();
        }

        const_iterator& operator++() noexcept
        {
            this->inc();
            return *this;
        }

        const_iterator& operator--() noexcept
        {
            this->dec();
            return *this;
        }

        const_iterator& operator++(int) noexcept
        {
            const_iterator tmp(*this);
            ++*this;
            return tmp;
        }

        const_iterator& operator--(int) noexcept
        {
            const_iterator tmp(*this);
            --*this;
            return tmp;
        }

        const_iterator& operator+=(difference_type diff) noexcept
        {
            this->advance(diff);
            return *this;
        }

        const_iterator& operator-=(difference_type diff) noexcept
        {
            this->advance(-diff);
            return *this;
        }

        friend const_iterator operator+(const_iterator lhs, const_iterator rhs) noexcept
        {
            return lhs += rhs;
        }

        friend const_iterator operator+(const_iterator lhs, difference_type rhs) noexcept
        {
            return lhs += rhs;
        }

        friend const_iterator operator+(difference_type lhs, const_iterator rhs) noexcept
        {
            return rhs += lhs;
        }

        friend difference_type operator-(const_iterator lhs, const_iterator rhs) noexcept
        {
            assert(lhs.get_container() == rhs.get_container());
            return static_cast<difference_type>(lhs.get_offset()) -
                   static_cast<difference_type>(rhs.get_offset());
        }

        friend const_iterator operator-(const_iterator lhs, difference_type rhs) noexcept
        {
            return lhs -= rhs;
        }

        [[nodiscard]]
        reference operator[](difference_type off) const noexcept
        {
            const_iterator tmp = *this + off;
            return *tmp;
        }

        [[nodiscard]]
        const small_vector* get_container() const noexcept
        {
            return m_p_vec;
        }

        explicit operator bool() const noexcept
        {
            return m_p_vec != nullptr;
        }

    private:
        const small_vector* m_p_vec = nullptr;
    };

    const_iterator cbegin() const noexcept
    {
        return const_iterator(this, 0);
    }

    const_iterator cend() const noexcept
    {
        return const_iterator(this, size());
    }

    const_iterator begin() const noexcept
    {
        return cbegin();
    }

    const_iterator end() const noexcept
    {
        return cend();
    }

    void insert(size_type where, const void* ref)
    {
        return visit_impl(
            [where, ref](auto& impl)
            { return impl.insert_one(where, ref); }
        );
    }

    void insert(const_iterator where, const void* ref)
    {
        assert(this == where.get_container());
        this->insert(where.get_offset(), ref);
    }

    void erase(size_type where, size_type count)
    {
        return visit_impl(
            [where, count](auto& impl)
            { return impl.erase_n(where, count); }
        );
    }

    void erase(size_type where)
    {
        this->erase(where, 1);
    }

    void erase(const_iterator start, const_iterator stop)
    {
        assert(this == start.get_container());
        assert(this == stop.get_container());
        difference_type diff = stop - start;
        if(diff < 0) [[unlikely]]
            return;
        this->erase(start.get_offset(), static_cast<size_type>(diff));
    }

    void erase(const_iterator where)
    {
        assert(this == where.get_container());
        this->erase(where.get_offset(), 1);
    }

    void* data_at(size_type idx) noexcept
    {
        return visit_impl(
            [idx](auto& impl)
            { return impl.data_at(idx); }
        );
    }

    const void* data_at(size_type idx) const noexcept
    {
        return visit_impl(
            [idx](auto& impl)
            { return impl.data_at(idx); }
        );
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis, size_type start, size_type count)
    {
        if(start >= size())
            throw_out_of_range();
        count = std::min(size() - count, count);
        visit_script_type(
            std::forward<Visitor>(vis),
            element_type_id(),
            data_at(start),
            data_at(start + count)
        );
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis, const_iterator start, const_iterator stop)
    {
        assert(this == start.get_container());
        assert(this == stop.get_container());

        visit_script_type(
            std::forward<Visitor>(vis),
            element_type_id(),
            data_at(start.get_offset()),
            data_at(stop.get_offset())
        );
    }
};
} // namespace asbind20::container

#endif
