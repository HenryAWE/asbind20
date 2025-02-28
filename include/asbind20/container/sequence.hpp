#ifndef ASBIND20_CONTAINER_SEQUENCE_HPP
#define ASBIND20_CONTAINER_SEQUENCE_HPP

#pragma once

#include <vector>
#include <deque>
#include "../detail/include_as.hpp"
#include "../utility.hpp"
#include "traits.hpp"

namespace asbind20::container
{
class container_base
{
public:
    class handle_proxy
    {
    public:
        using handle_type = void*;

        handle_proxy() noexcept = default;
        handle_proxy(const handle_proxy&) = delete;

        // for consistency with object proxy
        handle_proxy(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
            : m_handle(nullptr)
        {
            (void)ti;
        }

        handle_proxy(handle_proxy&& other) noexcept
            : m_handle(std::exchange(other.m_handle, nullptr))
        {}

        handle_proxy(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, handle_type handle)
            : m_handle(nullptr)
        {
            if(!ti) [[unlikely]]
                return;
            if(handle)
            {
                ti->GetEngine()->AddRefScriptObject(handle, ti);
                m_handle = handle;
            }
        }

        ~handle_proxy()
        {
            assert(m_handle == nullptr);
        }

        handle_proxy& operator=(handle_proxy&& rhs) noexcept
        {
            if(this == &rhs)
                return *this;

            // m_handle should be empty to avoid potential memory leak
            assert(this->m_handle == nullptr);
            m_handle = std::exchange(rhs.m_handle, nullptr);

            return *this;
        }

        void assign(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, handle_type handle)
        {
            if(!ti) [[unlikely]]
                return;

            AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = ti->GetEngine();
            if(m_handle)
                engine->ReleaseScriptObject(m_handle, ti);
            m_handle = handle;
            if(handle)
                engine->AddRefScriptObject(handle, ti);
        }

        void destroy(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            if(!ti) [[unlikely]]
                return;
            if(!m_handle)
                return;

            ti->GetEngine()->ReleaseScriptObject(m_handle, ti);
            m_handle = nullptr;
        }

        void* data_address()
        {
            return &m_handle;
        }

        const void* data_address() const
        {
            return &m_handle;
        }

        void* object_ref() const noexcept
        {
            return m_handle;
        }

    private:
        handle_type m_handle = nullptr;
    };

    class object_proxy
    {
    public:
        object_proxy() noexcept = default;
        object_proxy(const object_proxy&) = delete;

        object_proxy(object_proxy&& other) noexcept
            : m_ptr(std::exchange(other.m_ptr, nullptr)) {}

        object_proxy(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
            : m_ptr(nullptr)
        {
            if(!ti) [[unlikely]]
                return;

            m_ptr = ti->GetEngine()->CreateScriptObject(ti);
        }

        object_proxy(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, void* ptr)
            : m_ptr(nullptr)
        {
            if(!ti) [[unlikely]]
                return;
            if(!ptr) [[unlikely]]
                return;

            m_ptr = ti->GetEngine()->CreateScriptObjectCopy(ptr, ti);
        }

        ~object_proxy()
        {
            assert(m_ptr == nullptr);
        }

        object_proxy& operator=(object_proxy&& rhs) noexcept
        {
            if(this == &rhs)
                return *this;

            // m_ptr should be empty to avoid potential memory leak
            assert(this->m_ptr == nullptr);
            m_ptr = std::exchange(rhs.m_ptr, nullptr);

            return *this;
        }

        void assign(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, void* ptr)
        {
            if(!ti) [[unlikely]]
                return;

            AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = ti->GetEngine();
            if(m_ptr)
                engine->ReleaseScriptObject(m_ptr, ti);
            m_ptr = ptr;
            if(ptr)
                engine->CreateScriptObjectCopy(ptr, ti);
        }

        void destroy(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            if(!ti) [[unlikely]]
                return;
            if(!m_ptr)
                return;

            ti->GetEngine()->ReleaseScriptObject(m_ptr, ti);
            m_ptr = nullptr;
        }

        void* data_address()
        {
            return m_ptr;
        }

        const void* data_address() const
        {
            return m_ptr;
        }

        void* object_ref() const noexcept
        {
            return m_ptr;
        }

    private:
        void* m_ptr = nullptr;
    };

