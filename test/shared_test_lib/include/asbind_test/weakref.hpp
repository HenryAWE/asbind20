// Weak reference support for script objects in tests

#ifndef ASBIND_TEST_WEAKREF_HPP
#define ASBIND_TEST_WEAKREF_HPP

#pragma once

#include <asbind20/asbind.hpp>

namespace asbind_test
{
class script_weakref
{
public:
    script_weakref() = delete;

    script_weakref(const script_weakref&);

    script_weakref(asbind20::typeinfo_pointer ti);
    script_weakref(asbind20::typeinfo_pointer ti, void* ref);

    ~script_weakref();

    script_weakref& operator=(const script_weakref& other);

    bool operator==(const script_weakref& rhs) const;

    void reset(void* ref);

    void* get();

    /**
     * @brief Checks equality of the referenced object
     */
    bool equals_to(void* ref) const;

    [[nodiscard]]
    asbind20::typeinfo_pointer referenced_type() const
    {
        return m_ti->GetSubType();
    }

private:
    void* m_ref;
    asbind20::script_typeinfo m_ti;
    asbind20::lockable_shared_bool m_flag;
};

void register_weakref(
    asbind20::engine_pointer engine,
    bool use_generic = asbind20::has_max_portability()
);

void register_weakref(
    const asbind20::unique_script_engine& engine,
    bool use_generic = asbind20::has_max_portability()
);
} // namespace asbind_test

#endif
