#include <asbind20/ext/weakref.hpp>

namespace asbind20::ext
{
static bool weakref_template_callback(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, bool&)
{
    auto* subtype = ti->GetSubType();
    if(!subtype) // primitive types
        return false;
    if(!(subtype->GetFlags() & AS_NAMESPACE_QUALIFIER asOBJ_REF))
        return false; // reference type only

    if(get_weakref_flag(subtype))
        return true;

    std::string msg = string_concat("The subtype \"", ti->GetName(), "\" doesn't support weak references");
    ti->GetEngine()->WriteMessage(
        "weakref", 0, 0, AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR, msg.c_str()
    );
    return false;
}

script_weakref::script_weakref(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
    : m_ref(nullptr), m_ti(ti), m_flag()
{}

script_weakref::script_weakref(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, void* ref)
    : m_ref(ref), m_ti(ti), m_flag()
{
    m_flag.connect_object(ref, ti->GetSubType());
}

// Those RAII helpers should be able to handle destruction for us
script_weakref::~script_weakref() = default;

script_weakref& script_weakref::operator=(const script_weakref& other)
{
    // No self-assignment
    if(m_ref == other.m_ref && m_flag == other.m_flag)
        return *this;

    assert(this->referenced_type() == other.referenced_type());

    m_ref = other.m_ref;
    m_flag = other.m_flag;

    return *this;
}

bool script_weakref::operator==(const script_weakref& rhs) const
{
    assert(this->referenced_type() == rhs.referenced_type());
    return m_ref == rhs.m_ref && m_flag == rhs.m_flag;
}

void script_weakref::reset(void* ref)
{
    // Retrieve the new weak ref
    m_ref = ref;
    if(ref)
    {
        m_flag.connect_object(ref, referenced_type());

        // Release the ref since we're only supposed to hold a weakref
        m_ti->GetEngine()->ReleaseScriptObject(ref, referenced_type());
    }
    else
        m_flag.reset();
}

void* script_weakref::get()
{
    if(!m_ref || !m_flag)
        return nullptr;

    std::lock_guard guard(m_flag);
    if(!m_flag.get_flag())
    {
        m_ti->GetEngine()->AddRefScriptObject(m_ref, referenced_type());
        return m_ref;
    }

    return nullptr;
}

bool script_weakref::equals_to(void* ref) const
{
    if(m_ref != ref)
        return false;

    auto* flag = m_ti->GetEngine()->GetWeakRefFlagOfScriptObject(
        ref, referenced_type()
    );
    return m_flag == flag;
}

template <bool UseGeneric>
static void register_weakref_impl(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    constexpr AS_NAMESPACE_QUALIFIER asQWORD flags =
        AS_NAMESPACE_QUALIFIER asOBJ_ASHANDLE |
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_DAK;

    auto reset_wrapper = [](script_weakref& this_, void* ref) -> script_weakref&
    {
        this_.reset(ref);
        return this_;
    };

    template_value_class<script_weakref, UseGeneric> c1(engine, "weakref<T>", flags);
    c1
        .default_constructor()
        .template constructor<void*>("T@+")
        .destructor()
        .method("T@ opImplCast()", fp<&script_weakref::get>)
        .method("T@ get() const", fp<&script_weakref::get>)
        .method("weakref<T>& opHndlAssign(const weakref<T>&in)", fp<(&script_weakref::operator=)>)
        .opAssign()
        .opEquals()
        .method("weakref<T>& opHndlAssign(T@)", reset_wrapper)
        .method("bool opEquals(const T@+) const", fp<&script_weakref::equals_to>);

    template_value_class<script_weakref, UseGeneric> c2(engine, "const_weakref<T>", flags);
    c2
        .default_constructor()
        .template constructor<void*>("T@+")
        .destructor()
        .method("const T@ opImplCast() const", fp<&script_weakref::get>)
        .method("const T@ get() const", fp<&script_weakref::get>)
        .method("const_weakref<T>& opHndlAssign(const const_weakref<T>&in)", fp<(&script_weakref::operator=)>)
        .opAssign()
        .opEquals()
        .method("weakref<T>& opHndlAssign(const T@)", reset_wrapper)
        .method("bool opEquals(const T@+) const", fp<&script_weakref::equals_to>);


    // Interoperability
    c2
        .method("const_weakref<T> &opHndlAssign(const weakref<T>&in)", fp<(&script_weakref::operator=)>)
        .method("bool opEquals(const weakref<T>&in) const", fp<(&script_weakref::operator==)>);
}

void register_weakref(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, bool use_generic)
{
    if(use_generic)
        register_weakref_impl<true>(engine);
    else
        register_weakref_impl<false>(engine);
}
} // namespace asbind20::ext
