#include <asbind20/ext/array.hpp>
#include <cassert>
#include <cstddef>
#include <cstring>
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

script_array::script_array(asITypeInfo* ti)
    : m_ti(ti), m_subtype_id(ti->GetSubTypeId())
{
    asIScriptEngine* engine = ti->GetEngine();

    m_ti->AddRef();
    m_elem_size = get_elem_size(engine, m_subtype_id);

    cache_data();

    if(m_ti->GetFlags() & asOBJ_GC)
        engine->NotifyGarbageCollectorOfNewObject(this, m_ti);
}

script_array::script_array(const script_array& other)
    : m_ti(other.m_ti), m_subtype_id(other.m_subtype_id), m_elem_size(other.m_elem_size)
{
    asIScriptEngine* engine = m_ti->GetEngine();

    m_ti->AddRef();
    m_elem_size = get_elem_size(engine, m_subtype_id);

    if(m_ti->GetFlags() & asOBJ_GC)
    {
        m_ti->GetEngine()->NotifyGarbageCollectorOfNewObject(this, m_ti);
    }
}

script_array::script_array(asITypeInfo* ti, void* list_buf)
    : m_ti(ti), m_subtype_id(ti->GetSubTypeId())
{
    asIScriptEngine* engine = ti->GetEngine();

    m_ti->AddRef();
    m_elem_size = get_elem_size(engine, m_subtype_id);

    asUINT buf_sz = *static_cast<asUINT*>(list_buf);
    list_buf = static_cast<asUINT*>(list_buf) + 1;

    cache_data();

    alloc_data(buf_sz);
    if(!(m_subtype_id & asTYPEID_MASK_OBJECT))
    {
        m_data.size = buf_sz;
        if(buf_sz > 0)
            std::memcpy(m_data.ptr, list_buf, buf_sz * m_elem_size);
    }
    else if((m_subtype_id & asTYPEID_OBJHANDLE) || (m_subtype_id & asOBJ_REF))
    {
        m_data.size = buf_sz;
        if(buf_sz > 0)
        {
            std::memcpy(m_data.ptr, list_buf, buf_sz * m_elem_size);
            std::memset(list_buf, 0, buf_sz * m_elem_size);
        }
    }
    else
    {
        assert((m_subtype_id & asTYPEID_MASK_OBJECT) && !(m_subtype_id & asTYPEID_OBJHANDLE));

        asITypeInfo* subtype_ti = engine->GetTypeInfoById(m_subtype_id);
        assert(subtype_ti == m_ti->GetSubType());

        bool is_script_obj = subtype_ti->GetFlags() & asOBJ_SCRIPT_OBJECT;
        size_type list_elem_size = is_script_obj ?
                                       sizeof(void*) :
                                       subtype_ti->GetSize();
        for(size_type i = 0; i < buf_sz; ++i)
        {
            assert(m_elem_size == sizeof(void*));
            void** dst = (void**)(m_data.ptr + i * m_elem_size);
            void* src = is_script_obj ?
                            *(void**)(static_cast<std::byte*>(list_buf) + i * list_elem_size) :
                            static_cast<std::byte*>(list_buf) + i * list_elem_size;

            *dst = engine->CreateScriptObjectCopy(src, subtype_ti);
        }

        m_data.size = buf_sz;
    }

    if(m_ti->GetFlags() & asOBJ_GC)
        m_ti->GetEngine()->NotifyGarbageCollectorOfNewObject(this, m_ti);
}

script_array::~script_array()
{
    deallocate(m_data.ptr);
    if(m_ti)
        m_ti->Release();
}

void script_array::addref()
{
    ++m_refcount;
}

void script_array::release()
{
    assert(m_refcount != 0);
    --m_refcount;
    if(m_refcount == 0)
        delete this;
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

    mem_grow_to(new_cap);
}

