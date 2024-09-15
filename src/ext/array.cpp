#include <asbind20/ext/array.hpp>
#include <cassert>
#include <cstring>
#include <algorithm>
#include <asbind20/invoke.hpp>

#ifndef ASBIND20_EXT_ARRAY_CACHE_ID
#    define ASBIND20_EXT_ARRAY_CACHE_ID (asPWORD(2000))
#endif

namespace asbind20::ext
{
asPWORD script_array_cache_id()
{
    return ASBIND20_EXT_ARRAY_CACHE_ID;
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

    script_init_list init_list(list_buf);

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
    if(this == &other)
        return *this;
    script_array(other).swap(*this);
    return *this;
}

void* script_array::operator new(std::size_t bytes)
{
    std::basic_string<char, std::char_traits<char>, as_allocator<char>> str;
    return allocate(bytes);
}

void script_array::operator delete(void* p)
{
    deallocate(p);
}

void script_array::swap(script_array& other) noexcept
{
    using std::swap;

    swap(m_refcount, other.m_refcount);
    swap(m_gc_flag, other.m_gc_flag);
    swap(m_ti, other.m_ti);
    swap(m_subtype_id, other.m_subtype_id);
    swap(m_elem_size, other.m_elem_size);
    swap(m_data, other.m_data);
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
    if(empty())
        return;
    destroy_n(m_data.ptr, size());
    m_data.size = 0;
}

void script_array::push_back(const void* value)
{
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
    if(empty())
        return;

    erase(size() - 1);
}

void script_array::append_range(const script_array& rng, size_type n)
{
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
    if(idx >= size())
    {
        set_script_exception("out of range");
        return;
    }
    if(n == 0)
        return;

    n = std::min(size() - idx, n);
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

void script_array::sort(size_type idx, size_type n, bool asc)
{
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


#define ASBIND20_EXT_ARRAY_SORT_CASE_IMPL(as_type_id) \
    case as_type_id: ASBIND20_EXT_ARRAY_SORT_IMPL(primitive_type_of_t<as_type_id>); break

#define ASBIND20_EXT_ARRAY_SORT_SWITCH_IMPL()               \
    switch(m_subtype_id)                                    \
    {                                                       \
        ASBIND20_EXT_ARRAY_SORT_CASE_IMPL(asTYPEID_BOOL);   \
        ASBIND20_EXT_ARRAY_SORT_CASE_IMPL(asTYPEID_INT8);   \
        ASBIND20_EXT_ARRAY_SORT_CASE_IMPL(asTYPEID_INT16);  \
        ASBIND20_EXT_ARRAY_SORT_CASE_IMPL(asTYPEID_INT32);  \
        ASBIND20_EXT_ARRAY_SORT_CASE_IMPL(asTYPEID_INT64);  \
        ASBIND20_EXT_ARRAY_SORT_CASE_IMPL(asTYPEID_UINT8);  \
        ASBIND20_EXT_ARRAY_SORT_CASE_IMPL(asTYPEID_UINT16); \
        ASBIND20_EXT_ARRAY_SORT_CASE_IMPL(asTYPEID_UINT32); \
        ASBIND20_EXT_ARRAY_SORT_CASE_IMPL(asTYPEID_UINT64); \
        ASBIND20_EXT_ARRAY_SORT_CASE_IMPL(asTYPEID_FLOAT);  \
        ASBIND20_EXT_ARRAY_SORT_CASE_IMPL(asTYPEID_DOUBLE); \
    default:                                                \
        ASBIND20_EXT_ARRAY_SORT_IMPL(int); /* enums */      \
        break;                                              \
    }

        if(asc)
        {
#define ASBIND20_EXT_ARRAY_SORT_IMPL(type) \
    std::sort((type*)start, (type*)sentinel)

            ASBIND20_EXT_ARRAY_SORT_SWITCH_IMPL();

#undef ASBIND20_EXT_ARRAY_SORT_IMPL
        }
        else
        {
#define ASBIND20_EXT_ARRAY_SORT_IMPL(type) \
    std::sort((type*)start, (type*)sentinel, std::greater<>{})

            ASBIND20_EXT_ARRAY_SORT_SWITCH_IMPL();

#undef ASBIND20_EXT_ARRAY_SORT_IMPL
        }

#undef ASBIND20_EXT_ARRAY_SORT_SWITCH_IMPL
#undef ASBIND20_EXT_ARRAY_SORT_CASE_IMPL
    }
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
    if((m_subtype_id & ~asTYPEID_MASK_SEQNBR) && !(m_subtype_id & asTYPEID_OBJHANDLE))
    {
        m_ti->GetEngine()->AssignScriptObject(ptr, value, m_ti->GetSubType());
    }
    else if(m_subtype_id & asTYPEID_OBJHANDLE)
    {
        void* tmp = *(void**)ptr;
        *(void**)ptr = *(void**)value;
        m_ti->GetEngine()->AddRefScriptObject(*(void**)value, m_ti->GetSubType());
        if(tmp)
            m_ti->GetEngine()->ReleaseScriptObject(tmp, m_ti->GetSubType());
    }
    else if(m_subtype_id == asTYPEID_BOOL ||
            m_subtype_id == asTYPEID_INT8 ||
            m_subtype_id == asTYPEID_UINT8)
    {
        *(char*)ptr = *(char*)value;
    }
    else if(m_subtype_id == asTYPEID_INT16 ||
            m_subtype_id == asTYPEID_UINT16)
    {
        *(short*)ptr = *(short*)value;
    }
    else if(m_subtype_id == asTYPEID_INT32 ||
            m_subtype_id == asTYPEID_UINT32 ||
            m_subtype_id == asTYPEID_FLOAT ||
            m_subtype_id > asTYPEID_DOUBLE) // enums
    {
        *(int*)ptr = *(int*)value;
    }
    else if(m_subtype_id == asTYPEID_INT64 ||
            m_subtype_id == asTYPEID_UINT64 ||
            m_subtype_id == asTYPEID_DOUBLE)
    {
        *(double*)ptr = *(double*)value;
    }
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
    if(!(m_subtype_id & ~asTYPEID_MASK_SEQNBR))
    {
        switch(m_subtype_id)
        {
#define ASBIND20_EXT_ARRAY_EQUALS_IMPL(type) \
    *((const type*)lhs) == *((const type*)rhs)

#define ASBIND20_EXT_ARRAY_EQUALS_CASE_IMPL(as_type_id) \
    case as_type_id: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(primitive_type_of_t<as_type_id>)

            ASBIND20_EXT_ARRAY_EQUALS_CASE_IMPL(asTYPEID_BOOL);
            ASBIND20_EXT_ARRAY_EQUALS_CASE_IMPL(asTYPEID_INT8);
            ASBIND20_EXT_ARRAY_EQUALS_CASE_IMPL(asTYPEID_INT16);
            ASBIND20_EXT_ARRAY_EQUALS_CASE_IMPL(asTYPEID_INT32);
            ASBIND20_EXT_ARRAY_EQUALS_CASE_IMPL(asTYPEID_INT64);
            ASBIND20_EXT_ARRAY_EQUALS_CASE_IMPL(asTYPEID_UINT8);
            ASBIND20_EXT_ARRAY_EQUALS_CASE_IMPL(asTYPEID_UINT16);
            ASBIND20_EXT_ARRAY_EQUALS_CASE_IMPL(asTYPEID_UINT32);
            ASBIND20_EXT_ARRAY_EQUALS_CASE_IMPL(asTYPEID_UINT64);
            ASBIND20_EXT_ARRAY_EQUALS_CASE_IMPL(asTYPEID_FLOAT);
            ASBIND20_EXT_ARRAY_EQUALS_CASE_IMPL(asTYPEID_DOUBLE);

        default:
            return ASBIND20_EXT_ARRAY_EQUALS_IMPL(int); // enums

#undef ASBIND20_EXT_ARRAY_EQUALS_IMPL
        }
    }
    else
    {
        if(m_subtype_id & asTYPEID_OBJHANDLE)
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


    bool result = true;
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
            void* tmp = *(void**)ptr;
            *(void**)ptr = *(void**)value;
            m_ti->GetEngine()->AddRefScriptObject(*(void**)value, m_ti->GetSubType());
            if(tmp)
                m_ti->GetEngine()->ReleaseScriptObject(tmp, m_ti->GetSubType());
        }
        else if(m_subtype_id == asTYPEID_BOOL ||
                m_subtype_id == asTYPEID_INT8 ||
                m_subtype_id == asTYPEID_UINT8)
        {
            *(char*)ptr = *(char*)value;
        }
        else if(m_subtype_id == asTYPEID_INT16 ||
                m_subtype_id == asTYPEID_UINT16)
        {
            *(short*)ptr = *(short*)value;
        }
        else if(m_subtype_id == asTYPEID_INT32 ||
                m_subtype_id == asTYPEID_UINT32 ||
                m_subtype_id == asTYPEID_FLOAT ||
                m_subtype_id > asTYPEID_DOUBLE) // enums
        {
            *(int*)ptr = *(int*)value;
        }
        else if(m_subtype_id == asTYPEID_INT64 ||
                m_subtype_id == asTYPEID_UINT64 ||
                m_subtype_id == asTYPEID_DOUBLE)
        {
            *(double*)ptr = *(double*)value;
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

static bool check_array_template(asITypeInfo* ti, bool& no_gc)
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

static void const_array_for_each(asIScriptFunction* cb, const script_array& this_)
{
    asIScriptEngine* engine = this_.script_type_info()->GetEngine();

    reuse_active_context ctx(engine);
    this_.for_each(
        [=, &ctx](const void* ptr) -> void
        {
            asEContextState prev_state = ctx.get()->GetState();
            if(prev_state == asEXECUTION_EXCEPTION ||
               prev_state == asEXECUTION_ABORTED) [[unlikely]]
                return;
            script_invoke<void>(ctx, cb, ptr);
        }
    );
}

void register_script_array(asIScriptEngine* engine, bool as_default)
{
    ref_class<script_array>(engine, "array<T>", asOBJ_TEMPLATE | asOBJ_GC)
        .template_callback(&check_array_template)
        .template_default_factory()
        .factory<asITypeInfo*, asUINT>("array<T>@ f(int&in, uint)")
        .factory<asITypeInfo*, asUINT, const void*>("array<T>@ f(int&in, uint, const T&in)")
        .template_list_factory("T")
        .opAssign()
        .opEquals()
        .addref(&script_array::addref)
        .release(&script_array::release)
        .get_refcount(&script_array::get_refcount)
        .set_flag(&script_array::set_gc_flag)
        .get_flag(&script_array::get_gc_flag)
        .enum_refs(&script_array::enum_refs)
        .release_refs(&script_array::release_refs)
        .method("T& opIndex(uint idx)", &script_array::opIndex)
        .method("const T& opIndex(uint idx) const", &script_array::opIndex)
        .method("T& get_front() property", &script_array::get_front)
        .method("const T& get_front() const property", &script_array::get_front)
        .method("void set_front(const T&in) property", &script_array::set_front)
        .method("T& get_back() property", &script_array::get_back)
        .method("const T& get_back() const property", &script_array::get_back)
        .method("void set_back(const T&in) property", &script_array::set_back)
        .method("uint get_size() const property", &script_array::size)
        .method("uint get_capacity() const property", &script_array::capacity)
        .method("bool get_empty() const property", &script_array::empty)
        .method("void reserve(uint)", &script_array::reserve)
        .method("void shrink_to_fit()", &script_array::shrink_to_fit)
        .method("void push_back(const T&in value)", &script_array::push_back)
        .method("void append_range(const array<T>&in rng, uint n=-1)", &script_array::append_range)
        .method("void insert(uint idx, const T&in value)", &script_array::insert)
        .method("void insert_range(uint idx, const array<T>&in rng, uint n=-1)", &script_array::insert_range)
        .method("void pop_back()", &script_array::pop_back)
        .method("void erase(uint idx, uint n=1)", &script_array::erase)
        .funcdef("void const_for_each_callback(const T&in if_handle_then_const)")
        .method("void for_each(const const_for_each_callback&in cb) const", &const_array_for_each)
        .method("void sort(uint idx=0, uint n=-1, bool asc=true)", &script_array::sort)
        .method("void clear()", &script_array::clear);

    engine->SetTypeInfoUserDataCleanupCallback(
        &script_array::cache_cleanup_callback,
        script_array_cache_id()
    );

    if(as_default)
    {
        int r = engine->RegisterDefaultArrayType("array<T>");
        assert(r >= 0);
    }
}
} // namespace asbind20::ext