    template <bool IsHandle>
    using proxy_type = std::conditional_t<
        IsHandle,
        handle_proxy,
        object_proxy>;

protected:
    template <typename Container, typename Proxy>
    struct container_proxy
    {
        Container c;

        template <typename... Args>
        container_proxy(Args&&... args) noexcept(std::is_nothrow_constructible_v<Container, Args...>)
            : c(std::forward<Args>(args)...)
        {}

        template <typename... Args>
        void emplace_back(Args&&... args)
        {
            if constexpr(requires() { Proxy::emplace_back(c, std::forward<Args>(args)...); })
            {
                Proxy::emplace_back(c, std::forward<Args>(args)...);
            }
            else
            {
                c.emplace_back(std::forward<Args>(args)...);
            }
        }

        template <typename... Args>
        void emplace_front(Args&&... args)
        {
            if constexpr(requires() { Proxy::emplace_front(c, std::forward<Args>(args)...); })
            {
                Proxy::emplace_front(c, std::forward<Args>(args)...);
            }
            else
            {
                c.emplace_front(std::forward<Args>(args)...);
            }
        }

        constexpr std::size_t size() const noexcept
        {
            return c.size();
        }

        auto iterator_at(std::size_t idx)
        {
            using std::end;
            if(idx >= size()) [[unlikely]]
                return end(c);

            using std::begin;
            return std::next(begin(c), idx);
        }

        auto iterator_at(std::size_t idx) const
        {
            using std::end;
            if(idx >= size()) [[unlikely]]
                return end(c);

            using std::begin;
            return std::next(begin(c), idx);
        }
    };
};

template <>
struct container_traits<std::vector>
{
    using sequence_container_tag = void;

    template <typename T, typename Allocator>
    using container_type = std::vector<T, Allocator>;

    template <typename C>
    struct proxy
    {
        template <typename... Args>
        static void emplace_front(C& c, Args&&... args)
        {
            c.emplace(c.begin(), std::forward<Args>(args)...);
        }
    };
};

template <>
struct container_traits<std::deque>
{
    using sequence_container_tag = void;

    template <typename T, typename Allocator>
    using container_type = std::deque<T, Allocator>;
};

namespace detail
{
    template <typename C, template <typename...> typename T>
    struct get_proxy
    {
        using type = void;
    };

    template <typename C, template <typename...> typename T>
    requires(requires() { typename container_traits<T>::template proxy<C>; })
    struct get_proxy<C, T>
    {
        using type = typename container_traits<T>::template proxy<C>;
    };

    // Deal with the std::vector<bool>

    template <template <typename...> typename T>
    struct is_std_vector : public std::false_type
    {};

    template <>
    struct is_std_vector<std::vector> : public std::true_type
    {};
} // namespace detail

template <template <typename...> typename Container, typename Alloc = as_allocator<void>>
requires(requires() { typename container_traits<Container>::sequence_container_tag; })
class sequence final : public container_base
{
public:
    using size_type = std::size_t;

    sequence() = delete;

    sequence(const sequence&) = delete;

    sequence(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        int elem_type_id
    )
    {
        setup_impl(engine, elem_type_id);
    }

    ~sequence()
    {
        impl().~seq_interface();
    }

    auto get_engine() const noexcept
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return impl().get_engine();
    }

    int element_type_id() const
    {
        return impl().element_type_id();
    }

    auto element_type_info() const
        -> AS_NAMESPACE_QUALIFIER asITypeInfo*
    {
        return impl().element_type_info();
    }

    size_type size() const noexcept
    {
        return impl().size();
    }

    void push_front(const void* ref)
    {
        impl().push_front(ref);
    }

    void push_back(const void* ref)
    {
        impl().push_back(ref);
    }

    void* address_at(size_type idx)
    {
        return impl().address_at(idx);
    }

    const void* address_at(size_type idx) const
    {
        return impl().address_at(idx);
    }

private:
    class seq_interface
    {
    public:
        seq_interface() = default;

        virtual ~seq_interface() = default;

        virtual int element_type_id() const noexcept = 0;

        virtual auto element_type_info() const
            -> AS_NAMESPACE_QUALIFIER asITypeInfo*
        {
            return get_engine()->GetTypeInfoById(element_type_id());
        }

