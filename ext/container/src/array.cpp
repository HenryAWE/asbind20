#include <asbind20/ext/array.hpp>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <asbind20/invoke.hpp>

#ifndef ASBIND20_EXT_ARRAY_CACHE_ID
#    define ASBIND20_EXT_ARRAY_CACHE_ID 2000
#endif

namespace asbind20::ext
{
asPWORD script_array_cache_id()
{
    return static_cast<asPWORD>(ASBIND20_EXT_ARRAY_CACHE_ID);
}

static asUINT get_elem_size(asIScriptEngine* engine, int subtype_id)
{
    if(subtype_id & asTYPEID_MASK_OBJECT)
        return sizeof(asPWORD);
    else
        return engine->GetSizeOfPrimitiveType(subtype_id);
}

void script_array::notify_gc_for_this()
{
    assert(m_ti != nullptr);
    if(m_ti->GetFlags() & asOBJ_GC)
        m_ti->GetEngine()->NotifyGarbageCollectorOfNewObject(this, m_ti);
}

script_array::script_array(asITypeInfo* ti)
    : m_ti(ti), m_subtype_id(ti->GetSubTypeId())
{
    asIScriptEngine* engine = m_ti->GetEngine();

    m_ti->AddRef();
    m_elem_size = get_elem_size(engine, m_subtype_id);

    cache_data();

    notify_gc_for_this();
}

script_array::script_array(const script_array& other)
    : m_ti(other.m_ti), m_subtype_id(other.m_subtype_id), m_elem_size(other.m_elem_size)
{
    asIScriptEngine* engine = m_ti->GetEngine();

    m_ti->AddRef();
    m_elem_size = get_elem_size(engine, m_subtype_id);

    if(!other.empty())
    {
        mem_resize_to(other.size());
        copy_construct_range(
            m_data.ptr,
            other.m_data.ptr,
            other.size()
        );
        m_data.size = other.size();
    }

    notify_gc_for_this();
}

script_array::script_array(asITypeInfo* ti, size_type n)
    : m_ti(ti), m_subtype_id(ti->GetSubTypeId())
{
    asIScriptEngine* engine = m_ti->GetEngine();

    m_ti->AddRef();
    m_elem_size = get_elem_size(engine, m_subtype_id);

    cache_data();

    if(n != 0)
    {
        mem_resize_to(n);
        default_construct_n(m_data.ptr, n);
        m_data.size = n;
    }

    notify_gc_for_this();
}

script_array::script_array(asITypeInfo* ti, size_type n, const void* value)
    : m_ti(ti), m_subtype_id(ti->GetSubTypeId())
{
    asIScriptEngine* engine = m_ti->GetEngine();

    m_ti->AddRef();
    m_elem_size = get_elem_size(engine, m_subtype_id);

    if(n != 0)
    {
        mem_resize_to(n);
        value_construct_n(m_data.ptr, value, n);
        m_data.size = n;
    }

    notify_gc_for_this();
}

script_array::script_array(asITypeInfo* ti, void* list_buf)
    : m_ti(ti), m_subtype_id(ti->GetSubTypeId())
{
    asIScriptEngine* engine = ti->GetEngine();

    m_ti->AddRef();
    m_elem_size = get_elem_size(engine, m_subtype_id);

    script_init_list_repeat init_list(list_buf);

    cache_data();

    mem_resize_to(init_list.size());
    if(!(m_subtype_id & asTYPEID_MASK_OBJECT))
    {
        if(init_list.size() > 0)
        {
            std::memcpy(
                m_data.ptr,
                init_list.data(),
                static_cast<std::size_t>(init_list.size()) * m_elem_size
            );
        }
        m_data.size = init_list.size();
    }
    else if((m_subtype_id & asTYPEID_OBJHANDLE) || (subtype_flags() & asOBJ_REF))
    {
        if(init_list.size() > 0)
        {
            std::memcpy(
                m_data.ptr,
                init_list.data(),
                static_cast<std::size_t>(init_list.size()) * m_elem_size
            );
            std::memset(
                init_list.data(),
                0,
                static_cast<std::size_t>(init_list.size()) * m_elem_size
            );
        }
        m_data.size = init_list.size();
    }
    else
    {
        assert((m_subtype_id & asTYPEID_MASK_OBJECT) && !(m_subtype_id & asTYPEID_OBJHANDLE));

        asITypeInfo* subtype_ti = engine->GetTypeInfoById(m_subtype_id);
        assert(subtype_ti == m_ti->GetSubType());

        size_type list_elem_size = subtype_ti->GetSize();
        for(size_type i = 0; i < init_list.size(); ++i)
        {
            assert(m_elem_size == sizeof(void*));
            void** dst = (void**)(m_data.ptr + i * m_elem_size);
            void* src = static_cast<std::byte*>(init_list.data()) + i * list_elem_size;

            *dst = engine->CreateScriptObjectCopy(src, subtype_ti);
        }

        m_data.size = init_list.size();
    }

    notify_gc_for_this();
}

script_array::~script_array()
{
    clear();
    deallocate(m_data.ptr);
    if(m_ti)
        m_ti->Release();
}

script_array& script_array::operator=(const script_array& other)
{
    if(m_within_callback)
    {
        set_script_exception("Cannot modify array within callback");
        return *this;
    }

    if(this == &other)
        return *this;

    assert(m_ti == other.m_ti);

    clear();
    append_range(other, other.size());

    return *this;
}

void* script_array::operator new(std::size_t bytes)
{
    return allocate(bytes);
}

void script_array::operator delete(void* p)
{
    deallocate(p);
}

void script_array::addref()
{
    asAtomicInc(m_refcount);
}

void script_array::release()
{
    m_gc_flag = false;
    if(asAtomicDec(m_refcount) == 0)
    {
        delete this;
    }
}

void script_array::reserve(size_type new_cap)
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }

    if(new_cap <= capacity())
        return;

    size_type target_cap = capacity();
    if(new_cap - capacity() < target_cap / 2)
        target_cap += target_cap / 2;
    else
        target_cap = new_cap;

    mem_resize_to(target_cap);
}