void script_array::push_back(void* value)
{
    reserve(size() + 1);

    void* ptr = m_data.ptr + m_data.size * m_elem_size;

    if((m_subtype_id & ~asTYPEID_MASK_SEQNBR) && !(m_subtype_id & asTYPEID_OBJHANDLE))
    {
        void** dst = static_cast<void**>(ptr);
        *dst = m_ti->GetEngine()->CreateScriptObjectCopy(value, m_ti->GetSubType());
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

    ++m_data.size;

    return;
}

void script_array::move_data(array_data& src, array_data& dst)
{
    asIScriptEngine* engine = m_ti->GetEngine();
    if(m_subtype_id & asTYPEID_OBJHANDLE)
    {
        assert(dst.capacity >= src.size);

        void** sentinel = (void**)(dst.ptr + src.size);
        void** p = (void**)src.ptr;
        void** q = (void**)dst.ptr;

        while(q < sentinel)
        {
            assert(*q == nullptr);
            *q = *p;
            if(*q)
                engine->AddRefScriptObject(*q, m_ti->GetSubType());

            ++p;
            ++q;
        }
    }
    else if(m_subtype_id & asTYPEID_MASK_OBJECT)
    {
        void** sentinel = (void**)dst.ptr + src.size;
        void** p = (void**)src.ptr;
        void** q = (void**)dst.ptr;

        asITypeInfo* subtype_ti = m_ti->GetSubType();
        while(q < sentinel)
        {
            *q = engine->CreateScriptObjectCopy(*p, subtype_ti);
            engine->ReleaseScriptObject(*p, subtype_ti);

            ++p;
            ++q;
        }
    }
    else // Primitive types
    {
        std::memcpy(dst.ptr, src.ptr, src.size * m_elem_size);
    }

    dst.size = src.size;
}

void script_array::assign_impl(void* ptr, void* value)
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

bool script_array::equals_impl(const void* lhs, const void* rhs, asIScriptContext* ctx, const array_cache* cache) const
{
    if(!(m_subtype_id & ~asTYPEID_MASK_SEQNBR))
    {
        switch(m_subtype_id)
        {
#define ASBIND20_EXT_ARRAY_EQUALS_IMPL(type) *((const type*)lhs) == *((const type*)rhs)
        case asTYPEID_BOOL: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(bool);
        case asTYPEID_INT8: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(asINT8);
        case asTYPEID_INT16: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(asINT16);
        case asTYPEID_INT32: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(asINT32);
        case asTYPEID_INT64: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(asINT64);
        case asTYPEID_UINT8: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(asBYTE);
        case asTYPEID_UINT16: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(asWORD);
        case asTYPEID_UINT32: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(asDWORD);
        case asTYPEID_UINT64: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(asQWORD);
        case asTYPEID_FLOAT: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(float);
        case asTYPEID_DOUBLE: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(double);
        default: return ASBIND20_EXT_ARRAY_EQUALS_IMPL(int); // enums
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

        if(!cache)
            return false;

        if(cache->subtype_opEquals)
        {
            auto result = script_invoke<bool>(
                ctx,
                static_cast<const asIScriptObject*>(*((void**)const_cast<void*>(lhs))),
                cache->subtype_opEquals,
                rhs
            );

            if(!result.has_value())
                return false;
            return *result;
        }

        // Fallback to OpCmp() == 0
        if(cache->subtype_opCmp)
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
                    cache->subtype_opCmp = func;
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
}

void script_array::release_refs(asIScriptEngine* engine)
{
}

void* script_array::operator[](asUINT idx)
{
    assert(idx < size());
    std::size_t offset = idx * m_elem_size;
    return m_data.ptr + offset;
}

const void* script_array::operator[](asUINT idx) const
{
    assert(idx < size());
    std::size_t offset = idx * m_elem_size;
    return m_data.ptr + offset;
}

void* script_array::ref_at(size_type idx)
{
    if((m_subtype_id & asTYPEID_MASK_OBJECT) && !(m_subtype_id & asTYPEID_OBJHANDLE))
        return *(void**)(*this)[idx];
    else
        return (*this)[idx];
}

void* script_array::opIndex(asUINT idx)
{
    if(idx >= size())
    {
        set_script_exception("Array index out of range");
        return nullptr;
    }

    return ref_at(idx);
}

bool script_array::operator==(const script_array& other) const
{
    if(m_ti != other.m_ti)
        return false;
    if(size() != other.size())
        return false;

    assert(m_elem_size == other.m_elem_size);

    asIScriptContext* ctx = nullptr;
    bool ctx_is_nested = false;
    if(m_subtype_id & ~asTYPEID_MASK_SEQNBR)
    {
        ctx = asGetActiveContext();
        if(ctx->GetEngine() == m_ti->GetEngine() && ctx->PushState() >= 0)
            ctx_is_nested = true;
        else
            ctx = nullptr;

        if(!ctx)
            ctx = m_ti->GetEngine()->CreateContext();
    }

    bool result = true;
    array_cache* cache = reinterpret_cast<array_cache*>(m_ti->GetUserData(script_array_cache_id()));
    for(size_type i = 0; i < size(); ++i)
    {
        if(!equals_impl((*this)[i], other[i], ctx, cache))
        {
            result = false;
            break;
        }
    }

    if(ctx)
    {
        if(ctx_is_nested)
        {
            asEContextState state = ctx->GetState();
            ctx->PopState();
            if(state == asEXECUTION_ABORTED)
                ctx->Abort();
        }
        else
            ctx->Release();
    }

    return result;
}

void* script_array::get_front()
{
    if(empty())
    {
        set_script_exception("Empty array");
    }
    return ref_at(0);
}

void* script_array::get_back()
{
    if(empty())
    {
        set_script_exception("Empty array");
    }

    return ref_at(size() - 1);
}

asQWORD script_array::subtype_flags() const
{
    return m_ti->GetSubType()->GetFlags();
}

void* script_array::allocate(std::size_t bytes)
{
    return asAllocMem(bytes);
}

void script_array::deallocate(void* ptr) noexcept
{
    asFreeMem(ptr);
}

void script_array::alloc_data(size_type cap)
{
    assert(m_data.ptr == nullptr);
    assert(size() == 0);

    if(cap == 0)
        return;

    m_data.ptr = (std::byte*)allocate(cap * m_elem_size);
    m_data.capacity = cap;
}

void script_array::construct_elem(void* ptr, size_type n)
{
    if((m_subtype_id & asTYPEID_MASK_OBJECT) && !(m_subtype_id & asTYPEID_OBJHANDLE))
    {
        void** sentinel = (void**)(static_cast<std::byte*>(ptr) + n * sizeof(void*));

        asIScriptEngine* engine = m_ti->GetEngine();
        asITypeInfo* subtype_ti = m_ti->GetSubType();

        for(void** p = static_cast<void**>(ptr); p < sentinel; ++p)
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
        std::memset(ptr, 0, n * m_elem_size);
    }
}

bool script_array::mem_grow_to(size_type new_cap)
{
    assert(new_cap > capacity());

    array_data tmp;
    tmp.capacity = new_cap;
    tmp.size = 0;
    tmp.ptr = (std::byte*)allocate(new_cap * m_elem_size);
    if(!tmp.ptr)
    {
        set_script_exception("out of memory");
        return false;
    }
    std::memset(tmp.ptr, 0, new_cap * m_elem_size);

    move_data(m_data, tmp);
    deallocate(m_data.ptr);
    m_data = tmp;

    return true;
}

void script_array::copy_other_impl(script_array& other)
{
    assert(m_ti == other.m_ti);
    assert(m_elem_size == other.m_elem_size);
}

static bool check_array_template(asITypeInfo* ti, bool& no_gc)
{
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

void register_script_array(asIScriptEngine* engine, bool as_default)
{
    ref_class<script_array>(engine, "array<T>", asOBJ_TEMPLATE | asOBJ_GC)
        .template_(&check_array_template)
        .default_factory()
        .list_factory("T")
        .addref(&script_array::addref)
        .release(&script_array::release)
        .get_refcount(&script_array::get_refcount)
        .set_flag(&script_array::set_gc_flag)
        .get_flag(&script_array::get_gc_flag)
        .enum_refs(&script_array::enum_refs)
        .release_refs(&script_array::release_refs)
        .method("bool opEquals(const array<T>&in) const", &script_array::operator==)
        .method("T& opIndex(uint idx)", &script_array::opIndex)
        .method("const T& opIndex(uint idx) const", &script_array::opIndex)
        .method("T& get_front() property", &script_array::get_front)
        .method("const T& get_front() const property", &script_array::get_front)
        .method("T& get_back() property", &script_array::get_back)
        .method("const T& get_back() const property", &script_array::get_back)
        .method("uint get_size() const property", &script_array::size)
        .method("uint get_capacity() const property", &script_array::capacity)
        .method("bool get_empty() const property", &script_array::empty)
        .method("void push_back(const T&in)", &script_array::push_back);

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
