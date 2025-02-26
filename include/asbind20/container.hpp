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
            m_data.ptr= nullptr;
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
} // namespace container
} // namespace asbind20

#endif