        virtual auto get_engine() const
            -> AS_NAMESPACE_QUALIFIER asIScriptEngine* = 0;

        virtual size_type size() const noexcept = 0;

        virtual void push_back(const void* ref) = 0;
        virtual void push_front(const void* ref) = 0;

        virtual void* address_at(size_type idx) const = 0;
    };

    template <int TypeId>
    requires(is_primitive_type(TypeId))
    class seq_primitive : public seq_interface
    {
        // Deal with the vector<bool>
        static constexpr bool vector_bool_workaround =
            TypeId == AS_NAMESPACE_QUALIFIER asTYPEID_BOOL &&
            detail::is_std_vector<Container>::value;

    public:
        using value_type = primitive_type_of_t<TypeId>;
        using allocator_type = typename std::allocator_traits<Alloc>::template rebind_alloc<value_type>;
        using traits_type = container_traits<Container>;
        using container_type = std::conditional_t<
            vector_bool_workaround,
            std::vector<std::uint8_t, typename std::allocator_traits<Alloc>::template rebind_alloc<std::uint8_t>>,
            typename traits_type::template container_type<value_type, allocator_type>>;

        seq_primitive(AS_NAMESPACE_QUALIFIER asIScriptEngine* e)
            : m_engine(e) {}

        auto get_engine() const
            -> AS_NAMESPACE_QUALIFIER asIScriptEngine* final
        {
            return m_engine;
        }

        int element_type_id() const noexcept override
        {
            return TypeId;
        }

        size_type size() const noexcept final
        {
            return m_container.size();
        }

        void push_back(const void* ref) override
        {
            m_container.emplace_back(ref_to_elem_val(ref));
        }

        void push_front(const void* ref) override
        {
            m_container.emplace_front(ref_to_elem_val(ref));
        }

        void* address_at(size_type idx) const final
        {
            auto it = m_container.iterator_at(idx);
            using std::end;
            if(it == end(m_container.c))
                return nullptr;
            return (void*)std::addressof(*it);
        }

    private:
        AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine;

        using container_proxy_type = container_proxy<
            container_type,
            typename detail::get_proxy<container_type, Container>::type>;

        container_proxy_type m_container;

        static auto ref_to_elem_val(const void* ref) noexcept
        {
            if constexpr(vector_bool_workaround)
                return *static_cast<const std::uint8_t*>(ref);
            else
                return *static_cast<const value_type*>(ref);
        }
    };

    class seq_enum final : public seq_primitive<AS_NAMESPACE_QUALIFIER asTYPEID_INT32>
    {
        using my_base = seq_primitive<AS_NAMESPACE_QUALIFIER asTYPEID_INT32>;

    public:
        seq_enum(AS_NAMESPACE_QUALIFIER asIScriptEngine* e, int elem_type_id)
            : my_base(e), m_type_id(elem_type_id) {}

        int element_type_id() const noexcept override
        {
            return m_type_id;
        }

    private:
        int m_type_id;
    };

    template <bool IsHandle>
    class seq_object final : public seq_interface
    {
    public:
        using value_type = typename container_base::proxy_type<IsHandle>;

        template <typename T>
        class object_allocator :
            public std::allocator_traits<Alloc>::template rebind_alloc<T>
        {
            using my_base = typename std::allocator_traits<Alloc>::template rebind_alloc<T>;

        public:
            using pointer = T*;
            using value_type = T;
            using proxy_type = typename seq_object::value_type;

            object_allocator(AS_NAMESPACE_QUALIFIER asITypeInfo* ti) noexcept(std::is_nothrow_default_constructible_v<my_base>)
                : my_base(), m_ti(ti)
            {}

            template <typename U>
            object_allocator(const object_allocator<U>& other) noexcept(std::is_nothrow_copy_assignable_v<my_base>)
                : my_base(other), m_ti(other.get_type_info())
            {}

            auto get_type_info() const
                -> AS_NAMESPACE_QUALIFIER asITypeInfo*
            {
                return m_ti;
            }

            template <typename... Args>
            requires(is_only_constructible_v<T, AS_NAMESPACE_QUALIFIER asITypeInfo*, Args...>)
            void construct(pointer mem, Args&&... args)
            {
                new(mem) T(get_type_info(), std::forward<Args>(args)...);
            }

            void construct(pointer mem, proxy_type&& val) noexcept
            {
                new(mem) T(std::move(val));
            }

            void destroy(proxy_type* mem) noexcept
            {
                if(!mem) [[unlikely]]
                    return;

                mem->destroy(get_type_info());
                mem->~proxy_type();
            }

        private:
            AS_NAMESPACE_QUALIFIER asITypeInfo* m_ti;
        };

