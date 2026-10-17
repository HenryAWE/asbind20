/**
 * @file shared_test_lib/src/weakref.cpp
 * @brief Weak reference support for script objects in tests
 *
 * Ported from the removed extension library (asbind20 v1.8):
 * `asbind20/ext/weakref.cpp`.
 *
 * Note: The old `weakref_template_callback` was defined but never registered,
 * so it's dropped here to keep the historical behavior (any subtype is
 * accepted by the template).
 */

#include <asbind_test/weakref.hpp>
#include <mutex>

namespace asbind_test
{
script_weakref::script_weakref(const script_weakref&) = default;

script_weakref::script_weakref(asbind20::typeinfo_pointer ti)
    : m_ref(nullptr), m_ti(ti), m_flag()
{}

script_weakref::script_weakref(asbind20::typeinfo_pointer ti, void* ref)
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
static void register_weakref_impl(asbind20::engine_pointer engine)
{
    using namespace asbind20;

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

void register_weakref(asbind20::engine_pointer engine, bool use_generic)
{
    if(use_generic)
        register_weakref_impl<true>(engine);
    else
        register_weakref_impl<false>(engine);
}

void register_weakref(const asbind20::unique_script_engine& engine, bool use_generic)
{
    register_weakref(engine.get(), use_generic);
}
} // namespace asbind_test
