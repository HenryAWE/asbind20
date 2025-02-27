/**
 * @file container.hpp
 * @author HenryAWE
 * @brief Tools for implementing container of script object
 */

#ifndef ASBIND20_CONTAINER_HPP
#define ASBIND20_CONTAINER_HPP

#include "detail/include_as.hpp"
#include <cstddef>
#include <cassert>
#include <cstring>
#include <ranges>
#include <vector>
#include <deque>
#include "utility.hpp"

namespace asbind20
{
/**
 * @brief Tools for implementing container of script object
 */
namespace container
{
    /**
     * @brief Helper for storing a single script object
     */
    class single
    {
    public:
        single() noexcept
        {
            m_data.ptr = nullptr;
        };

        single(const single&) = delete;

        single(single&& other) noexcept
        {
            *this = std::move(other);
        }

        ~single()
        {
            assert(m_data.ptr == nullptr && "reference not released");
        }

        single& operator=(const single&) = delete;

        single& operator=(single&& other) noexcept
        {
            if(this == &other) [[unlikely]]
                return *this;

            std::memcpy(&m_data, &other.m_data, sizeof(m_data));
            other.m_data.ptr = nullptr;

            return *this;
        }

        void* data_address(int type_id)
        {
            assert(!is_void(type_id));

            if(is_primitive_type(type_id))
                return m_data.primitive;
            else if(is_objhandle(type_id))
                return &m_data.handle;
            else
                return m_data.ptr;
        }

        const void* data_address(int type_id) const
        {
            assert(!is_void(type_id));

            if(is_primitive_type(type_id))
                return m_data.primitive;
            else if(is_objhandle(type_id))
                return &m_data.handle;
            else
                return m_data.ptr;
        }

        /**
         * @brief Get the referenced object
         *
         * @note Only available if stored data is @b NOT primitive value
         */
        void* object_ref() const
        {
            return m_data.ptr;
        }

        void construct(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id)
        {
            assert(!is_void(type_id));

            if(is_primitive_type(type_id))
            {
                std::memset(m_data.primitive, 0, 8);
            }
            else if(is_objhandle(type_id))
            {
                m_data.handle = nullptr;
            }
            else
            {
                m_data.ptr = engine->CreateScriptObject(
                    engine->GetTypeInfoById(type_id)
                );
            }
        }

        void copy_construct(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id, const void* ref)
        {
            assert(!is_void(type_id));

            if(is_primitive_type(type_id))
            {
                copy_primitive_value(m_data.primitive, ref, type_id);
            }
            else if(is_objhandle(type_id))
            {
                void* handle = *static_cast<void* const*>(ref);
                m_data.handle = handle;
                if(handle)
                {
                    engine->AddRefScriptObject(
                        handle,
                        engine->GetTypeInfoById(type_id)
                    );
                }
            }
            else
            {
                m_data.ptr = engine->CreateScriptObjectCopy(
                    const_cast<void*>(ref),
                    engine->GetTypeInfoById(type_id)
                );
            }
        }

        void copy_assign_from(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id, const void* ref)
        {
            assert(!is_void(type_id));

            if(is_primitive_type(type_id))
            {
                copy_primitive_value(m_data.primitive, ref, type_id);
            }
            else if(is_objhandle(type_id))
            {
                AS_NAMESPACE_QUALIFIER asITypeInfo* ti = engine->GetTypeInfoById(type_id);
                if(m_data.handle)
                    engine->ReleaseScriptObject(m_data.handle, ti);
                void* handle = *static_cast<void* const*>(ref);
                m_data.handle = handle;
                if(handle)
                {
                    engine->AddRefScriptObject(
                        handle, ti
                    );
                }
            }
            else
            {
                engine->AssignScriptObject(
                    m_data.ptr,
                    const_cast<void*>(ref),
                    engine->GetTypeInfoById(type_id)
                );
            }
        }

        void copy_assign_to(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id, void* out) const
        {
            assert(!is_void(type_id));
            assert(out != nullptr);

            if(is_primitive_type(type_id))
            {
                copy_primitive_value(out, m_data.primitive, type_id);
            }
            else if(is_objhandle(type_id))
            {
                void** out_handle = static_cast<void**>(out);

                AS_NAMESPACE_QUALIFIER asITypeInfo* ti = engine->GetTypeInfoById(type_id);
                if(*out_handle)
                    engine->ReleaseScriptObject(*out_handle, ti);
                *out_handle = m_data.handle;
                if(m_data.handle)
                {
                    engine->AddRefScriptObject(
                        m_data.handle, ti
                    );
                }
            }
            else
            {
                engine->AssignScriptObject(
                    out,
                    m_data.ptr,
                    engine->GetTypeInfoById(type_id)
                );
            }
        }

        void destroy(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id)
        {
            if(is_primitive_type(type_id))
            {
                // Suppressing assertion in destructor
                assert((m_data.ptr = nullptr, true));
                return;
            }

            if(!m_data.ptr)
                return;
            engine->ReleaseScriptObject(
                m_data.ptr,
                engine->GetTypeInfoById(type_id)
            );
            m_data.ptr = nullptr;
        }

    private:
        union internal_t
        {
            std::byte primitive[8]; // primitive value
            void* handle; // script handle
            void* ptr; // script object
        };

        internal_t m_data;
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

            ~handle_proxy()
            {
                assert(m_handle == nullptr);
            }

            handle_proxy& operator=(handle_proxy&) = delete;
            handle_proxy& operator=(handle_proxy&&) = delete;

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
    };

    template <template <typename...> typename Container>
    struct container_traits;

    template <>
    struct container_traits<std::vector>
    {
        using sequence_container_tag = void;

        template <typename T, typename Allocator>
        using container_type = std::vector<T, Allocator>;
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

        void push_back(const void* ref)
        {
            impl().push_back(ref);
        }

        void* nth_address(size_type idx)
        {
            return impl().nth_address(idx);
        }

        const void* nth_address(size_type idx) const
        {
            return impl().nth_address(idx);
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

            virtual void* nth_address(size_type idx) const = 0;
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
                using std::size;
                return size(m_container);
            }

            void push_back(const void* ref) override
            {
                m_container.emplace_back(ref_to_elem_val(ref));
            }

            void* nth_address(size_type idx) const final
            {
                using std::begin;

                return (void*)std::addressof(*std::next(begin(m_container), idx));
            }

        private:
            AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine;
            container_type m_container;

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
                using value_type = typename seq_object::value_type;

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

                void construct(pointer mem, value_type&& val) noexcept
                {
                    new(mem) T(std::move(val));
                }

                void destroy(value_type* mem) noexcept
                {
                    if(!mem) [[unlikely]]
                        return;

                    mem->destroy(get_type_info());
                    mem->~T();
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
                return m_container.get_allocator().get_type_info();
            }

            size_type size() const noexcept final
            {
                using std::size;
                return size(m_container);
            }

            void push_back(const void* ref) override
            {
                m_container.emplace_back(ref_to_ptr(ref));
            }

            void* nth_address(size_type idx) const final
            {
                using std::begin;

                return (void*)std::next(begin(m_container), idx)->data_address();
            }

        private:
            int m_type_id;
            container_type m_container;

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
} // namespace container
} // namespace asbind20

#endif
