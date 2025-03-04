/**
 * @file sequence.hpp
 * @author HenryAWE
 * @brief Sequence containers for AngelScript
 *
 * @warning THIS MODULE IS CURRENTLY EXPERIMENTAL!
 * Interfaces will change rapidly in future versions. Use it at your own risk.
 */

#ifndef ASBIND20_CONTAINER_SEQUENCE_HPP
#define ASBIND20_CONTAINER_SEQUENCE_HPP

#pragma once

#include <vector>
#include <deque>
#include "../detail/include_as.hpp"
#include "../utility.hpp"

#if defined(__GNUC__) && !defined(__clang__)
#    if __GNUC__ <= 12
#        error "This header requires at least GCC 13"
#    endif
#endif

namespace asbind20::container
{
template <template <typename...> typename Container>
struct sequence_container_traits
{};

template <>
struct sequence_container_traits<std::vector>
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

        template <typename... Args>
        static void pop_front(C& c)
        {
            c.erase(c.begin());
        }
    };
};

template <>
struct sequence_container_traits<std::deque>
{
    using sequence_container_tag = void;

    template <typename T, typename Allocator>
    using container_type = std::deque<T, Allocator>;
};

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

        handle_proxy(std::in_place_t, handle_type handle) noexcept
            : m_handle(handle)
        {}

        ~handle_proxy()
        {
            assert(m_handle == nullptr);
        }

        handle_proxy& operator=(handle_proxy&& rhs) noexcept
        {
            if(this == &rhs)
                return *this;

            // Swap the pointer
            // Current pointer will be released by rhs
            std::swap(m_handle, rhs.m_handle);

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

        object_proxy(std::in_place_t, void* ptr) noexcept
            : m_ptr(ptr)
        {}

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

            // Swap the pointer
            // Current pointer will be released by rhs
            std::swap(m_ptr, rhs.m_ptr);

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

        template <typename... Args>
        void pop_back(Args&&... args)
        {
            if constexpr(requires() { Proxy::pop_back(c, std::forward<Args>(args)...); })
            {
                Proxy::pop_back(c, std::forward<Args>(args)...);
            }
            else
            {
                c.pop_back(std::forward<Args>(args)...);
            }
        }

        template <typename... Args>
        void pop_front(Args&&... args)
        {
            if constexpr(requires() { Proxy::pop_front(c, std::forward<Args>(args)...); })
            {
                Proxy::pop_front(c, std::forward<Args>(args)...);
            }
            else
            {
                c.pop_front(std::forward<Args>(args)...);
            }
        }

        constexpr std::size_t size() const noexcept
        {
            return c.size();
        }

        constexpr std::size_t capacity() const noexcept
        {
            if constexpr(requires() { c.capacity(); })
                return c.capacity();
            else
                return size();
        }

        void reserve(std::size_t new_cap)
        {
            if constexpr(requires() { c.reserve(new_cap); })
            {
                c.reserve(new_cap);
            }
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

namespace detail
{
    template <typename C, template <typename...> typename T>
    struct get_proxy
    {
        using type = void;
    };

    template <typename C, template <typename...> typename T>
    requires(requires() { typename sequence_container_traits<T>::template proxy<C>; })
    struct get_proxy<C, T>
    {
        using type = typename sequence_container_traits<T>::template proxy<C>;
    };

    // Deal with the std::vector<bool>

    template <template <typename...> typename T>
    struct is_std_vector : public std::false_type
    {};

    template <>
    struct is_std_vector<std::vector> : public std::true_type
    {};
} // namespace detail

/**
 * @brief Sequence container of script object
 *
 * @tparam Container Underlying supported standard sequence container.
 *                   Currently available options are: `std::vector` and `std::deque`.
 * @tparam Alloc Allocator that can be rebound to primitive and internal proxy types.
 */
template <template <typename...> typename Container, typename Alloc = as_allocator<void>>
requires(requires() { typename sequence_container_traits<Container>::sequence_container_tag; })
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

    sequence(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        int elem_type_id,
        script_init_list_repeat ilist
    )
    {
        setup_impl(engine, elem_type_id, ilist);
    }

    ~sequence()
    {
        impl().~seq_interface();
    }

    [[nodiscard]]
    auto get_engine() const noexcept
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return impl().get_engine();
    }

    [[nodiscard]]
    int element_type_id() const
    {
        return impl().element_type_id();
    }

    [[nodiscard]]
    auto element_type_info() const
        -> AS_NAMESPACE_QUALIFIER asITypeInfo*
    {
        return impl().element_type_info();
    }

    [[nodiscard]]
    size_type size() const noexcept
    {
        return impl().size();
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
        return impl().empty();
    }

    void push_front(const void* ref)
    {
        impl().push_front(ref);
    }

    void push_back(const void* ref)
    {
        impl().push_back(ref);
    }

    void emplace_front()
    {
        impl().emplace_front();
    }

    void emplace_back()
    {
        impl().emplace_back();
    }

    void pop_front()
    {
        impl().pop_front();
    }

    void pop_back()
    {
        impl().pop_back();
    }

    void clear()
    {
        impl().clear();
    }

    void enum_refs()
    {
        impl().enum_refs();
    }

    [[nodiscard]]
    void* address_at(size_type idx)
    {
        return impl().address_at(idx);
    }

    [[nodiscard]]
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
        virtual size_type capacity() const noexcept = 0;
        virtual void reserve(size_type new_cap) = 0;

        virtual void clear() noexcept = 0;

        virtual void enum_refs() = 0;

        virtual void push_back(const void* ref) = 0;
        virtual void push_front(const void* ref) = 0;
        virtual void pop_back() = 0;
        virtual void pop_front() = 0;

        virtual void emplace_front() = 0;
        virtual void emplace_back() = 0;

        virtual void* address_at(size_type idx) const = 0;

        bool empty() const
        {
            return size() == 0;
        }

        class seq_interface_iter
        {
        public:
            virtual ~seq_interface_iter() = default;

            virtual void* object_ref() const = 0;

            virtual void inc() = 0;
            virtual void dec() = 0;

            virtual bool equal_to(const seq_interface_iter& other) const noexcept = 0;

            virtual bool is_sentinel() const noexcept
            {
                return false;
            }

            virtual void copy_to(void* mem) const = 0;
        };

        class seq_iter_sentinel : public seq_interface_iter
        {
        public:
            void* object_ref() const override
            {
                return nullptr;
            }

            void inc() override {}

            void dec() override {}

            bool equal_to(const seq_interface_iter& other) const noexcept override
            {
                return dynamic_cast<const seq_iter_sentinel*>(&other) != nullptr;
            }

            bool is_sentinel() const noexcept override
            {
                return true;
            }

            void copy_to(void* mem) const override
            {
                new(mem) seq_iter_sentinel();
            }
        };

        virtual void new_proxy_begin(void* mem) const = 0;
        virtual void new_proxy_end(void* mem) const = 0;

        virtual void erase_iter(void* new_pos_mem, const seq_interface_iter& it) = 0;

        virtual void insert_iter(void* new_pos_mem, const seq_interface_iter& it, const void* ref) = 0;
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
        using traits_type = sequence_container_traits<Container>;
        using container_type = std::conditional_t<
            vector_bool_workaround,
            std::vector<std::uint8_t, typename std::allocator_traits<Alloc>::template rebind_alloc<std::uint8_t>>,
            typename traits_type::template container_type<value_type, allocator_type>>;

        seq_primitive(AS_NAMESPACE_QUALIFIER asIScriptEngine* e)
            : m_engine(e) {}

        seq_primitive(AS_NAMESPACE_QUALIFIER asIScriptEngine* e, script_init_list_repeat ilist)
            : m_engine(e)
        {
            value_type* start = static_cast<value_type*>(ilist.data());
            value_type* stop = start + ilist.size();

            m_container.c.insert(
                m_container.c.end(),
                start,
                stop
            );
        }

        ~seq_primitive()
        {
            clear();
        }

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

        size_type capacity() const noexcept final
        {
            return m_container.capacity();
        }

        void reserve(size_type new_cap) noexcept final
        {
            return m_container.reserve(new_cap);
        }

        void clear() noexcept final
        {
            return m_container.c.clear();
        }

        void enum_refs() final {}

        void push_back(const void* ref) override
        {
            m_container.emplace_back(ref_to_elem_val(ref));
        }

        void push_front(const void* ref) override
        {
            m_container.emplace_front(ref_to_elem_val(ref));
        }

        void emplace_front() override
        {
            m_container.emplace_front();
        }

        void emplace_back() override
        {
            m_container.emplace_back();
        }

        void pop_back() override
        {
            if(!this->empty())
                m_container.pop_back();
        }

        void pop_front() override
        {
            if(!this->empty())
                m_container.pop_front();
        }

        void* address_at(size_type idx) const final
        {
            auto it = m_container.iterator_at(idx);
            using std::end;
            if(it == end(m_container.c))
                return nullptr;
            return (void*)std::addressof(*it);
        }

        class seq_primitive_iter final : public seq_interface::seq_interface_iter
        {
        public:
            using underlying_type = typename container_type::iterator;

            template <typename... Args>
            seq_primitive_iter(Args&&... args) noexcept
                : underlying(std::forward<Args>(args)...)
            {}

            void* object_ref() const override
            {
                return std::to_address(underlying);
            }

            void inc() override
            {
                ++underlying;
            }

            void dec() override
            {
                --underlying;
            }

            bool equal_to(const seq_interface::seq_interface_iter& other) const noexcept override
            {
                if(other.is_sentinel())
                    return false;

                assert(dynamic_cast<const seq_primitive_iter*>(&other));
                const auto& same_t_other = static_cast<const seq_primitive_iter&>(other);
                return underlying == same_t_other.underlying;
            }

            void copy_to(void* mem) const override
            {
                new(mem) seq_primitive_iter(underlying);
            }

            underlying_type underlying;
        };

        using iterator_proxy = seq_primitive_iter;

        void new_proxy_begin(void* mem) const override
        {
            new(mem) iterator_proxy(
                const_cast<seq_primitive&>(*this).m_container.c.begin()
            );
        }

        void new_proxy_end(void* mem) const override
        {
            new(mem) iterator_proxy(
                const_cast<seq_primitive&>(*this).m_container.c.end()
            );
        }

        void erase_iter(void* new_pos_mem, const seq_interface::seq_interface_iter& it) override
        {
            if(this->empty() || it.is_sentinel()) [[unlikely]]
            {
                new_proxy_end(new_pos_mem);
                return;
            }

            assert(dynamic_cast<const iterator_proxy*>(&it));
            auto& same_t_it = static_cast<const iterator_proxy&>(it);
            new(new_pos_mem) iterator_proxy(
                m_container.c.erase(same_t_it.underlying)
            );
        }

        void insert_iter(void* new_pos_mem, const seq_interface::seq_interface_iter& it, const void* ref) override
        {
            auto do_insert = [&](typename iterator_proxy::underlying_type pos)
            {
                new(new_pos_mem) iterator_proxy(
                    m_container.c.emplace(pos, ref_to_elem_val(ref))
                );
            };

            if(it.is_sentinel() || this->empty())
                do_insert(m_container.c.end());
            else
            {
                assert(dynamic_cast<const iterator_proxy*>(&it));
                auto& same_t_it = static_cast<const iterator_proxy&>(it);
                do_insert(same_t_it.underlying);
            }
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

    // enums are stored as 32-bit integer
    class seq_enum final : public seq_primitive<AS_NAMESPACE_QUALIFIER asTYPEID_INT32>
    {
        using my_base = seq_primitive<AS_NAMESPACE_QUALIFIER asTYPEID_INT32>;

    public:
        seq_enum(AS_NAMESPACE_QUALIFIER asIScriptEngine* e, int elem_type_id)
            : my_base(e), m_type_id(elem_type_id) {}

        seq_enum(AS_NAMESPACE_QUALIFIER asIScriptEngine* e, int elem_type_id, script_init_list_repeat ilist)
            : my_base(e, ilist), m_type_id(elem_type_id) {}

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

        // Implementation Details for Future Maintenance:
        //
        // The custom allocator (`object_allocator`) will emulate an intrusive container.
        // It stores an `asITypeInfo*` as its member and will pass it to element for construction/copying/destruction if necessary.
        //
        // The move assignment of proxy type (aka the `value_type` member alias of `seq_object`) will actually SWAP the stored object with incoming proxy.
        // This can make sure its stored object can be correctly released by the customized allocator.
        // See the above code containing definition of proxy type for details.
        //
        // The reference counter of `asITypeInfo` will be increased in the constructor of `seq_object` and released in the destructor.

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

            void construct(pointer mem, std::in_place_t, void* ptr) noexcept
            {
                new(mem) T(std::in_place, ptr);
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

        using traits_type = sequence_container_traits<Container>;
        using allocator_type = object_allocator<value_type>;
        using container_type = typename traits_type::template container_type<value_type, allocator_type>;

        seq_object(AS_NAMESPACE_QUALIFIER asIScriptEngine* e, int elem_type_id)
            : m_type_id(elem_type_id), m_container(allocator_type(e->GetTypeInfoById(elem_type_id)))
        {
            element_type_info()->AddRef();
        }

        seq_object(AS_NAMESPACE_QUALIFIER asIScriptEngine* e, int elem_type_id, script_init_list_repeat ilist)
            : m_type_id(elem_type_id), m_container(allocator_type(e->GetTypeInfoById(elem_type_id)))
        {
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti = element_type_info();
            ti->AddRef();

            reserve(ilist.size());

            auto flags = ti->GetFlags();
            if(is_objhandle(elem_type_id) || (flags & AS_NAMESPACE_QUALIFIER asOBJ_REF))
            {
                for(std::size_t i = 0; i < ilist.size(); ++i)
                {
                    void* ptr = ((void**)ilist.data())[i];
                    m_container.emplace_back(std::in_place, ptr);
                }

                // Set the original list to 0, preventing it from being double freed.
                std::memset(ilist.data(), 0, sizeof(void*) * ilist.size());
            }
            else
            {
                std::size_t elem_size = ti->GetSize();
                for(std::size_t i = 0; i < ilist.size(); ++i)
                {
                    void* ptr = (std::byte*)ilist.data() + elem_size * i;
                    m_container.emplace_back(ptr);
                }
            }
        }

        ~seq_object()
        {
            clear();
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

        size_type capacity() const noexcept final
        {
            return m_container.capacity();
        }

        void reserve(size_type new_cap) noexcept final
        {
            return m_container.reserve(new_cap);
        }

        void clear() noexcept final
        {
            return m_container.c.clear();
        }

        void enum_refs() final
        {
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti = element_type_info();
            auto* engine = ti->GetEngine();

            auto flags = ti->GetFlags();
            if(flags & AS_NAMESPACE_QUALIFIER asOBJ_REF)
            {
                for(auto it = m_container.c.begin(); it != m_container.c.end(); ++it)
                {
                    void* ref = it->object_ref();
                    if(!ref)
                        continue;
                    engine->GCEnumCallback(ref);
                }
            }
            else if((flags & AS_NAMESPACE_QUALIFIER asOBJ_VALUE) && (flags & AS_NAMESPACE_QUALIFIER asOBJ_GC))
            {
                for(auto it = m_container.c.begin(); it != m_container.c.end(); ++it)
                {
                    void* ref = it->object_ref();
                    if(!ref)
                        continue;
                    engine->ForwardGCEnumReferences(ref, ti);
                }
            }
        }

        void push_back(const void* ref) override
        {
            m_container.emplace_back(ref_to_ptr(ref));
        }

        void push_front(const void* ref) override
        {
            m_container.emplace_front(ref_to_ptr(ref));
        }

        void pop_back() override
        {
            if(!this->empty())
                m_container.pop_back();
        }

        void pop_front() override
        {
            if(!this->empty())
                m_container.pop_front();
        }

        void emplace_front() override
        {
            m_container.emplace_front();
        }

        void emplace_back() override
        {
            m_container.emplace_back();
        }

        void* address_at(size_type idx) const final
        {
            auto it = m_container.iterator_at(idx);
            using std::end;
            if(it == end(m_container.c))
                return nullptr;
            return (void*)it->data_address();
        }

        class seq_object_iter final : public seq_interface::seq_interface_iter
        {
        public:
            using underlying_type = typename container_type::iterator;

            template <typename... Args>
            seq_object_iter(Args&&... args) noexcept
                : underlying(std::forward<Args>(args)...)
            {}

            void* object_ref() const override
            {
                return underlying->object_ref();
            }

            void inc() override
            {
                ++underlying;
            }

            void dec() override
            {
                --underlying;
            }

            bool equal_to(const seq_interface::seq_interface_iter& other) const noexcept override
            {
                if(other.is_sentinel())
                    return false;

                assert(dynamic_cast<const seq_object_iter*>(&other));
                const auto& same_t_other = static_cast<const seq_object_iter&>(other);
                return underlying == same_t_other.underlying;
            }

            void copy_to(void* mem) const override
            {
                new(mem) seq_object_iter(underlying);
            }

            underlying_type underlying = nullptr;
        };

        using iterator_proxy = seq_object_iter;

        void new_proxy_begin(void* mem) const override
        {
            new(mem) iterator_proxy(
                const_cast<seq_object&>(*this).m_container.c.begin()
            );
        }

        void new_proxy_end(void* mem) const override
        {
            new(mem) iterator_proxy(
                const_cast<seq_object&>(*this).m_container.c.end()
            );
        }

        void erase_iter(void* new_pos_mem, const seq_interface::seq_interface_iter& it) override
        {
            if(this->empty() || it.is_sentinel()) [[unlikely]]
            {
                new_proxy_end(new_pos_mem);
                return;
            }

            assert(dynamic_cast<const iterator_proxy*>(&it));
            auto& same_t_it = static_cast<const iterator_proxy&>(it);
            new(new_pos_mem) iterator_proxy(
                m_container.c.erase(same_t_it.underlying)
            );
        }

        void insert_iter(void* new_pos_mem, const seq_interface::seq_interface_iter& it, const void* ref) override
        {
            auto do_insert = [&](typename iterator_proxy::underlying_type pos)
            {
                AS_NAMESPACE_QUALIFIER asITypeInfo* ti = this->element_type_info();
                value_type tmp(ti, ref_to_ptr(ref));
                try
                {
                    new(new_pos_mem) iterator_proxy(
                        m_container.c.emplace(pos, std::move(tmp))
                    );
                }
                catch(...)
                {
                    tmp.destroy(ti);
                    tmp.~value_type();
                    throw;
                }

                tmp.destroy(ti);
                tmp.~value_type();
            };

            if(it.is_sentinel() || this->empty())
                do_insert(m_container.c.end());
            else
            {
                assert(dynamic_cast<const iterator_proxy*>(&it));
                auto& same_t_it = static_cast<const iterator_proxy&>(it);
                do_insert(same_t_it.underlying);
            }
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
            {
                assert(ref != nullptr);
                return *static_cast<void* const*>(ref);
            }
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

    template <typename... Args>
    void setup_impl(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int elem_type_id, Args&&... args)
    {
        assert(!is_void_type(elem_type_id));

        if(is_primitive_type(elem_type_id))
        {
            switch(elem_type_id)
            {
#define ASBIND20_CONTAINER_SEQ_PRIMITIVE_CASE(as_type_id)             \
case AS_NAMESPACE_QUALIFIER as_type_id:                               \
    construct_impl<seq_primitive<AS_NAMESPACE_QUALIFIER as_type_id>>( \
        engine, std::forward<Args>(args)...                           \
    );                                                                \
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
                construct_impl<seq_enum>(engine, elem_type_id, std::forward<Args>(args)...);
                break;
            }
        }
        else
        {
            if(is_objhandle(elem_type_id))
                construct_impl<seq_object<true>>(engine, elem_type_id, std::forward<Args>(args)...);
            else
                construct_impl<seq_object<false>>(engine, elem_type_id, std::forward<Args>(args)...);
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

    class iterator_base
    {
    protected:
        static constexpr std::size_t iter_storage_size = std::max(
            sizeof(typename seq_enum::iterator_proxy),
            sizeof(typename seq_object<false>::iterator_proxy)
        );

        std::byte iter_data[iter_storage_size];

        iterator_base() noexcept
        {
            new(iter_data) seq_interface::seq_iter_sentinel();
        }

        template <typename Fn>
        iterator_base(std::in_place_t, Fn&& fn)
        {
            fn(static_cast<void*>(iter_data));
        }

        iterator_base(const iterator_base& other)
        {
            other.impl().copy_to(iter_data);
        }

        ~iterator_base()
        {
            using iter_t = seq_interface::seq_interface_iter;
            impl().~iter_t();
        }

        iterator_base& operator=(const iterator_base& rhs)
        {
            if(this == &rhs) [[unlikely]]
                return *this;

            reset_to(
                [&](void* mem)
                {
                    rhs.impl().copy_to(mem);
                }
            );
            return *this;
        }

        bool operator==(const iterator_base& rhs) const
        {
            return impl().equal_to(rhs.impl());
        }

        template <typename Fn>
        void reset_to(Fn&& f)
        {
            using iter_t = seq_interface::seq_interface_iter;
            impl().~iter_t();
            f(static_cast<void*>(iter_data));
        }

        seq_interface::seq_interface_iter& impl() noexcept
        {
            return *reinterpret_cast<seq_interface::seq_interface_iter*>(iter_data);
        }

        const seq_interface::seq_interface_iter& impl() const noexcept
        {
            return *reinterpret_cast<const seq_interface::seq_interface_iter*>(iter_data);
        }

        void inc()
        {
            impl().inc();
        }

        void dec()
        {
            impl().dec();
        }

        void* object_ref() const
        {
            return impl().object_ref();
        }

        bool is_sentinel() const noexcept
        {
            return impl().is_sentinel();
        }
    };

public:
    /**
     * @brief Iterator
     *
     * @note DO NOT expose this to script directly!
     *
     * Many of the operations will invalidate the iterator. Using an invalid iterator will probably crash your application!
     */
    class const_iterator : private iterator_base
    {
        friend sequence;

        template <typename Fn>
        const_iterator(std::in_place_t, Fn&& fn)
            : iterator_base(std::in_place, std::forward<Fn>(fn))
        {}

    public:
        using value_type = void;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;
        using pointer = const void*;
        using reference = const void*;

        const_iterator() = default;
        const_iterator(const const_iterator&) = default;

        const_iterator& operator=(const const_iterator&) = default;

        bool operator==(const const_iterator& rhs) const
        {
            return iterator_base::operator==(rhs);
        }

        const_iterator& operator++()
        {
            this->inc();
            return *this;
        }

        const_iterator& operator--()
        {
            this->dec();
            return *this;
        }

        const_iterator operator++(int)
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        const_iterator operator--(int)
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        const void* operator*() const
        {
            return this->object_ref();
        }

        operator bool() const noexcept
        {
            return !this->is_sentinel();
        }
    };

    const_iterator cbegin() const
    {
        return const_iterator(
            std::in_place,
            [&](void* mem)
            { impl().new_proxy_begin(mem); }
        );
    }

    const_iterator cend() const
    {
        return const_iterator(
            std::in_place,
            [&](void* mem)
            { impl().new_proxy_end(mem); }
        );
    }

    const_iterator begin() const
    {
        return cbegin();
    }

    const_iterator end() const
    {
        return cend();
    }

    const_iterator erase(const_iterator it)
    {
        return const_iterator(
            std::in_place,
            [&](void* mem)
            { impl().erase_iter(mem, it.impl()); }
        );
    }

    const_iterator insert(const_iterator it, const void* ref)
    {
        return const_iterator(
            std::in_place,
            [&](void* mem)
            { impl().insert_iter(mem, it.impl(), ref); }
        );
    }
};

template <typename Allocator = as_allocator<void>>
using vector = sequence<std::vector, Allocator>;

template <typename Allocator = as_allocator<void>>
using deque = sequence<std::deque, Allocator>;
} // namespace asbind20::container

#endif
