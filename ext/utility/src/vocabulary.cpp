#include <asbind20/ext/vocabulary.hpp>

namespace asbind20::ext
{
static bool optional_template_callback(asITypeInfo* ti, bool&)
{
    int subtype_id = ti->GetSubTypeId();
    if(subtype_id == asTYPEID_VOID)
        return false;

    return true;
}

script_optional::script_optional(asITypeInfo* ti)
    : m_ti(ti)
{
    m_ti->AddRef();
    assert(m_has_value == false);
}

script_optional::script_optional(asITypeInfo* ti, const void* val)
    : m_ti(ti)
{
    m_ti->AddRef();
    assign(val);
}

script_optional::~script_optional()
{
    reset();
    m_ti->Release();
}

script_optional& script_optional::operator=(const script_optional& other)
{
    if(&other == this)
        return *this;

    assert(m_ti == other.m_ti);
    if(other)
        assign(other.m_data.get(other.m_ti));
    else
        reset();

    return *this;
}

void script_optional::assign(const void* val)
{
    if(has_value())
        reset();

    assert(m_ti);
    assert(val != nullptr);

    m_has_value = m_data.copy_construct(m_ti, val);
}

void* script_optional::operator new(std::size_t bytes)
{
    return as_allocator<std::byte>::allocate(bytes);
}

void script_optional::operator delete(void* p)
{
    as_allocator<std::byte>::deallocate(
        static_cast<std::byte*>(p), sizeof(script_optional)
    );
}

void* script_optional::value()
{
    if(!has_value())
    {
        set_script_exception("bad optional access");
        return nullptr;
    }

    return m_data.get(m_ti);
}

const void* script_optional::value() const
{
    if(!has_value())
    {
        set_script_exception("bad optional access");
        return nullptr;
    }

    return m_data.get(m_ti);
}

void* script_optional::value_or(void* val)
{
    assert(val != nullptr);

    return m_has_value ? m_data.get(m_ti) : val;
}

const void* script_optional::value_or(const void* val) const
{
    assert(val != nullptr);

    return m_has_value ? m_data.get(m_ti) : val;
}

void script_optional::reset()
{
    if(!m_has_value)
        return;

    m_has_value = false;
    m_data.destruct(m_ti);
}

bool script_optional::data_t::use_storage(asITypeInfo* ti)
{
    int subtype_id = ti->GetSubTypeId();
    if(!(subtype_id & ~asTYPEID_MASK_SEQNBR))
    {
        return true;
    }
    else if(subtype_id & asTYPEID_OBJHANDLE)
        return true;

    return false;
}

bool script_optional::data_t::copy_construct(asITypeInfo* ti, const void* val)
{
    int subtype_id = ti->GetSubTypeId();
    if(!(subtype_id & ~asTYPEID_MASK_SEQNBR))
    {
        visit_primitive_type(
            [this]<typename T>(const T* val)
            {
                static_assert(sizeof(T) <= sizeof(data_t::storage));
                *(T*)storage = *val;
            },
            subtype_id,
            val
        );
    }
    else if(subtype_id & asTYPEID_OBJHANDLE)
    {
        void** obj = (void**)storage;
        *obj = (void*)val;
        if(*obj)
        {
            asIScriptEngine* engine = ti->GetEngine();
            engine->AddRefScriptObject(*obj, ti->GetSubType());
        }
    }
    else
    {
        asIScriptEngine* engine = ti->GetEngine();

        ptr = engine->CreateScriptObjectCopy((void*)val, ti->GetSubType());
        if(!ptr)
            return false;
    }

    return true;
}

void script_optional::data_t::destruct(asITypeInfo* ti)
{
    int subtype_id = ti->GetSubTypeId();
    if(!(subtype_id & ~asTYPEID_MASK_SEQNBR))
        return;
    else if(subtype_id & asTYPEID_OBJHANDLE)
    {
        void* obj = (void**)storage;
        if(obj)
        {
            asIScriptEngine* engine = ti->GetEngine();
            engine->ReleaseScriptObject(obj, ti->GetSubType());
        }
    }
    else
    {
        asIScriptEngine* engine = ti->GetEngine();
        engine->ReleaseScriptObject(ptr, ti);
    }
}

void* script_optional::data_t::get(asITypeInfo* ti)
{
    if(use_storage(ti))
        return storage;
    else
        return ptr;
}

const void* script_optional::data_t::get(asITypeInfo* ti) const
{
    if(use_storage(ti))
        return storage;
    else
        return ptr;
}

void script_optional::release()
{
    delete this;
}

static void* optional_value(script_optional& this_)
{
    return this_.value();
}

static void* optional_value_or(script_optional& this_, void* val)
{
    return this_.value_or(val);
}

static script_optional& optional_opAssign_value(const void* val, script_optional& this_)
{
    this_.assign(val);
    return this_;
}

namespace detail
{
    template <bool UseGeneric>
    void register_script_optional_impl(asIScriptEngine* engine)
    {
        template_class<script_optional, UseGeneric> c(engine, "optional<T>", asOBJ_SCOPED);
        c
            .template_callback(fp<&optional_template_callback>)
            .default_factory()
            .template factory<const void*>("const T&in")
            .release(fp<&script_optional::release>)
            .opAssign()
            .method("optional<T>& opAssign(const T&in)", fp<&optional_opAssign_value>)
            .method("bool get_has_value() const property", fp<&script_optional::has_value>)
            .method("bool opConv() const", fp<&script_optional::has_value>)
            .method("void reset()", fp<&script_optional::reset>)
            .method("T& get_value() property", fp<&optional_value>)
            .method("const T& get_value() const property", fp<&optional_value>)
            .method("void set_value(const T&in) property", fp<&script_optional::assign>)
            .method("T& value_or(T&in val)", fp<&optional_value_or>)
            .method("const T& value_or(const T&in val) const", fp<&optional_value_or>);
    }
} // namespace detail

void register_script_optional(asIScriptEngine* engine, bool generic)
{
    if(generic)
        detail::register_script_optional_impl<true>(engine);
    else
        detail::register_script_optional_impl<false>(engine);
}
} // namespace asbind20::ext