void script_array::shrink_to_fit()
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }

    if(size() == 0)
    {
        if(!m_data.ptr)
            return;
        deallocate(m_data.ptr);
        m_data.ptr = nullptr;
    }
    mem_resize_to(size());
}

void script_array::clear()
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }

    if(empty())
        return;
    destroy_n(m_data.ptr, size());
    m_data.size = 0;
}

void script_array::push_back(const void* value)
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }

    reserve(size() + 1);
    value_construct_n(
        m_data.ptr + m_data.size * m_elem_size,
        value,
        1
    );
    ++m_data.size;

    return;
}

void script_array::pop_back()
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }

    if(empty())
        return;

    erase(size() - 1);
}

void script_array::append_range(const script_array& rng, size_type n)
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }

    n = std::min(rng.size(), n);
    if(n == 0)
        return;
    reserve(size() + n);

    copy_construct_range(
        m_data.ptr + size() * m_elem_size,
        rng.m_data.ptr,
        n
    );
    m_data.size += n;
}

void script_array::insert(size_type idx, void* value)
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }

    if(idx > size())
    {
        set_script_exception("out of range");
        return;
    }
    if(idx == size())
    {
        push_back(value);
        return;
    }

    reserve(size() + 1);
    insert_range_impl(idx, value, 1);
}

void script_array::insert_range(size_type idx, const script_array& rng, size_type n)
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }

    if(idx > size())
    {
        set_script_exception("out of range");
        return;
    }
    if(idx == size())
    {
        append_range(rng, n);
        return;
    }

    n = std::min(rng.size(), n);
    if(n == 0)
        return;
    reserve(size() + n);
    insert_range_impl(idx, rng[0], n);
}

void script_array::erase(size_type idx, size_type n)
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }

    if(idx >= size())
    {
        set_script_exception("out of range");
        return;
    }

    n = std::min(size() - idx, n);

    erase_impl(idx, n);
}

namespace detail
{
    // std::remove_if() is not guaranteed to preserve "removed" values at the end of the range,
    // which need to release later.
    template <typename Pred>
    static void** stable_remove_if(void** start, void** sentinel, Pred&& pred)
    {
        void** last = sentinel;

        void** it = start;
        while(it != last)
        {
            assert(it < last);
            if(std::invoke(std::forward<Pred>(pred), *it))
            {
                void* tmp = *it;
                std::copy(it + 1, last, it);
                --last;
                *last = tmp;
            }
            else
                ++it;
        }

        return last;
    }
} // namespace detail

script_array::size_type script_array::erase_value(const void* val, size_type idx, size_type n)
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return 0;
    }
    if(idx >= size())
    {
        set_script_exception("out of range");
        return 0;
    }

    n = std::min(size() - idx, n);
    if(n == 0)
        return 0;

    callback_guard guard(*this);

    if(m_subtype_id & ~asTYPEID_MASK_SEQNBR)
    {
        reuse_active_context ctx(m_ti->GetEngine());
        array_cache* cache = reinterpret_cast<array_cache*>(m_ti->GetUserData(script_array_cache_id()));

        void** start = (void**)(*this)[idx];
        void** sentinel = (void**)(*this)[idx + n];

        void** last = detail::stable_remove_if(
            start,
            sentinel,
            [=, this, &ctx](void* pv) -> bool
            {
                return elem_opEquals(&pv, &val, ctx, cache);
            }
        );

        size_type idx_to_erase = idx + std::distance(start, last);
        if(idx_to_erase == size())
            return 0;
        size_type n_to_erase = std::distance(last, sentinel);
        erase_impl(idx_to_erase, n_to_erase);

        return n_to_erase;
    }
    else
    {
        void* start = (*this)[idx];
        void* sentinel = (*this)[idx + n];

        reuse_active_context ctx(m_ti->GetEngine());
        return visit_primitive_type(
            [=, this, &ctx]<typename T>(T* start, T* sentinel) -> size_type
            {
                auto it = std::remove(start, sentinel, *(const T*)val);

                size_type idx_to_erase = idx + (size_type)std::distance(start, it);
                if(idx_to_erase == size())
                    return 0;
                size_type n_to_erase = std::distance(it, sentinel);
                erase_impl(idx_to_erase, n_to_erase);

                return n_to_erase;
            },
            m_subtype_id,
            start,
            sentinel
        );
    }
}

void script_array::sort(size_type idx, size_type n, bool asc)
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }

    array_cache* cache = reinterpret_cast<array_cache*>(m_ti->GetUserData(script_array_cache_id()));
    if(m_subtype_id & ~asTYPEID_MASK_SEQNBR)
    {
        if(!cache || cache->opCmp_status == asMULTIPLE_FUNCTIONS)
        {
            asITypeInfo* subtype_ti = m_ti->GetEngine()->GetTypeInfoById(m_subtype_id);
            set_script_exception(string_concat(
                "Type \"",
                subtype_ti->GetName(),
                "\" has multiple matching opCmp method"
            ));
            return;
        }
    }

    if(idx >= size())
    {
        set_script_exception("out of range");
        return;
    }

    n = std::min(size() - idx, n);
    if(n < 2)
        return;

    if(m_subtype_id & ~asTYPEID_MASK_SEQNBR)
    {
        reuse_active_context ctx(m_ti->GetEngine());
        auto cmp = [=, &ctx](void* lhs, void* rhs) -> bool
        {
            if(!asc)
                std::swap(lhs, rhs);

            if(lhs == nullptr)
                return true;
            if(rhs == nullptr)
                return false;

            auto result = script_invoke<int>(
                ctx,
                static_cast<asIScriptObject*>(lhs),
                cache->subtype_opCmp,
                static_cast<asIScriptObject*>(rhs)
            );

            if(!result.has_value())
                return false;
            return *result < 0;
        };
        std::sort((void**)(*this)[idx], (void**)(*this)[idx + n], cmp);
    }
    else
    {
        void* start = (*this)[idx];
        void* sentinel = (*this)[idx + n];

        visit_primitive_type(
            [asc]<typename T>(T* start, T* sentinel) -> void
            {
                if(asc)
                    std::sort(start, sentinel);
                else
                    std::sort(start, sentinel, std::greater<T>{});
            },
            m_subtype_id,
            start,
            sentinel
        );
    }
}