        using traits_type = container_traits<Container>;
        using allocator_type = object_allocator<value_type>;
        using container_type = typename traits_type::template container_type<value_type, allocator_type>;

        seq_object(AS_NAMESPACE_QUALIFIER asIScriptEngine* e, int elem_type_id)
            : m_type_id(elem_type_id), m_container(allocator_type(e->GetTypeInfoById(elem_type_id)))
        {
            element_type_info()->AddRef();
        }

        ~seq_object()
        {
            element_type_info()->Release();
        }

        auto get_engine() const
            -> AS_NAMESPACE_QUALIFIER asIScriptEngine* final
        {
            return element_type_info()->GetEngine();
        }

        int element_type_id() const noexcept override
        {
            return m_type_id;
        }

        virtual auto element_type_info() const
            -> AS_NAMESPACE_QUALIFIER asITypeInfo* final
        {
            return m_container.c.get_allocator().get_type_info();
        }

        size_type size() const noexcept final
        {
            return m_container.size();
        }

        void push_back(const void* ref) override
        {
            m_container.emplace_back(ref_to_ptr(ref));
        }

        void push_front(const void* ref) override
        {
            m_container.emplace_front(ref_to_ptr(ref));
        }

        void* address_at(size_type idx) const final
        {
            auto it = m_container.iterator_at(idx);
            using std::end;
            if(it == end(m_container.c))
                return nullptr;
            return (void*)it->data_address();
        }

    private:
        int m_type_id;

        using container_proxy_type = container_proxy<
            container_type,
            typename detail::get_proxy<container_type, Container>::type>;

        container_proxy_type m_container;

        static void* ref_to_ptr(const void* ref)
        {
            if constexpr(IsHandle)
                return *static_cast<void* const*>(ref);
            else
                return const_cast<void*>(ref);
        }
    };

    static constexpr std::size_t storage_size = std::max(
        sizeof(seq_enum),
        sizeof(seq_object<false>)
    );

    alignas(std::max_align_t) std::byte m_data[storage_size];

    template <typename ImplType, typename... Args>
    void construct_impl(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, Args&&... args)
    {
        static_assert(storage_size >= sizeof(ImplType));
        new(m_data) ImplType(engine, std::forward<Args>(args)...);
    }

    void setup_impl(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int elem_type_id)
    {
        assert(!is_void(elem_type_id));

        if(is_primitive_type(elem_type_id))
        {
            switch(elem_type_id)
            {
#define ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(as_type_id)                     \
case AS_NAMESPACE_QUALIFIER as_type_id:                                       \
    construct_impl<seq_primitive<AS_NAMESPACE_QUALIFIER as_type_id>>(engine); \
    break

                ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(asTYPEID_BOOL);
                ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(asTYPEID_INT8);
                ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(asTYPEID_INT16);
                ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(asTYPEID_INT32);
                ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(asTYPEID_INT64);
                ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(asTYPEID_UINT8);
                ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(asTYPEID_UINT16);
                ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(asTYPEID_UINT32);
                ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(asTYPEID_UINT64);
                ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(asTYPEID_FLOAT);
                ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(asTYPEID_DOUBLE);

#undef ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE

            default: // enum
                assert(is_enum_type(elem_type_id));
                construct_impl<seq_enum>(engine, elem_type_id);
                break;
            }
        }
        else
        {
            if(is_objhandle(elem_type_id))
                construct_impl<seq_object<true>>(engine, elem_type_id);
            else
                construct_impl<seq_object<false>>(engine, elem_type_id);
        }
    }

    seq_interface& impl() noexcept
    {
        return *reinterpret_cast<seq_interface*>(m_data);
    }

    const seq_interface& impl() const noexcept
    {
        return *reinterpret_cast<const seq_interface*>(m_data);
    }
};
} // namespace asbind20::container

#endif
