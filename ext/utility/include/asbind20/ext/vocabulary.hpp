#ifndef ASBIND20_EXT_VOCABULARY_HPP
#define ASBIND20_EXT_VOCABULARY_HPP

#pragma once

#include <asbind20/asbind.hpp>

namespace asbind20::ext
{
namespace detail
{
    template <bool UseGeneric>
    void register_script_optional_impl(asIScriptEngine* engine);
}

void register_script_optional(asIScriptEngine* engine, bool generic = has_max_portability());

class script_optional
{
    template <bool UseGeneric>
    friend void detail::register_script_optional_impl(asIScriptEngine* engine);

public:
    script_optional() = delete;
    script_optional(const script_optional&) = delete;

    script_optional(asITypeInfo* ti);

    script_optional(asITypeInfo* ti, const script_optional& other);

    script_optional(asITypeInfo* ti, const void* value);

    ~script_optional();

    script_optional& operator=(const script_optional& other);

    bool operator==(const script_optional& rhs) const;

    void assign(const void* val);

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

    asITypeInfo* get_type_info() const
    {
        return m_ti;
    }

    void enum_refs(asIScriptEngine* engine);
    void release_refs(asIScriptEngine* engine);

    int subtype_id() const
    {
        return m_ti->GetSubTypeId();
    }

private:
    asITypeInfo* m_ti = nullptr;

    container::single m_data;
    bool m_has_value = false;
};
} // namespace asbind20::ext

#endif
