#pragma once

#include <asbind_test/framework.hpp>
#include <asbind20/container/small_vector.hpp>

namespace test_container
{
using int_sv_type = asbind20::container::small_vector<
    asbind20::container::typeinfo_identity,
    4 * sizeof(void*),
    std::allocator<void>>;

inline int_sv_type make_int_sv(std::initializer_list<int> values)
{
    // We won't use any AngelScript APIs for primitive element types,
    // so we can pass nullptr for the engine and use the type ID directly

    int_sv_type sv(nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    for(int val : values)
        sv.push_back(&val);
    return sv;
}
} // namespace test_container
