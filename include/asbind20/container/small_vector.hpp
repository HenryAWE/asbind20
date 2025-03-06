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
} // namespace detail

template <
    typeinfo_policy TypeInfoPolicy,
    std::size_t StaticCapacityBytes = 4 * sizeof(void*),
    typename Allocator = as_allocator<void>>
requires(StaticCapacityBytes > 0)
class small_vector
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
    class impl_interface
    {
    public:
        impl_interface(AS_NAMESPACE_QUALIFIER asITypeInfo* ti) noexcept
            : m_ti(ti) {}

        virtual ~impl_interface() = default;

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

        virtual int elem_type_id() const
        {
            return TypeInfoPolicy::get_type_id(m_ti);
        }

        auto elem_type_info() const
            -> AS_NAMESPACE_QUALIFIER asITypeInfo*
        {
            return TypeInfoPolicy::get_type_info(m_ti);
        }

        virtual size_type static_capacity() const noexcept = 0;
        virtual size_type capacity() const noexcept = 0;
        virtual size_type size() const noexcept = 0;

        virtual void* value_ref_at(size_type idx) const = 0;

        virtual void reserve(size_type new_cap) = 0;
        virtual void clear() noexcept = 0;

        bool empty() const noexcept
        {
            return size() == 0;
        }

        virtual void push_back(const void* ref) = 0;
        virtual void emplace_back() = 0;
        virtual void pop_back() noexcept = 0;

        // The iterator interface only stores an unsigned offset value,
        // so an invalid iterator from script can be reported by host,
        // instead of crashing the application by accessing invalid memory address.
        class iterator_interface
        {
        public:
            ~iterator_interface() = default;

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

            template <typename T>
            requires(!std::is_void_v<T>)
            T* ptr_to(T* base) const noexcept
            {
                return base + get_offset();
            }

        private:
            size_type m_off = 0;
        };

    private:
        // Although having a type info pointer is useless for a primitive element type,
        // it can keep the interface consistent with other types.
        script_typeinfo m_ti;
    };

    template <int TypeId>
    class impl_primitive_base : public impl_interface
    {
    public:
        using impl_interface::impl_interface;

        int elem_type_id() const noexcept final
        {
            assert(impl_interface::elem_type_id() == TypeId);
            return TypeId;
        }
    };

    // Storage for primitive and enum types
    template <typename ValueType, typename Base>
    class impl_primitive_storage : public Base
    {
    public:
        using value_type = ValueType;
        using pointer = value_type*;
        using const_pointer = const value_type*;

        using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<value_type>;

        template <typename... Args>
        impl_primitive_storage(Args&&... args)
            : Base(std::forward<Args>(args)...)
        {
            m_p_begin = get_static_storage();
            m_p_end = m_p_begin;
            m_p_capacity = m_p_begin + max_static_size();
        }

        ~impl_primitive_storage()
        {
            if(m_p_begin != get_static_storage())
            {
                std::allocator_traits<allocator_type>::deallocate(
                    m_alloc.second(), m_p_begin, size()
                );
            }
        }

        static consteval size_type max_static_size() noexcept
        {
            return StaticCapacityBytes / sizeof(value_type);
        }

        size_type static_capacity() const noexcept final
        {
            return max_static_size();
        }

        size_type capacity() const noexcept final
        {
            return m_p_capacity - m_p_begin;
        }

        size_type size() const noexcept final
        {
            return m_p_end - m_p_begin;
        }

        void* value_ref_at(size_type idx) const override
        {
            if(idx >= size())
                return nullptr;
            return &m_p_begin[idx];
        }

        void reserve(size_type new_cap) final
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
                    m_alloc.second(), m_p_begin, current_size
                );
            }

            m_p_begin = tmp;
            m_p_end = m_p_begin + current_size;
            m_p_capacity = m_p_begin + new_cap;
        }

        void clear() noexcept override
        {
            m_p_end = m_p_begin;
        }

        void push_back(const void* ref) override
        {
            reserve(size() + 1);
            emplace_back_impl(*static_cast<const value_type*>(ref));
        }

        void emplace_back() override
        {
            reserve(size() + 1);
            emplace_back_impl(value_type());
        }

        void pop_back() noexcept override
        {
            if(this->empty())
                return;
            --m_p_end;
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
    using impl_primitive = impl_primitive_storage<primitive_type_of_t<TypeId>, impl_primitive_base<TypeId>>;
    using impl_enum = impl_primitive_storage<int, impl_interface>;

    template <bool IsHandle>
    class impl_object final :
        public impl_primitive_storage<void*, impl_interface>
    {
        using my_base = impl_primitive_storage<void*, impl_interface>;

    public:
        using value_type = void*;

        using my_base::my_base;

        ~impl_object()
        {
            clear();
        }

        void* value_ref_at(size_type idx) const override
        {
            if(idx >= this->size())
                return nullptr;
            if constexpr(IsHandle)
                return &this->m_p_begin[idx];
            else
                return this->m_p_begin[idx];
        }

        void clear() noexcept final
        {
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti = this->elem_type_info();
            assert(ti != nullptr);

            for(value_type* it = this->m_p_begin; it != this->m_p_end; ++it)
            {
                void* obj = *it;

                if(!obj)
                    return;
                ti->GetEngine()->ReleaseScriptObject(obj, ti);
            }

            my_base::clear();
        }

        void push_back(const void* ref) override
        {
            this->reserve(this->size() + 1);
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti = this->elem_type_info();
            assert(ti != nullptr);
            void* obj = ref_to_obj(ref);

            if constexpr(IsHandle)
            {
                if(obj)
                    ti->GetEngine()->AddRefScriptObject(obj, ti);
                *this->m_p_end = obj;
            }
            else
            {
                assert(obj != nullptr);
                *this->m_p_end = ti->GetEngine()->CreateScriptObjectCopy(obj, ti);
            }

            ++this->m_p_end;
        }

        void emplace_back() override
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

        void pop_back() noexcept override
        {
            if(this->empty())
                return;

            AS_NAMESPACE_QUALIFIER asITypeInfo* ti = this->elem_type_info();
            assert(ti != nullptr);

            void* obj = *this->m_p_end;
            if constexpr(IsHandle)
                assert(obj != nullptr);
            if(obj)
                ti->GetEngine()->ReleaseScriptObject(obj, ti);

            --this->m_p_end;
        }

    private:
        static void* ref_to_obj(const void* ref) noexcept
        {
            if constexpr(IsHandle)
                return *static_cast<void* const*>(ref);
            else
                return const_cast<void*>(ref);
        }
    };

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

    template <typename... Args>
    void init_impl(int type_id, AS_NAMESPACE_QUALIFIER asITypeInfo* ti, Args&&... args)
    {
        assert(!is_void_type(type_id));

        if(!is_primitive_type(type_id))
        {
            if(is_objhandle(type_id))
                new(m_impl_data) impl_object<true>(ti, std::forward<Args>(args)...);
            else
                new(m_impl_data) impl_object<false>(ti, std::forward<Args>(args)...);
            return;
        }

        switch(type_id)
        {
#define ASBIND20_SMALL_VECTOR_IMPL_CASE(as_type_id) \
case AS_NAMESPACE_QUALIFIER as_type_id:             \
    new(m_impl_data) impl_primitive<as_type_id>(    \
        ti, std::forward<Args>(args)...             \
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
            new(m_impl_data) impl_enum(ti, std::forward<Args>(args)...);
            break;

#undef ASBIND20_SMALL_VECTOR_IMPL_CASE
        }
    }

public:
    small_vector() = delete;

    ~small_vector()
    {
        impl().~impl_interface();
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
        impl().get_type_info();
    }

    auto element_type_info() const
        -> AS_NAMESPACE_QUALIFIER asITypeInfo*
    {
        TypeInfoPolicy::get_type_info(get_type_info());
    }

    int element_type_id() const
    {
        return impl().elem_type_id();
    }

    size_type static_capacity() const noexcept
    {
        return impl().static_capacity();
    }

    size_type capacity() const noexcept
    {
        return impl().capacity();
    }

    size_type size() const noexcept
    {
        return impl().size();
    }

    void clear() noexcept
    {
        impl().clear();
    }

    bool empty() const noexcept
    {
        return impl().empty();
    }

    void* operator[](size_type idx) noexcept
    {
        return impl().value_ref_at(idx);
    }

    const void* operator[](size_type idx) const noexcept
    {
        return impl().value_ref_at(idx);
    }

    void push_back(const void* ref)
    {
        return impl().push_back(ref);
    }

    void emplace_back()
    {
        return impl().emplace_back();
    }

    void pop_back() noexcept
    {
        return impl().pop_back();
    }
};
} // namespace asbind20::container

#endif
