#include <asbind20/ext/vocabulary.hpp>

namespace asbind20::ext
{
static bool optional_template_callback(asITypeInfo* ti, bool& no_gc)
{
    int subtype_id = ti->GetSubTypeId();
    if(subtype_id == AS_NAMESPACE_QUALIFIER asTYPEID_VOID)
        return false;

    if(is_primitive_type(subtype_id))
        no_gc = true;
    else
    {
        asQWORD subtype_flags = ti->GetSubType()->GetFlags();
        if(subtype_flags & AS_NAMESPACE_QUALIFIER asOBJ_POD)
            no_gc = true;
        else if((subtype_flags & AS_NAMESPACE_QUALIFIER asOBJ_REF))
        {
            if(subtype_flags & AS_NAMESPACE_QUALIFIER asOBJ_NOCOUNT)
                no_gc = true;
            else
                no_gc = false;
        }
        else if((subtype_flags & AS_NAMESPACE_QUALIFIER asOBJ_VALUE))
        {
            if(!(subtype_flags & AS_NAMESPACE_QUALIFIER asOBJ_GC))
                no_gc = true;
            else
                no_gc = false;
        }
        else
            no_gc = false;
    }

    return true;
}

script_optional::script_optional(asITypeInfo* ti)
    : m_ti(ti)
{
    m_ti->AddRef();
    assert(m_has_value == false);
}

script_optional::script_optional(asITypeInfo* ti, const script_optional& other)
    : m_ti(ti)
{
    m_ti->AddRef();
    assert(m_ti == other.m_ti);
    if(other.has_value())
        assign(other.m_data.data_address(other.subtype_id()));
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
        assign(other.m_data.data_address(other.subtype_id()));
    else
        reset();

    return *this;
}

void script_optional::assign(const void* val)
{
    assert(m_ti != nullptr);

    if(has_value())
    {
        m_data.copy_assign_from(m_ti->GetEngine(), subtype_id(), val);
    }
    else
    {
        m_data.copy_construct(m_ti->GetEngine(), subtype_id(), val);
        m_has_value = true;
    }
}

void* script_optional::value()
{
    if(!has_value())
    {
        set_script_exception("bad optional access");
        return nullptr;
    }

    return m_data.data_address(subtype_id());
}

const void* script_optional::value() const
{
    if(!has_value())
    {
        set_script_exception("bad optional access");
        return nullptr;
    }

    return m_data.data_address(subtype_id());
}

void* script_optional::value_or(void* val)
{
    assert(val != nullptr);

    return m_has_value ? m_data.data_address(subtype_id()) : val;
}

const void* script_optional::value_or(const void* val) const
{
    assert(val != nullptr);

    return m_has_value ? m_data.data_address(subtype_id()) : val;
}

void script_optional::reset()
{
    if(!m_has_value)
        return;

    m_data.destroy(m_ti->GetEngine(), subtype_id());
    m_has_value = false;
}

void script_optional::enum_refs(asIScriptEngine* engine)
{
    if(m_ti->GetFlags() & AS_NAMESPACE_QUALIFIER asOBJ_GC)
    {
        AS_NAMESPACE_QUALIFIER asITypeInfo* subtype = m_ti->GetSubType();
        engine->ForwardGCEnumReferences(
            m_data.object_ref(), subtype
        );
    }
}

void script_optional::release_refs(asIScriptEngine* engine)
{
    (void)engine;
    assert(engine == m_ti->GetEngine());
    reset();
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
        constexpr AS_NAMESPACE_QUALIFIER asQWORD flags =
            AS_NAMESPACE_QUALIFIER asOBJ_GC |
            AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_CDAK |
            AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS |
            AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS;
        template_value_class<script_optional, UseGeneric> c(
            engine,
            "optional<T>",
            flags
        );

#define ASBIND20_EXT_OPTIONAL_METHOD(name, ret, args) \
        static_cast<ret(script_optional::*) args>(&script_optional::name)

        c
            .template_callback(fp<&optional_template_callback>)
            .default_constructor()
            .copy_constructor()
            .template constructor<const void*>("const T&in")
            .destructor()
            .opAssign()
            .enum_refs(fp<&script_optional::enum_refs>)
            .release_refs(fp<&script_optional::release_refs>)
            .method(
                "optional<T>& opAssign(const T&in if_handle_then_const)",
                [](const void* val, script_optional& this_) -> script_optional&
                {
                    this_.assign(val);
                    return this_;
                }
            )
            .method("bool get_has_value() const property", fp<&script_optional::has_value>)
            .method("bool opConv() const", fp<&script_optional::has_value>)
            .method("void reset()", fp<&script_optional::reset>)
            .method("T& get_value() property", fp<ASBIND20_EXT_OPTIONAL_METHOD(value, void*, ())>)
            .method("const T& get_value() const property", fp<ASBIND20_EXT_OPTIONAL_METHOD(value, void*, ())>)
            .method("void set_value(const T&in) property", fp<&script_optional::assign>)
            .method("T& value_or(T&in val)", fp<ASBIND20_EXT_OPTIONAL_METHOD(value_or, void*, (void*))>)
            .method("const T& value_or(const T&in val) const", fp<ASBIND20_EXT_OPTIONAL_METHOD(value_or, void*, (void*))>);

#undef ASBIND20_EXT_OPTIONAL_METHOD
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