void script_array::reverse(size_type idx, size_type n)
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }

    if(idx >= size())
    {
        set_script_exception("out of range");
        return;
    }

    n = std::min(size() - idx, n);
    if(n < 2)
        return;

    if(m_subtype_id & ~asTYPEID_MASK_SEQNBR)
    {
        std::reverse((void**)(*this)[idx], (void**)(*this)[idx + n]);
    }
    else
    {
        void* start = (*this)[idx];
        void* sentinel = (*this)[idx + n];

        visit_primitive_type(
            []<typename T>(T* start, T* sentinel) -> void
            {
                std::reverse(start, sentinel);
            },
            m_subtype_id,
            start,
            sentinel
        );
    }
}

script_array::size_type script_array::find(const void* value, size_type idx) const
{
    if(idx >= size())
        return -1;

    if(m_subtype_id & ~asTYPEID_MASK_SEQNBR)
    {
        array_cache* cache = reinterpret_cast<array_cache*>(m_ti->GetUserData(script_array_cache_id()));

        reuse_active_context ctx(m_ti->GetEngine());
        for(size_type i = idx; i < size(); ++i)
        {
            if(elem_opEquals((*this)[i], &value, ctx, cache))
                return i;
        }
    }
    else // Primitive types
    {
        for(size_type i = idx; i < size(); ++i)
        {
            if(elem_opEquals((*this)[i], value, nullptr, nullptr))
                return i;
        }
    }

    return -1;
}

bool script_array::contains(const void* value, size_type idx) const
{
    return find(value, idx) != -1;
}

script_array::size_type script_array::count(const void* value, size_type idx, size_type n) const
{
    if(idx >= size())
        return 0;

    n = std::min(n, size() - idx);
    if(n == 0)
        return 0;

    size_type counter = 0;

    if(m_subtype_id & ~asTYPEID_MASK_SEQNBR)
    {
        array_cache* cache = reinterpret_cast<array_cache*>(m_ti->GetUserData(script_array_cache_id()));

        reuse_active_context ctx(m_ti->GetEngine());
        for(size_type i = idx; i < size(); ++i)
        {
            if(elem_opEquals((*this)[i], &value, ctx, cache))
                ++counter;
        }
    }
    else // Primitive types
    {
        for(size_type i = idx; i < size(); ++i)
        {
            if(elem_opEquals((*this)[i], value, nullptr, nullptr))
                ++counter;
        }
    }

    return counter;
}

script_optional* script_array::find_optional(const void* val, size_type idx)
{
    asIScriptEngine* engine = m_ti->GetEngine();

    size_type result = find(val, idx);

    asITypeInfo* ret_ti = engine->GetTypeInfoByDecl("optional<uint>");
    script_optional* opt = (script_optional*)engine->CreateScriptObject(ret_ti);
    if(!opt) [[unlikely]]
        return nullptr;
    if(result == -1)
        return opt;

    assert(opt->get_type_info()->GetSubTypeId() == asTYPEID_UINT32);
    opt->assign(&result);

    return opt;
}

void script_array::copy_construct_range(void* start, const void* input_start, size_type n)
{
    asIScriptEngine* engine = m_ti->GetEngine();
    if(m_subtype_id & asTYPEID_OBJHANDLE)
    {
        void** sentinel = (void**)start + n;
        void** src = (void**)input_start;
        void** dst = (void**)start;

        asITypeInfo* subtype_ti = engine->GetTypeInfoById(m_subtype_id);
        while(dst < sentinel)
        {
            *dst = *src;
            if(*dst)
                engine->AddRefScriptObject(*dst, subtype_ti);

            ++src;
            ++dst;
        }
    }
    else if(m_subtype_id & asTYPEID_MASK_OBJECT)
    {
        void** sentinel = (void**)start + n;
        void** src = (void**)input_start;
        void** dst = (void**)start;

        asITypeInfo* subtype_ti = m_ti->GetSubType();
        while(dst < sentinel)
        {
            *dst = engine->CreateScriptObjectCopy(*src, subtype_ti);

            ++src;
            ++dst;
        }
    }
    else // Primitive types
    {
        std::memcpy(
            start,
            input_start,
            static_cast<std::size_t>(n) * m_elem_size
        );
    }
}

void script_array::move_construct_range(void* start, void* input_start, size_type n)
{
    asIScriptEngine* engine = m_ti->GetEngine();
    if(m_subtype_id & asTYPEID_OBJHANDLE)
    {
        void** sentinel = (void**)start + n;
        void** src = (void**)input_start;
        void** dst = (void**)start;

        while(dst < sentinel)
        {
            assert(*dst == nullptr);
            *dst = std::exchange(*src, nullptr);

            ++src;
            ++dst;
        }
    }
    else if(m_subtype_id & asTYPEID_MASK_OBJECT)
    {
        void** sentinel = (void**)start + n;
        void** src = (void**)input_start;
        void** dst = (void**)start;

        asITypeInfo* subtype_ti = m_ti->GetSubType();
        while(dst < sentinel)
        {
            *dst = engine->CreateScriptObjectCopy(*src, subtype_ti);
            engine->ReleaseScriptObject(*src, subtype_ti);

            ++src;
            ++dst;
        }
    }
    else // Primitive types
    {
        std::memcpy(
            start,
            input_start,
            static_cast<std::size_t>(n) * m_elem_size
        );
    }
}

