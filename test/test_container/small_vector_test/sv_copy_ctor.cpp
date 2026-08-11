// Tests for small_vector copy construction (primitive element type).
// Copying is deep: the copy owns its own storage independent from the source.

#include <asbind_test/framework.hpp>
#include <asbind20/container/small_vector.hpp>
#include <gmock/gmock.h>

namespace
{
using namespace asbind20;

using sv_type = container::small_vector<
    container::typeinfo_identity,
    4 * sizeof(void*),
    std::allocator<void>>;

sv_type make_int_sv(std::initializer_list<int> values)
{
    sv_type v(nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    for(int val : values)
        v.push_back(&val);
    return v;
}
} // namespace

TEST(SmallVector, CopyCtorEmpty)
{
    auto v = make_int_sv({});
    ASSERT_THAT(v, ::testing::IsEmpty());

    sv_type copy(v);
    EXPECT_THAT(copy, ::testing::IsEmpty());
    EXPECT_EQ(copy.element_type_id(), v.element_type_id());
    EXPECT_NE(copy.data(), v.data());
}

TEST(SmallVector, CopyCtorStatic)
{
    auto v = make_int_sv({0, 1, 2});
    ASSERT_LE(v.capacity(), v.static_capacity());

    sv_type copy(v);
    ASSERT_EQ(copy.size(), 3);
    EXPECT_EQ(*static_cast<int*>(copy[0]), 0);
    EXPECT_EQ(*static_cast<int*>(copy[1]), 1);
    EXPECT_EQ(*static_cast<int*>(copy[2]), 2);
    EXPECT_NE(copy.data(), v.data());

    // The copy is independent from the original
    int val = 99;
    v.assign(0, &val);
    EXPECT_EQ(*static_cast<int*>(copy[0]), 0);
}

TEST(SmallVector, CopyCtorDynamic)
{
    auto v = make_int_sv({});
    for(int i = 0; i < 64; ++i)
        v.push_back(&i);
    ASSERT_GT(v.capacity(), v.static_capacity());
    std::size_t orig_cap = v.capacity();

    sv_type copy(v);
    ASSERT_EQ(copy.size(), 64);
    EXPECT_EQ(copy.capacity(), orig_cap);
    EXPECT_NE(copy.data(), v.data());
    for(std::size_t i = 0; i < copy.size(); ++i)
    {
        SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
        EXPECT_EQ(*static_cast<int*>(copy[i]), static_cast<int>(i));
    }

    // Modifying the source must not affect the copy
    v.erase(0, 2);
    ASSERT_EQ(v.size(), 62);
    ASSERT_EQ(copy.size(), 64);
    EXPECT_EQ(*static_cast<int*>(copy[0]), 0);
    EXPECT_EQ(*static_cast<int*>(copy[63]), 63);
}
