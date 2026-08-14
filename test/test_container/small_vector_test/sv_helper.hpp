#pragma once

#include <atomic>
#include <asbind_test/framework.hpp>
#include <asbind20/container/small_vector.hpp>

namespace test_container
{
using int_sv_type = asbind20::container::small_vector<
    asbind20::container::typeinfo_identity,
    4 * sizeof(void*),
    std::allocator<void>>;

// We won't use any AngelScript APIs for primitive element types,
// so we can pass nullptr for the engine and use the type ID directly
int_sv_type make_int_sv(std::initializer_list<int> values = {});

// Helper class for testing small_vector with reference-counted handle elements
class sv_ref_foo
{
public:
    using counter_type = AS_NAMESPACE_QUALIFIER asUINT;

    sv_ref_foo()
        : data(0)
    {
        ++live_count;
    }

    ~sv_ref_foo()
    {
        --live_count;
    }

    void addref()
    {
        m_use_count += 1;
    }

    void release()
    {
        ASSERT_NE(m_use_count, 0);
        m_use_count -= 1;
        if(m_use_count == 0)
            delete this;
    }

    void set_gc_flag() {}

    bool get_gc_flag() const
    {
        return true;
    }

    void enum_refs(asbind20::engine_pointer) {}

    void release_refs(asbind20::engine_pointer) {}

    [[nodiscard]]
    counter_type use_count() const
    {
        return m_use_count;
    }

    int data = 0;

    inline static std::atomic_int live_count = 0;

private:
    counter_type m_use_count = 1;
};

// Always register by generic calling convention
void register_sv_ref_foo(asbind20::engine_pointer engine);
} // namespace test_container