void script_array::move_construct_range_backward(void* start, void* input_start, size_type n)
{
    asIScriptEngine* engine = m_ti->GetEngine();
    if(m_subtype_id & asTYPEID_OBJHANDLE)
    {
        void** sentinel = (void**)start;
        void** p = (void**)input_start + n;
        void** q = (void**)start + n;

        while(q != sentinel)
        {
            assert(*(q - 1) == nullptr);
            *(q - 1) = std::exchange(*(p - 1), nullptr);

            --p;
            --q;
        }
    }
    else if(m_subtype_id & asTYPEID_MASK_OBJECT)
    {
        void** sentinel = (void**)start;
        void** p = (void**)input_start + n;
        void** q = (void**)start + n;

        asITypeInfo* subtype_ti = m_ti->GetSubType();
        while(q != sentinel)
        {
            *(q - 1) = engine->CreateScriptObjectCopy(*(p - 1), subtype_ti);
            engine->ReleaseScriptObject(*(p - 1), subtype_ti);

            --p;
            --q;
        }
    }
    else // Primitive types
    {
        std::size_t offset_bytes = static_cast<std::size_t>(n) * m_elem_size;
        std::copy_backward(
            (std::byte*)input_start,
            (std::byte*)input_start + offset_bytes,
            (std::byte*)start + offset_bytes
        );
    }
}

void script_array::copy_assign_range_backward(void* start, const void* input_start, size_type n)
{
    asIScriptEngine* engine = m_ti->GetEngine();
    if(m_subtype_id & asTYPEID_OBJHANDLE)
    {
        void** sentinel = (void**)start;
        void** src = (void**)input_start + n;
        void** dst = (void**)start + n;

        asITypeInfo* subtype_ti = engine->GetTypeInfoById(m_subtype_id);
        while(dst != sentinel)
        {
            void* old = *(dst - 1);
            *(dst - 1) = *(src - 1);
            if(*(dst - 1) != nullptr)
                engine->AddRefScriptObject(*(dst - 1), subtype_ti);
            if(old)
                engine->ReleaseScriptObject(old, subtype_ti);

            --src;
            --dst;
        }
    }
    else if(m_subtype_id & asTYPEID_MASK_OBJECT)
    {
        void** sentinel = (void**)start;
        void** src = (void**)input_start + n;
        void** dst = (void**)start + n;

        asITypeInfo* subtype_ti = m_ti->GetSubType();
        while(dst != sentinel)
        {
            *(dst - 1) = engine->CreateScriptObjectCopy(*(src - 1), subtype_ti);

            --src;
            --dst;
        }
    }
    else // Primitive types
    {
        std::size_t offset_bytes = static_cast<std::size_t>(n) * m_elem_size;
        std::copy_backward(
            (std::byte*)input_start,
            (std::byte*)input_start + offset_bytes,
            (std::byte*)start + offset_bytes
        );
    }
}

void script_array::insert_range_impl(size_type idx, const void* src, size_type n)
{
    assert(idx < size());
    assert(size() + n <= capacity());

    size_type elems_to_move = size() - idx;
    move_construct_range_backward(
        (*this)[idx + n],
        (*this)[idx],
        elems_to_move
    );
    copy_assign_range_backward(
        (*this)[idx],
        src,
        n
    );
    m_data.size += n;
}

void script_array::copy_assign_at(void* ptr, void* value)
{
    visit_script_type(
        overloaded{
            // objhandle
            [this](void** ptr, void** value)
            {
                void* old = *ptr;
                *ptr = *value;
                asIScriptEngine* engine = m_ti->GetEngine();
                asITypeInfo* subtype_ti = m_ti->GetSubType();
                engine->AddRefScriptObject(*value, subtype_ti);
                if(old)
                    engine->ReleaseScriptObject(old, subtype_ti);
            },
            // classes
            [this](void* ptr, void* value)
            {
                m_ti->GetEngine()->AssignScriptObject(ptr, value, m_ti->GetSubType());
            },
            // primitives
            [this]<typename T>(T* ptr, T* value) requires(std::is_fundamental_v<T>)
            {
                *ptr = *value;
            }
            },
            m_subtype_id,
            ptr,
            value
    );
}

void script_array::destroy_n(void* start, size_type n)
{
    if(m_subtype_id & asTYPEID_MASK_OBJECT)
    {
        asIScriptEngine* engine = m_ti->GetEngine();

        void** sentinel = (void**)start + n;

        for(void** p = static_cast<void**>(start); p != sentinel; ++p)
        {
            if(*p)
                engine->ReleaseScriptObject(*p, m_ti->GetSubType());
        }
    }

    std::memset(
        start,
        0,
        static_cast<std::size_t>(n) * m_elem_size
    );
}

bool script_array::elem_opEquals(const void* lhs, const void* rhs, asIScriptContext* ctx, const array_cache* cache) const
{
    if(is_primitive_type(m_subtype_id))
    {
        return visit_primitive_type(
            [](const auto* lhs, const auto* rhs) -> bool
            {
                return *lhs == *rhs;
            },
            m_subtype_id,
            lhs,
            rhs
        );
    }
    else
    {
        if(is_objhandle(m_subtype_id))
        {
            if(*(const void* const*)lhs == *(const void* const*)rhs)
                return true;
        }

        if(!cache) [[unlikely]]
            return false;

        assert(ctx != nullptr);
        if(cache->subtype_opEquals) [[likely]]
        {
            auto result = script_invoke<bool>(
                ctx,
                static_cast<const asIScriptObject*>(*(void**)lhs),
                cache->subtype_opEquals,
                *(void**)rhs
            );

            if(!result.has_value())
                return false;
            return *result;
        }
        // Fallback to OpCmp() == 0
        else if(cache->subtype_opCmp)
        {
            auto result = script_invoke<int>(
                ctx,
                static_cast<const asIScriptObject*>(*((void**)const_cast<void*>(lhs))),
                cache->subtype_opCmp,
                rhs
            );

            if(!result.has_value())
                return false;
            return *result == 0;
        }
    }

    return false;
}

