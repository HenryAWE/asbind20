// Tests for small_vector::insert/erase edge cases, including insertion
// that triggers reallocation and out-of-range error paths.

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

TEST(SmallVector, InsertAtEnd)
{
    auto v = make_int_sv({0, 1});

    int val = 2;
    v.insert(v.size(), &val); // == push_back
    ASSERT_EQ(v.size(), 3);
    EXPECT_EQ(*static_cast<int*>(v[2]), 2);

    // iterator overload
    int val2 = 3;
    v.insert(v.end(), &val2);
    ASSERT_EQ(v.size(), 4);
    EXPECT_EQ(*static_cast<int*>(v[3]), 3);
}

TEST(SmallVector, InsertMiddle)
{
    auto v = make_int_sv({0, 2});

    int val = 1;
    v.insert(1, &val);
    ASSERT_EQ(v.size(), 3);
    EXPECT_EQ(*static_cast<int*>(v[0]), 0);
    EXPECT_EQ(*static_cast<int*>(v[1]), 1);
    EXPECT_EQ(*static_cast<int*>(v[2]), 2);
}

TEST(SmallVector, InsertTriggeringRealloc)
{
    // Fill the static storage, then insert in the middle so that
    // reallocation is required.
    sv_type v(nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    for(std::size_t i = 0; i < v.static_capacity(); ++i)
    {
        int val = static_cast<int>(i);
        v.push_back(&val);
    }
    ASSERT_EQ(v.capacity(), v.static_capacity());

    int val = 100;
    v.insert(2, &val);
    ASSERT_EQ(v.size(), v.static_capacity() + 1);
    EXPECT_GT(v.capacity(), v.static_capacity());

    for(std::size_t i = 0; i < v.size(); ++i)
    {
        int expected = static_cast<int>(i);
        if(i == 2)
            expected = 100;
        else if(i > 2)
            expected = static_cast<int>(i - 1);
        SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
        EXPECT_EQ(*static_cast<int*>(v[i]), expected);
    }
}

TEST(SmallVector, InsertRepeatedly)
{
    // Repeated insertion at the same position, growing the vector
    sv_type v(nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    for(int i = 0; i < 8; ++i)
    {
        int val = i;
        v.insert(0, &val);
    }

    ASSERT_EQ(v.size(), 8);
    for(std::size_t i = 0; i < v.size(); ++i)
    {
        SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
        EXPECT_EQ(*static_cast<int*>(v[i]), static_cast<int>(7 - i));
    }
}

TEST(SmallVector, EraseFromMiddle)
{
    auto v = make_int_sv({0, 1, 2, 3, 4});

    v.erase(1, 2); // remove [1, 3)
    ASSERT_EQ(v.size(), 3);
    EXPECT_EQ(*static_cast<int*>(v[0]), 0);
    EXPECT_EQ(*static_cast<int*>(v[1]), 3);
    EXPECT_EQ(*static_cast<int*>(v[2]), 4);
}

#ifndef ASBIND20_NO_EXCEPTIONS

TEST(SmallVector, InsertOutOfRange)
{
    auto v = make_int_sv({0, 1});

    int val = 42;
    EXPECT_THAT(
        [&]() { v.insert(v.size() + 1, &val); },
        ::testing::Throws<std::out_of_range>()
    );
}

TEST(SmallVector, EraseOutOfRange)
{
    auto v = make_int_sv({0, 1});

    EXPECT_THAT(
        [&]() { v.erase(v.size(), 1); },
        ::testing::Throws<std::out_of_range>()
    );

    // Count is clamped to the tail; no exception expected
    EXPECT_NO_THROW(v.erase(v.size() - 1, 2));
    EXPECT_EQ(v.size(), 1);
}

TEST(SmallVector, AssignOutOfRange)
{
    auto v = make_int_sv({0, 1});

    int val = 42;
    EXPECT_THAT(
        [&]() { v.assign(v.size(), &val); },
        ::testing::Throws<std::out_of_range>()
    );
}

TEST(SmallVector, RemoveOutOfRange)
{
    auto v = make_int_sv({0, 1});

    EXPECT_THAT(
        [&]() { v.remove(v.size()); },
        ::testing::Throws<std::out_of_range>()
    );
}

TEST(SmallVector, ReverseOutOfRange)
{
    auto v = make_int_sv({0, 1});

    EXPECT_THAT(
        [&]() { v.reverse(v.size()); },
        ::testing::Throws<std::out_of_range>()
    );
}

TEST(SmallVector, VisitOutOfRange)
{
    auto v = make_int_sv({0, 1});

    // The visitor must accept all pointer kinds because every branch of
    // visit_script_type is instantiated at compile time.
    auto visitor = []<typename T>(T* start, T* stop)
    {
        (void)start;
        (void)stop;
    };
    EXPECT_THAT(
        [&]() { v.visit(visitor, v.size(), 1); },
        ::testing::Throws<std::out_of_range>()
    );
}

TEST(SmallVector, VisitEmptyOutOfRange)
{
    // visit(0, 0) on an empty vector is out of range
    auto v = make_int_sv({});

    auto visitor = []<typename T>(T* start, T* stop)
    {
        (void)start;
        (void)stop;
    };
    EXPECT_THAT(
        [&]() { v.visit(visitor, 0, 0); },
        ::testing::Throws<std::out_of_range>()
    );
}

#endif
