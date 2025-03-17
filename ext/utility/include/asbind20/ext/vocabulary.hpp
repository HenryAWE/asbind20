#ifndef ASBIND20_EXT_VOCABULARY_HPP
#define ASBIND20_EXT_VOCABULARY_HPP

#pragma once

#include <asbind20/asbind.hpp>
#include <optional>

namespace asbind20::ext
{
void register_script_optional(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool use_generic = has_max_portability()
);

class script_optional
{
    friend void register_script_optional(AS_NAMESPACE_QUALIFIER asIScriptEngine*, bool);

public:
    script_optional() = delete;

    script_optional(const script_optional& other);

    script_optional(AS_NAMESPACE_QUALIFIER asITypeInfo* ti);

    script_optional(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, std::nullopt_t);

    script_optional(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, const script_optional& other);

    script_optional(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, const void* value);

    ~script_optional();

    script_optional& operator=(const script_optional& other);

    bool operator==(const script_optional& rhs) const;

    void emplace();
    void assign(const void* val);

    [[nodiscard]]
    bool has_value() const noexcept
    {
        return m_has_value;
    }

    void* value();
    const void* value() const;

    void* value_or(void* val);
    const void* value_or(const void* val) const;

    void reset();

    explicit operator bool() const
    {
        return has_value();
    }

    [[nodiscard]]
    auto get_type_info() const
        -> AS_NAMESPACE_QUALIFIER asITypeInfo*
    {
        return m_ti.get();
    }

    void enum_refs(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine);
    void release_refs(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine);

    [[nodiscard]]
    int element_type_id() const
    {
        return m_ti->GetSubTypeId();
    }

private:
    script_typeinfo m_ti;

    container::single m_data;
    bool m_has_value = false;
};

inline script_optional make_script_optional(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    std::string_view elem_decl
)
{
    auto* ti = engine->GetTypeInfoByDecl(
        string_concat("optional<", elem_decl, '>').c_str()
    );
    return script_optional(ti);
}

inline script_optional make_script_optional(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    std::string_view elem_decl,
    const void* ref
)
{
    auto* ti = engine->GetTypeInfoByDecl(
        string_concat("optional<", elem_decl, '>').c_str()
    );
    return script_optional(ti, ref);
}
} // namespace asbind20::ext

#endif