void script_array::cache_data()
{
    if(!(m_subtype_id & ~asTYPEID_MASK_SEQNBR))
        return;

    array_cache* cache = reinterpret_cast<array_cache*>(m_ti->GetUserData(script_array_cache_id()));
    if(cache)
        return;

    std::lock_guard guard(as_exclusive_lock);

    // Double-check to prevent cache from being created by another thread
    cache = reinterpret_cast<array_cache*>(m_ti->GetUserData(script_array_cache_id()));
    if(cache)
        return;

    cache = reinterpret_cast<array_cache*>(allocate(sizeof(array_cache)));
    if(!cache)
    {
        asIScriptContext* ctx = asGetActiveContext();
        if(ctx)
            ctx->SetException("out of memory");
        return;
    }
    std::memset(cache, 0, sizeof(array_cache));

    bool must_be_const = m_subtype_id & asTYPEID_HANDLETOCONST;

    asITypeInfo* subtype_ti = m_ti->GetEngine()->GetTypeInfoById(m_subtype_id);
    assert(subtype_ti != nullptr);

    for(asUINT i = 0; i < subtype_ti->GetMethodCount(); ++i)
    {
        asIScriptFunction* func = subtype_ti->GetMethodByIndex(i);

        if(func->GetParamCount() == 1 && (!must_be_const || func->IsReadOnly()))
        {
            using namespace std::literals;

            asDWORD flags = 0;
            int return_type_id = func->GetReturnTypeId(&flags);
            if(flags != asTM_NONE)
                continue;

            bool is_opEquals = false;
            bool is_opCmp = false;

            if(return_type_id == asTYPEID_INT32 && func->GetName() == "opCmp"sv)
                is_opCmp = true;
            else if(return_type_id == asTYPEID_BOOL && func->GetName() == "opEquals"sv)
                is_opEquals = true;

            if(!is_opEquals && !is_opCmp)
                continue;

            int param_type_id;
            func->GetParam(0, &param_type_id, &flags);

            if((param_type_id & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)) !=
               (m_subtype_id & ~(asTYPEID_OBJHANDLE | asTYPEID_HANDLETOCONST)))
                continue;

            if((flags & asTM_INREF))
            {
                if((param_type_id & asTYPEID_OBJHANDLE) || (must_be_const && !(flags & asTM_CONST)))
                    continue;
            }
            else if(param_type_id & asTYPEID_OBJHANDLE)
            {
                if(must_be_const && !(param_type_id & asTYPEID_HANDLETOCONST))
                    continue;
            }
            else
                continue;

            if(is_opCmp)
            {
                if(cache->subtype_opCmp || cache->opCmp_status)
                {
                    cache->subtype_opCmp = 0;
                    cache->opCmp_status = asMULTIPLE_FUNCTIONS;
                }
                else
                    cache->subtype_opCmp = func;
            }
            else if(is_opEquals)
            {
                if(cache->subtype_opEquals || cache->opEquals_status)
                {
                    cache->subtype_opEquals = 0;
                    cache->opEquals_status = asMULTIPLE_FUNCTIONS;
                }
                else
                    cache->subtype_opEquals = func;
            }
        }
    }

    if(!cache->subtype_opEquals && cache->opEquals_status == 0)
        cache->opEquals_status = asNO_FUNCTION;
    if(!cache->subtype_opCmp && cache->opCmp_status == 0)
        cache->opCmp_status = asNO_FUNCTION;

    m_ti->SetUserData(cache, script_array_cache_id());
}

void script_array::cache_cleanup_callback(asITypeInfo* ti)
{
    deallocate(ti->GetUserData(script_array_cache_id()));
}

int script_array::get_refcount() const
{
    return m_refcount;
}

void script_array::set_gc_flag()
{
    m_gc_flag = true;
}

bool script_array::get_gc_flag() const
{
    return m_gc_flag;
}

void script_array::enum_refs(asIScriptEngine* engine)
{
    std::lock_guard guard(*this);

    // Mainly using implementation from the official add-oon

    if(m_subtype_id & asTYPEID_MASK_OBJECT)
    {
        void** p = (void**)m_data.ptr;

        asITypeInfo* subType = engine->GetTypeInfoById(m_subtype_id);
        if((subType->GetFlags() & asOBJ_REF))
        {
            for(asUINT i = 0; i < m_data.size; ++i)
            {
                if(p[i])
                    engine->GCEnumCallback(p[i]);
            }
        }
        else if((subType->GetFlags() & asOBJ_VALUE) && (subType->GetFlags() & asOBJ_GC))
        {
            for(asUINT i = 0; i < m_data.size; ++i)
            {
                if(p[i])
                    engine->ForwardGCEnumReferences(p[i], subType);
            }
        }
    }
}

void script_array::release_refs(asIScriptEngine* engine)
{
    clear();
}

void* script_array::operator[](size_type idx)
{
    std::size_t offset = static_cast<std::size_t>(idx) * m_elem_size;
    return m_data.ptr + offset;
}

const void* script_array::operator[](size_type idx) const
{
    std::size_t offset = static_cast<std::size_t>(idx) * m_elem_size;
    return m_data.ptr + offset;
}

bool script_array::element_indirect() const
{
    return (m_subtype_id & asTYPEID_MASK_OBJECT) &&
           !(m_subtype_id & asTYPEID_OBJHANDLE);
}

void* script_array::pointer_to(size_type idx)
{
    if(element_indirect())
        return *(void**)(*this)[idx];
    else
        return (*this)[idx];
}

const void* script_array::pointer_to(size_type idx) const
{
    if(element_indirect())
        return *(void**)(*this)[idx];
    else
        return (*this)[idx];
}

void* script_array::opIndex(size_type idx)
{
    if(idx >= size())
    {
        set_script_exception("Array index out of range");
        return nullptr;
    }

    return pointer_to(idx);
}

bool script_array::operator==(const script_array& other) const
{
    if(m_ti != other.m_ti)
        return false;
    if(size() != other.size())
        return false;

    assert(m_elem_size == other.m_elem_size);

    if(m_subtype_id & ~asTYPEID_MASK_SEQNBR)
    {
        array_cache* cache = reinterpret_cast<array_cache*>(m_ti->GetUserData(script_array_cache_id()));

        reuse_active_context ctx(m_ti->GetEngine());
        for(size_type i = 0; i < size(); ++i)
        {
            if(!elem_opEquals((*this)[i], other[i], ctx, cache))
                return false;
        }
    }
    else // Primitive types
    {
        for(size_type i = 0; i < size(); ++i)
        {
            if(!elem_opEquals((*this)[i], other[i], nullptr, nullptr))
                return false;
        }
    }

    return true;
}

void* script_array::get_front()
{
    if(empty())
    {
        set_script_exception("Empty array");
    }
    return pointer_to(0);
}

