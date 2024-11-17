#ifndef ASBIND20_EXT_CONTAINER_DICTIONARY_HPP
#define ASBIND20_EXT_CONTAINER_DICTIONARY_HPP

#pragma once

#include <map>
#include <cstring>
#include <asbind20/asbind.hpp>
#ifndef NDEBUG
#    include <iostream>
#endif

namespace asbind20::ext
{
class dictionary
{
public:
    using key_type = std::string;

    struct mapped_type
    {
        union storage_t
        {
            std::byte local[8];
            void* ptr;
        };

        storage_t storage;
        int type_id;

        mapped_type() = delete;

        mapped_type(asIScriptEngine* engine, void* ref, int tid)
            : type_id(tid)
        {
            std::memset(&storage, 0, sizeof(storage));

            if(!(type_id & ~asTYPEID_MASK_SEQNBR))
            {
                visit_primitive_type(
                    [this]<typename T>(T* val) -> void
                    {
                        static_assert(sizeof(T) <= sizeof(storage_t::local));
                        *(T*)storage.local = *val;
                    },
                    tid,
                    ref
                );
                return;
            }

            if(type_id & asTYPEID_OBJHANDLE)
            {
                *(void**)storage.local = ref;
                if(ref)
                    engine->AddRefScriptObject(ref, engine->GetTypeInfoById(type_id));
            }
            else
            {
                if(!ref)
                {
                    storage.ptr = nullptr;
                    return;
                }

                storage.ptr = engine->CreateScriptObjectCopy(ref, type_info(engine));
            }
        }

        void get(asIScriptEngine* engine, void* out) const
        {
            if(!(type_id & ~asTYPEID_MASK_SEQNBR))
            {
                copy_primitive_value(out, storage.local, type_id);
                return;
            }

            if(!(type_id & asTYPEID_OBJHANDLE))
            {
                assert(type_id & asTYPEID_MASK_OBJECT);
                engine->AssignScriptObject(out, storage.ptr, type_info(engine));
            }
            if(type_id & asTYPEID_OBJHANDLE)
            {
                asITypeInfo* ti = type_info(engine);

                void* tmp = *(void**)out;
                *(void**)out = *(void**)storage.local;
                engine->AddRefScriptObject(*(void**)storage.local, ti);
                if(tmp)
                    engine->ReleaseScriptObject(tmp, ti);
            }
        }

        asITypeInfo* type_info(asIScriptEngine* engine) const
        {
            assert(type_id & ~asTYPEID_MASK_SEQNBR);
            return engine->GetTypeInfoById(type_id);
        }

        void release_data(asIScriptEngine* engine)
        {
            if(!(type_id & ~asTYPEID_MASK_SEQNBR))
                return;

            if(type_id & asTYPEID_OBJHANDLE)
            {
                void* handle = *(void**)storage.local;
                if(handle)
                {
                    asITypeInfo* ti = type_info(engine);
                    engine->ReleaseScriptObject(handle, ti);
                }
            }
            else
            {
                void* handle = storage.ptr;
                if(handle)
                {
                    asITypeInfo* ti = type_info(engine);
                    engine->ReleaseScriptObject(handle, ti);
                }
            }
        }
    };

    using value_type = std::pair<const std::string, mapped_type>;

    template <typename T, typename UnderlyingAlloc = as_allocator<T>>
    class allocator : public UnderlyingAlloc
    {
        using my_base = UnderlyingAlloc;

    public:
        using value_type = T;
        using underlying_allocator = UnderlyingAlloc;

        template <typename U>
        struct rebind
        {
            using other = allocator<U>;
        };

        allocator(asIScriptEngine* engine)
            : my_base(), m_engine(engine) {}

        template <typename U>
        allocator(const allocator<U>& other) noexcept
            : my_base(other), m_engine(other.get_engine())
        {}

        asIScriptEngine* get_engine() const noexcept
        {
            return m_engine;
        }

        void destroy(mapped_type* p)
        {
#ifndef NDEBUG
            std::cerr
                << "[dict alloc] Destroy at " << (void*)p << ", type = " << p->type_id
                << std::endl;
#endif
            p->release_data(m_engine);
        }

        void destroy(dictionary::value_type* p)
        {
            destroy(&p->second);
            std::destroy_at(p);
        }

    private:
        asIScriptEngine* m_engine;
    };

    using container_type = std::map<
        key_type,
        mapped_type,
        std::less<>,
        allocator<value_type>>;

    using iterator = typename container_type::iterator;

    dictionary() = delete;

    dictionary(asIScriptEngine* engine)
        : m_container(allocator<value_type>(engine)) {}

    bool try_emplace(const key_type& k, const void* value, int type_id)
    {
        return m_container.try_emplace(
                              k,
                              get_engine(),
                              const_cast<void*>(value),
                              type_id
        )
            .second;
    }

    bool get(const key_type& k, void* value, int type_id) const
    {
        auto it = m_container.find(k);
        if(it == m_container.end())
            return false;
        if(it->second.type_id != type_id)
            return false;

        it->second.get(get_engine(), value);
        return true;
    }

    bool erase(const key_type& k)
    {
        auto it = m_container.find(k);
        if(it == m_container.end())
            return false;

        m_container.erase(it);
        return true;
    }

    bool contains(std::string_view k) const
    {
        return m_container.contains(k);
    }

    bool contains(const key_type& k) const
    {
        return m_container.contains(k);
    }

    void clear() noexcept
    {
        m_container.clear();
    }

    asIScriptEngine* get_engine() const
    {
        return m_container.get_allocator().get_engine();
    }

    void lock()
    {
        m_mx.lock();
    }

    bool try_lock()
    {
        return m_mx.try_lock();
    }

    void unlock()
    {
        m_mx.unlock();
    }

    void addref()
    {
        asAtomicInc(m_refcount);
    }

    void release()
    {
        if(asAtomicDec(m_refcount) == 0)
            delete this;
    }

    int get_refcount() const;
    void set_gc_flag();
    bool get_gc_flag() const;
    void enum_refs(asIScriptEngine* engine);
    void release_refs(asIScriptEngine* engine);

private:
    container_type m_container;
    std::mutex m_mx;
    int m_refcount = 1;
    bool m_gc_flag = false;
};

void register_script_dictionary(asIScriptEngine* engine, bool generic = has_max_portability());
} // namespace asbind20::ext

#endif
