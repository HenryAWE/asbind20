#include <asbind20/ext/vocabulary.hpp>

namespace asbind20::ext
{
static bool optional_template_callback(
    AS_NAMESPACE_QUALIFIER asITypeInfo* ti, bool& no_gc
)
{
    int subtype_id = ti->GetSubTypeId();
    if(is_void_type(subtype_id))
        return false;

    if(is_primitive_type(subtype_id))
        no_gc = true;
    else
        no_gc = !type_requires_gc(ti->GetSubType());

    return true;
}

script_optional::script_optional(const script_optional& other)
    : m_ti(other.m_ti)
{
    if(other.has_value())
    {
        assign(other.m_data.data_address(other.element_type_id()));
    }
}

script_optional::script_optional(
    AS_NAMESPACE_QUALIFIER asITypeInfo* ti
)
    : m_ti(ti)
{
    m_data.construct(m_ti->GetEngine(), element_type_id());
    m_has_value = true;
}

script_optional::script_optional(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, std::nullopt_t)
    : m_ti(ti)
{
    assert(ti);
    assert(m_has_value == false);
}

script_optional::script_optional(
    AS_NAMESPACE_QUALIFIER asITypeInfo* ti, const script_optional& other
)
    : script_optional(other)
{
    (void)ti;
    assert(m_ti == ti);
}

script_optional::script_optional(
    AS_NAMESPACE_QUALIFIER asITypeInfo* ti, const void* val
)
    : m_ti(ti)
{
    assign(val);
}

script_optional::~script_optional()
{
    reset();
}

script_optional& script_optional::operator=(const script_optional& other)
{
    if(&other == this)
        return *this;

    assert(m_ti == other.m_ti);
    if(other)
        assign(other.m_data.data_address(other.element_type_id()));
    else
        reset();

    return *this;
}

void script_optional::emplace()
{
    assert(m_ti != nullptr);
    reset();
    m_data.construct(m_ti->GetEngine(), element_type_id());
    m_has_value = true;
}

void script_optional::assign(const void* val)
{
    assert(m_ti != nullptr);

    if(has_value())
    {
        m_data.copy_assign_from(m_ti->GetEngine(), element_type_id(), val);
    }
    else
    {
        m_data.copy_construct(m_ti->GetEngine(), element_type_id(), val);
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

    return m_data.data_address(element_type_id());
}

const void* script_optional::value() const
{
    if(!has_value())
    {
        set_script_exception("bad optional access");
        return nullptr;
    }

    return m_data.data_address(element_type_id());
}

void* script_optional::value_or(void* val)
{
    assert(val != nullptr);

    return m_has_value ? m_data.data_address(element_type_id()) : val;
}

const void* script_optional::value_or(const void* val) const
{
    assert(val != nullptr);

    return m_has_value ? m_data.data_address(element_type_id()) : val;
}

void script_optional::reset()
{
    if(!m_has_value)
        return;

    m_data.destroy(m_ti->GetEngine(), element_type_id());
    m_has_value = false;
}

void script_optional::enum_refs(asIScriptEngine* engine)
{
    assert(m_ti->GetFlags() & AS_NAMESPACE_QUALIFIER asOBJ_GC);

    m_data.enum_refs(m_ti->GetSubType());
}

void script_optional::release_refs(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    (void)engine;
    assert(engine == m_ti->GetEngine());
    reset();
}

void register_script_optional(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool use_generic
)
{
    auto helper = [engine]<bool UseGeneric>(std::bool_constant<UseGeneric>)
    {
        value_class<std::nullopt_t, true>(
            engine, "nullopt_t", AS_NAMESPACE_QUALIFIER asOBJ_POD
        );

        global<true>(engine)
            .property("const nullopt_t nullopt", std::nullopt);

        constexpr AS_NAMESPACE_QUALIFIER asQWORD flags =
            AS_NAMESPACE_QUALIFIER asOBJ_GC |
            AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_CDAK |
            AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS;
        template_value_class<script_optional, UseGeneric> c(
            engine,
            "optional<T>",
            flags
        );

        c
            .template_callback(fp<&optional_template_callback>)
            .default_constructor()
            .copy_constructor()
            .template constructor<const void*>("const T&in")
            .constructor_function(
                "const nullopt_t&in",
                [](void* mem, AS_NAMESPACE_QUALIFIER asITypeInfo* ti, const void*)
                {
                    new(mem) script_optional(ti, std::nullopt);
                },
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
            )
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
            .property("const bool has_value", &script_optional::m_has_value)
            .template opConv<bool>()
            .method("void reset()", fp<&script_optional::reset>)
            .method("T& get_value() property", fp<overload_cast<>(&script_optional::value)>)
            .method("const T& get_value() const property", fp<overload_cast<>(&script_optional::value, const_)>)
            .method("void set_value(const T&in) property", fp<&script_optional::assign>)
            .method("T& value_or(T&in val)", fp<overload_cast<void*>(&script_optional::value_or)>)
            .method("const T& value_or(const T&in val) const", fp<overload_cast<const void*>(&script_optional::value_or, const_)>);
    };

    if(use_generic)
        helper(std::true_type{});
    else
        helper(std::false_type{});
}
} // namespace asbind20::ext