void* script_array::get_back()
{
    if(empty())
    {
        set_script_exception("Empty array");
    }

    return pointer_to(size() - 1);
}

void script_array::set_front(void* value)
{
    if(empty())
    {
        push_back(value);
    }
    else
    {
        copy_assign_at(pointer_to(0), value);
    }
}

void script_array::set_back(void* value)
{
    if(empty())
    {
        push_back(value);
    }
    else
    {
        copy_assign_at(pointer_to(size() - 1), value);
    }
}

asQWORD script_array::subtype_flags() const
{
    return m_ti->GetSubType()->GetFlags();
}

script_array::callback_guard::callback_guard(const script_array& this_) noexcept
    : m_this(this_)
{
    assert(!m_this.m_within_callback);
    m_this.m_within_callback = true;
}

script_array::callback_guard::~callback_guard()
{
    m_this.m_within_callback = false;
}

void* script_array::allocate(std::size_t bytes)
{
    return asAllocMem(bytes);
}

void script_array::deallocate(void* mem) noexcept
{
    asFreeMem(mem);
}

void script_array::default_construct_n(void* start, size_type n)
{
    if((m_subtype_id & asTYPEID_MASK_OBJECT) && !(m_subtype_id & asTYPEID_OBJHANDLE))
    {
        void** sentinel = (void**)(static_cast<std::byte*>(start) + n * sizeof(void*));

        asIScriptEngine* engine = m_ti->GetEngine();
        asITypeInfo* subtype_ti = m_ti->GetSubType();

        for(void** p = static_cast<void**>(start); p < sentinel; ++p)
        {
            *p = static_cast<void*>(engine->CreateScriptObject(subtype_ti));

            if(*p == nullptr) [[unlikely]]
            { // Failed to create script object
                // According to the official array add-on,
                // the remaining entries need to set to null so destructor won't destroy invalid objects.
                // Besides, the exception has already be set by CreateScriptObject().
                std::memset(p, 0, sizeof(void*) * (sentinel - p));
                return;
            }
        }
    }
    else
    {
        std::memset(
            start,
            0,
            static_cast<std::size_t>(n) * m_elem_size
        );
    }
}

void script_array::value_construct_n(void* start, const void* value, size_type n)
{
    for(size_type i = 0; i < n; ++i)
    {
        void* ptr = static_cast<std::byte*>(start) + i * m_elem_size;

        if((m_subtype_id & ~asTYPEID_MASK_SEQNBR) && !(m_subtype_id & asTYPEID_OBJHANDLE))
        {
            void** dst = static_cast<void**>(ptr);
            *dst = m_ti->GetEngine()->CreateScriptObjectCopy(
                const_cast<void*>(value),
                m_ti->GetSubType()
            );
        }
        else if(m_subtype_id & asTYPEID_OBJHANDLE)
        {
            assert(*(void**)ptr == nullptr);
            *(void**)ptr = *(void**)value;
            m_ti->GetEngine()->AddRefScriptObject(*(void**)value, m_ti->GetSubType());
        }
        else // primitive types
        {
            copy_primitive_value(ptr, value, m_subtype_id);
        }
    }
}

bool script_array::mem_resize_to(size_type new_cap)
{
    assert(new_cap != 0);
    assert(new_cap >= size());

    std::byte* tmp = (std::byte*)allocate(
        static_cast<std::size_t>(new_cap) * m_elem_size
    );
    if(!tmp)
    {
        set_script_exception("out of memory");
        return false;
    }
    std::memset(
        tmp,
        0,
        static_cast<std::size_t>(new_cap) * m_elem_size
    );

    if(m_data.ptr)
    {
        move_construct_range(tmp, m_data.ptr, m_data.size);
        deallocate(m_data.ptr);
    }
    m_data.capacity = new_cap;
    m_data.ptr = tmp;

    return true;
}

void script_array::erase_impl(size_type idx, size_type n)
{
    assert(idx < size());
    assert(idx + n <= size());

    if(n == 0)
        return;

    size_type elems_to_move = size() - idx - n;
    destroy_n((*this)[idx], n);
    if(elems_to_move != 0)
    {
        move_construct_range(
            (*this)[idx],
            (*this)[idx + n],
            elems_to_move
        );
    }

    m_data.size -= n;
}

script_array::size_type script_array::script_find_if(asIScriptFunction* fn, size_type idx) const
{
    if(!fn) [[unlikely]]
        return -1;

    if(idx >= size())
        return -1;

    callback_guard guard(*this);

    reuse_active_context ctx(m_ti->GetEngine());
    for(size_type i = idx; i < size(); ++i)
    {
        const void* ptr = (m_subtype_id & ~asTYPEID_MASK_SEQNBR) ?
                              *(const void**)(*this)[i] :
                              (*this)[i];

        auto result = script_invoke<bool>(ctx, fn, ptr);
        if(!result.has_value()) [[unlikely]]
            return -1;
        else if(*result)
            return i;
    }

    // Not found
    return -1;
}

