// Tests for small_vector::const_iterator semantics:
// random access, comparisons, subscripts, and underflow guards.

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

TEST(SmallVector, ConstIteratorBasics)
{
    auto v = make_int_sv({0, 1, 2});
    const auto& cv = std::as_const(v);

    auto it = cv.cbegin();
    EXPECT_TRUE(it); // operator bool
    EXPECT_EQ(it.get_container(), &cv);
    EXPECT_EQ(cv.cend().get_container(), &cv);

    // Default constructed iterator is empty
    sv_type::const_iterator empty_it;
    EXPECT_FALSE(static_cast<bool>(empty_it));
    EXPECT_EQ(*empty_it, nullptr);
}

TEST(SmallVector, ConstIteratorDerefAndSubscript)
{
    auto v = make_int_sv({10, 20, 30});
    const auto& cv = std::as_const(v);

    auto it = cv.cbegin();
    EXPECT_EQ(*static_cast<const int*>(*it), 10);
    EXPECT_EQ(*static_cast<const int*>(it[1]), 20);
    EXPECT_EQ(*static_cast<const int*>(it[2]), 30);
}

TEST(SmallVector, ConstIteratorIncrementDecrement)
{
    auto v = make_int_sv({0, 1, 2});
    const auto& cv = std::as_const(v);

    auto it = cv.cbegin();
    ++it;
    EXPECT_EQ(*static_cast<const int*>(*it), 1);
    it++;
    EXPECT_EQ(*static_cast<const int*>(*it), 2);

    --it;
    EXPECT_EQ(*static_cast<const int*>(*it), 1);
    it--;
    EXPECT_EQ(*static_cast<const int*>(*it), 0);
}

TEST(SmallVector, ConstIteratorRandomAccess)
{
    auto v = make_int_sv({0, 1, 2, 3, 4});
    const auto& cv = std::as_const(v);

    auto it = cv.cbegin() + 2;
    EXPECT_EQ(*static_cast<const int*>(*it), 2);

    it += 2;
    EXPECT_EQ(*static_cast<const int*>(*it), 4);

    it -= 3;
    EXPECT_EQ(*static_cast<const int*>(*it), 1);

    auto it2 = it + 1;
    EXPECT_EQ(it2 - it, 1);
    EXPECT_EQ(it - it2, -1);

    // past-the-end arithmetic
    EXPECT_EQ(cv.cend() - cv.cbegin(), 5);

    // comparisons
    EXPECT_TRUE(cv.cbegin() < cv.cend());
    EXPECT_TRUE(cv.cend() > cv.cbegin());
    EXPECT_TRUE(cv.cbegin() == cv.cbegin());
    EXPECT_TRUE(cv.cbegin() != cv.cend());
    EXPECT_TRUE(cv.cbegin() <= cv.cbegin() + 1);
    EXPECT_TRUE(cv.cend() >= cv.cend());
}

TEST(SmallVector, ConstIteratorUnderflowGuard)
{
    auto v = make_int_sv({0, 1, 2});
    const auto& cv = std::as_const(v);

    // -- on begin() is guarded and stays at begin()
    auto it = cv.cbegin();
    --it;
    EXPECT_EQ(it, cv.cbegin());

    // -1 on begin() is guarded
    auto it2 = cv.cbegin() - 1;
    EXPECT_EQ(it2, cv.cbegin());
}
