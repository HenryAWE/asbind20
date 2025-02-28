#ifndef ASBIND20_EXT_CONTAINER_DICTIONARY_HPP
#define ASBIND20_EXT_CONTAINER_DICTIONARY_HPP

#pragma once

#include <map>
#include <cstring>
#include <asbind20/asbind.hpp>

namespace asbind20::ext
{
class dictionary
{
public:
    using key_type = std::string;

    struct mapped_type
    {
        container::single data;
        int type_id;

        mapped_type() = delete;

        mapped_type(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, void* ref, int tid)
            : type_id(tid)
        {
           data.copy_construct(engine, tid, ref);
        }

        void get(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, void* out) const
        {
            data.copy_assign_to(engine, type_id, out);
        }

        void release_data(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
        {
            data.destroy(engine, type_id);
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
        m_gc_flag = false;
        ++m_refcount;
    }

    void release()
    {
        m_gc_flag = false;
        m_refcount.dec_and_try_delete(this);
    }

    int get_refcount() const;
    void set_gc_flag();
    bool get_gc_flag() const;
    void enum_refs(asIScriptEngine* engine);
    void release_refs(asIScriptEngine* engine);

private:
    container_type m_container;
    std::mutex m_mx;
    atomic_counter m_refcount;
    bool m_gc_flag = false;
};

void register_script_dictionary(asIScriptEngine* engine, bool generic = has_max_portability());
} // namespace asbind20::ext

#endif