script_array::size_type script_array::script_erase_if(asIScriptFunction* fn, size_type idx, size_type n)
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return 0;
    }
    if(idx >= size())
    {
        set_script_exception("out of range");
        return 0;
    }

    n = std::min(size() - idx, n);
    if(n == 0)
        return 0;

    callback_guard guard(*this);

    if(m_subtype_id & ~asTYPEID_MASK_SEQNBR)
    {
        reuse_active_context ctx(m_ti->GetEngine());
        auto pred = [=, &ctx](void* pv) -> bool
        {
            if(pv == nullptr)
                return false;

            auto result = script_invoke<bool>(
                ctx,
                fn,
                static_cast<asIScriptObject*>(pv)
            );

            if(!result.has_value()) [[unlikely]]
                return false;
            return *result;
        };

        void** start = (void**)(*this)[idx];
        void** sentinel = (void**)(*this)[idx + n];

        void** last = detail::stable_remove_if(
            start,
            sentinel,
            pred
        );

        void** it = start;
        while(it != last)
        {
            assert(it < last);
            if(pred(*it))
            {
                void* tmp = *it;
                std::copy(it + 1, last, it);
                --last;
                *last = tmp;
            }
            else
                ++it;
        }

        size_type idx_to_erase = idx + std::distance(start, last);
        if(idx_to_erase == size())
            return 0;
        size_type n_to_erase = std::distance(last, sentinel);
        erase_impl(idx_to_erase, n_to_erase);

        return n_to_erase;
    }
    else
    {
        void* start = (*this)[idx];
        void* sentinel = (*this)[idx + n];

        reuse_active_context ctx(m_ti->GetEngine());
        return visit_primitive_type(
            [=, this, &ctx]<typename T>(T* start, T* sentinel) -> size_type
            {
                auto pred = [=, &ctx](const T& v) -> bool
                {
                    auto result = script_invoke<bool>(ctx, fn, &v);

                    if(!result.has_value()) [[unlikely]]
                        return false;
                    else
                        return *result;
                };

                auto it = std::remove_if(start, sentinel, pred);

                size_type idx_to_erase = idx + (size_type)std::distance(start, it);
                if(idx_to_erase == size())
                    return 0;
                size_type n_to_erase = std::distance(it, sentinel);
                erase_impl(idx_to_erase, n_to_erase);

                return n_to_erase;
            },
            m_subtype_id,
            start,
            sentinel
        );
    }
}

void script_array::script_for_each(asIScriptFunction* fn, size_type idx, size_type n) const
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }
    if(idx >= size())
        return;

    callback_guard guard(*this);

    n = std::min(n, size() - idx);

    reuse_active_context ctx(m_ti->GetEngine(), true);
    for(size_type i = idx; i < n; ++i)
    {
        auto result = script_invoke<void>(ctx, fn, pointer_to(i));
        if(!result.has_value())
            return;
    }
}

void script_array::script_sort_by(asIScriptFunction* fn, size_type idx, size_type n, bool stable)
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return;
    }

    if(idx >= size())
    {
        set_script_exception("out of range");
        return;
    }

    n = std::min(size() - idx, n);
    if(n < 2)
        return;

    callback_guard guard(*this);

    if(m_subtype_id & ~asTYPEID_MASK_SEQNBR)
    {
        reuse_active_context ctx(m_ti->GetEngine());
        auto cmp = [=, &ctx](void* lhs, void* rhs) -> bool
        {
            if(lhs == nullptr)
                return true;
            if(rhs == nullptr)
                return false;

            auto result = script_invoke<bool>(
                ctx,
                fn,
                static_cast<asIScriptObject*>(lhs),
                static_cast<asIScriptObject*>(rhs)
            );

            if(!result.has_value())
                return false;
            return *result;
        };

        if(stable)
            std::stable_sort((void**)(*this)[idx], (void**)(*this)[idx + n], cmp);
        else
            std::sort((void**)(*this)[idx], (void**)(*this)[idx + n], cmp);
    }
    else
    {
        void* start = (*this)[idx];
        void* sentinel = (*this)[idx + n];

        reuse_active_context ctx(m_ti->GetEngine());
        visit_primitive_type(
            [=, &ctx]<typename T>(T* start, T* sentinel)
            {
                auto cmp = [=, &ctx](const T& lhs, const T& rhs) -> bool
                {
                    auto result = script_invoke<bool>(ctx, fn, &lhs, &rhs);

                    if(!result.has_value()) [[unlikely]]
                        return false;
                    else
                        return *result;
                };

                if(stable)
                    std::stable_sort(start, sentinel, cmp);
                else
                    std::sort(start, sentinel, cmp);
            },
            m_subtype_id,
            start,
            sentinel
        );
    }
}

script_array::size_type script_array::script_count_if(asIScriptFunction* fn, size_type idx, size_type n) const
{
    if(m_within_callback) [[unlikely]]
    {
        set_script_exception("Cannot modify array within callback");
        return 0;
    }

    if(idx >= size())
        return 0;

    n = std::min(size() - idx, n);
    if(n == 0)
        return 0;

    callback_guard guard(*this);

    size_type counter = 0;

    if(m_subtype_id & ~asTYPEID_MASK_SEQNBR)
    {
        reuse_active_context ctx(m_ti->GetEngine());
        auto pred = [=, &ctx](const void* val) -> bool
        {
            if(!val)
                return false;

            auto result = script_invoke<bool>(
                ctx,
                fn,
                static_cast<const asIScriptObject*>(val)
            );

            if(!result.has_value())
                return false;
            return *result;
        };

        for(const void** it = (const void**)(*this)[idx]; it != (const void**)(*this)[idx + n]; ++it)
        {
            if(pred(*it))
                ++counter;
        }
    }
    else
    {
        const void* start = (*this)[idx];
        const void* sentinel = (*this)[idx + n];

        reuse_active_context ctx(m_ti->GetEngine());
        visit_primitive_type(
            [=, &ctx, &counter]<typename T>(const T* start, const T* sentinel)
            {
                for(const T* it = start; it != sentinel; ++it)
                {
                    auto result = script_invoke<bool>(ctx, fn, it);

                    if(!result.has_value()) [[unlikely]]
                        return;
                    else if(*result)
                        ++counter;
                };
            },
            m_subtype_id,
            start,
            sentinel
        );
    }

    return counter;
}

