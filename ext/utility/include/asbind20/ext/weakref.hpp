#ifndef ASBIND20_EXT_WEAKREF_HPP
#define ASBIND20_EXT_WEAKREF_HPP

#pragma once

#include <asbind20/asbind.hpp>

namespace asbind20::ext
{
class weakref
{
public:
    weakref() = delete;

    weakref(AS_NAMESPACE_QUALIFIER asITypeInfo* ti);
    weakref(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, void* ref);

    ~weakref();

    weakref& operator=(const weakref& other);

    bool operator==(const weakref& rhs) const;

    void reset(void* ref);

    void* get();

    /**
     * @brief Checks equality of the referenced object
     */
    bool equals_to(void* ref) const;

    AS_NAMESPACE_QUALIFIER asITypeInfo* referenced_type() const
    {
        return m_ti->GetSubType();
    }

private:
    void* m_ref;
    script_typeinfo m_ti;
    lockable_shared_bool m_flag;
};

void register_weakref(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, bool use_generic = has_max_portability());
} // namespace asbind20::ext

#endif