static bool array_template_callback(asITypeInfo* ti, bool& no_gc)
{
    // Using implementation from the official add-on

    int type_id = ti->GetSubTypeId();
    if(type_id == asTYPEID_VOID)
        return false;

    if((type_id & asTYPEID_MASK_OBJECT) && !(type_id & asTYPEID_OBJHANDLE))
    {
        asITypeInfo* subtype_ti = ti->GetEngine()->GetTypeInfoById(type_id);
        asQWORD flags = subtype_ti->GetFlags();
        if((flags & asOBJ_VALUE) && !(flags & asOBJ_POD))
        {
            if(!get_default_constructor(subtype_ti))
            {
                ti->GetEngine()->WriteMessage(
                    "array", 0, 0, asMSGTYPE_ERROR, "The subtype has no default constructor"
                );
                return false;
            }
        }
        else if((flags & asOBJ_REF))
        {
            if(!get_default_factory(ti->GetSubType()))
            {
                ti->GetEngine()->WriteMessage(
                    "array", 0, 0, asMSGTYPE_ERROR, "The subtype has no default factory"
                );
                return false;
            }
        }

        // The object type is not garbage collected
        if(!(flags & asOBJ_GC))
            no_gc = true;
    }
    else if(!(type_id & asTYPEID_OBJHANDLE))
    {
        // Arrays with primitives cannot form circular references.
        no_gc = true;
    }
    else
    {
        assert(type_id & asTYPEID_OBJHANDLE);

        // According to the official add-on, if a handle cannot refer to an object type
        // that can form a circular reference with the array,
        // it's not necessary to make the array garbage collected.
        asITypeInfo* subtype_ti = ti->GetEngine()->GetTypeInfoById(type_id);
        asQWORD flags = subtype_ti->GetFlags();
        if(!(flags & asOBJ_GC))
        {
            if((flags & asOBJ_SCRIPT_OBJECT))
            {
                // According to the official add-on, the classes that derive from it can form a circular reference.
                if((flags & asOBJ_NOINHERIT))
                {
                    // A script class declared as final cannot be inherited from.
                    no_gc = true;
                }
            }
            else
            {
                // Class registered by host, we assume it knows what it is doing.
                no_gc = true;
            }
        }
    }

    return true;
}

namespace detail
{
    template <bool UseGeneric>
    void register_script_array_impl(asIScriptEngine* engine, bool as_default)
    {
        template_class<script_array, UseGeneric> c(engine, "array<T>", asOBJ_GC);
        c
            .template_callback(fp<&array_template_callback>)
            .default_factory()
            .template factory<asUINT>("uint", use_explicit)
            .template factory<asUINT, const void*>("uint, const T&in")
            .list_factory("repeat T")
            .opAssign()
            .opEquals()
            .addref(fp<&script_array::addref>)
            .release(fp<&script_array::release>)
            .get_refcount(fp<&script_array::get_refcount>)
            .set_gc_flag(fp<&script_array::set_gc_flag>)
            .get_gc_flag(fp<&script_array::get_gc_flag>)
            .enum_refs(fp<&script_array::enum_refs>)
            .release_refs(fp<&script_array::release_refs>)
            .method("T& opIndex(uint idx)", fp<&script_array::opIndex>)
            .method("const T& opIndex(uint idx) const", fp<&script_array::opIndex>)
            .method("T& get_front() property", fp<&script_array::get_front>)
            .method("const T& get_front() const property", fp<&script_array::get_front>)
            .method("void set_front(const T&in) property", fp<&script_array::set_front>)
            .method("T& get_back() property", fp<&script_array::get_back>)
            .method("const T& get_back() const property", fp<&script_array::get_back>)
            .method("void set_back(const T&in) property", fp<&script_array::set_back>)
            .method("uint get_size() const property", fp<&script_array::size>)
            .method("uint get_capacity() const property", fp<&script_array::capacity>)
            .method("bool get_empty() const property", fp<&script_array::empty>)
            .method("void reserve(uint)", fp<&script_array::reserve>)
            .method("void shrink_to_fit()", fp<&script_array::shrink_to_fit>)
            .method("void push_back(const T&in value)", fp<&script_array::push_back>)
            .method("void append_range(const array<T>&in rng, uint n=-1)", fp<&script_array::append_range>)
            .method("void insert(uint idx, const T&in value)", fp<&script_array::insert>)
            .method("void insert_range(uint idx, const array<T>&in rng, uint n=-1)", fp<&script_array::insert_range>)
            .method("void pop_back()", fp<&script_array::pop_back>)
            .method("void erase(uint idx, uint n=1)", fp<&script_array::erase>)
            .method("uint erase_value(const T&in value, uint idx=0, uint n=-1)", fp<&script_array::erase_value>)
            .funcdef("bool erase_if_callback(const T&in if_handle_then_const)")
            .method("void erase_if(const erase_if_callback&in fn, uint idx=0, uint n=-1)", fp<&script_array::script_erase_if>)
            .funcdef("void for_each_callback(const T&in if_handle_then_const)")
            .method("void for_each(const for_each_callback&in fn, uint idx=0, uint n=-1) const", fp<&script_array::script_for_each>)
            .method("void sort(uint idx=0, uint n=-1, bool asc=true)", fp<&script_array::sort>)
            .method("void reverse(uint idx=0, uint n=-1)", fp<&script_array::reverse>)
            .funcdef("bool sort_by_callback(const T&in if_handle_then_const, const T&in if_handle_then_const)")
            .method("void sort_by(const sort_by_callback&in fn, uint idx=0, uint n=-1, bool stable=true)", fp<&script_array::script_sort_by>)
            .method("uint find(const T&in, uint idx=0) const", fp<&script_array::find>)
            .funcdef("bool find_if_callback(const T&in)")
            .method("uint find_if(const find_if_callback&in fn, uint idx=0) const", fp<&script_array::script_find_if>)
            .method("bool contains(const T&in, uint idx=0) const", fp<&script_array::contains>)
            .method("uint count(const T&in, uint idx=0, uint n=-1) const", fp<&script_array::count>)
            .funcdef("bool count_if_callback(const T&in if_handle_then_const)")
            .method("uint count_if(const count_if_callback&in fn, uint idx=0, uint n=-1) const", fp<&script_array::script_count_if>)
            .method("void clear()", fp<&script_array::clear>);

        if(engine->GetTypeIdByDecl("optional<uint>") >= 0)
        {
            c
                .method("optional<uint>@ find_optional(const T&in, uint idx=0) const", fp<&script_array::find_optional>);
        }

        if(as_default)
            c.as_array();

        engine->SetTypeInfoUserDataCleanupCallback(
            &script_array::cache_cleanup_callback,
            script_array_cache_id()
        );
    }
} // namespace detail

void register_script_array(asIScriptEngine* engine, bool as_default, bool generic)
{
    if(generic)
        detail::register_script_array_impl<true>(engine, as_default);
    else
        detail::register_script_array_impl<false>(engine, as_default);
}
} // namespace asbind20::ext
