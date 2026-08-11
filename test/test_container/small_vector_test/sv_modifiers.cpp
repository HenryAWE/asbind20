// Tests for small_vector modifiers: resize, assign, remove, reverse,
// data_at, clear, and shrink_to_fit (including the fallback to static
// storage). All tests here use an `int` element type and don't need a
// script engine.

#include <asbind_test/framework.hpp>
#include <asbind20/container/small_vector.hpp>
#include <gmock/gmock.h>
#include "sv_helper.hpp"

namespace
{
using namespace asbind20;

using sv_type = test_container::int_sv_type;
using test_container::make_int_sv;

void push_ints(sv_type& v, std::initializer_list<int> values)
{
    for(int val : values)
        v.push_back(&val);
}
} // namespace

TEST(SmallVector, ResizeGrow)
{
    auto v = make_int_sv({});
    v.resize(5);
    ASSERT_EQ(v.size(), 5);
    for(std::size_t i = 0; i < v.size(); ++i)
    {
        SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
        EXPECT_EQ(*static_cast<int*>(v[i]), 0)
            << "new elements should be zero-initialized";
    }
}

TEST(SmallVector, ResizeShrink)
{
    auto v = make_int_sv({0, 1, 2, 3});

    v.resize(2);
    ASSERT_EQ(v.size(), 2);
    EXPECT_EQ(*static_cast<int*>(v[0]), 0);
    EXPECT_EQ(*static_cast<int*>(v[1]), 1);
}

TEST(SmallVector, ResizeToZero)
{
    auto v = make_int_sv({0, 1, 2});

    v.resize(0);
    EXPECT_THAT(v, ::testing::IsEmpty());
}

TEST(SmallVector, ResizeNoChange)
{
    auto v = make_int_sv({7});

    v.resize(1); // same size, must be a no-op
    ASSERT_EQ(v.size(), 1);
    EXPECT_EQ(*static_cast<int*>(v[0]), 7);
}

TEST(SmallVector, Assign)
{
    auto v = make_int_sv({0, 1, 2});

    int val = 42;
    v.assign(1, &val);
    EXPECT_EQ(*static_cast<int*>(v[1]), 42);

    // iterator overload
    int val2 = 1013;
    v.assign(v.begin() + 2, &val2);
    EXPECT_EQ(*static_cast<int*>(v[2]), 1013);

    // untouched element
    EXPECT_EQ(*static_cast<int*>(v[0]), 0);
}

TEST(SmallVector, Remove)
{
    // `remove()` moves the element to the end of the buffer without
    // shrinking the size; the caller is expected to erase the tail later.
    auto v = make_int_sv({0, 1, 2, 3});

    v.remove(1);
    ASSERT_EQ(v.size(), 4);
    EXPECT_EQ(*static_cast<int*>(v[0]), 0);
    EXPECT_EQ(*static_cast<int*>(v[1]), 2);
    EXPECT_EQ(*static_cast<int*>(v[2]), 3);
    EXPECT_EQ(*static_cast<int*>(v[3]), 1); // moved to the end

    // Documented usage: erase the tail element after remove()
    v.erase(v.size() - 1);
    ASSERT_EQ(v.size(), 3);
    EXPECT_EQ(*static_cast<int*>(v[0]), 0);
    EXPECT_EQ(*static_cast<int*>(v[1]), 2);
    EXPECT_EQ(*static_cast<int*>(v[2]), 3);
}

TEST(SmallVector, Reverse)
{
    auto v = make_int_sv({0, 1, 2, 3, 4});

    v.reverse(0, 5);
    ASSERT_EQ(v.size(), 5);
    for(std::size_t i = 0; i < v.size(); ++i)
    {
        SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
        EXPECT_EQ(*static_cast<int*>(v[i]), static_cast<int>(4 - i));
    }

    // Reverse a subrange [1, 4) only
    push_ints(v, {5, 6}); // now [4, 3, 2, 1, 0, 5, 6]
    v.reverse(1, 3);      // expect [4, 1, 2, 3, 0, 5, 6]
    const int expected[] = {4, 1, 2, 3, 0, 5, 6};
    ASSERT_EQ(v.size(), 7);
    for(std::size_t i = 0; i < v.size(); ++i)
    {
        SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
        EXPECT_EQ(*static_cast<int*>(v[i]), expected[i]);
    }
}

TEST(SmallVector, ReverseNoOp)
{
    auto v = make_int_sv({0, 1, 2});

    v.reverse(0, 0); // n == 0 -> no effect
    ASSERT_EQ(v.size(), 3);
    for(std::size_t i = 0; i < v.size(); ++i)
    {
        SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
        EXPECT_EQ(*static_cast<int*>(v[i]), static_cast<int>(i));
    }
}

TEST(SmallVector, DataAt)
{
    auto v = make_int_sv({10, 20});

    int* base = static_cast<int*>(v.data());
    ASSERT_THAT(base, ::testing::NotNull());
    EXPECT_EQ(static_cast<int*>(v.data_at(0)), base);
    EXPECT_EQ(static_cast<int*>(v.data_at(1)), base + 1);
    EXPECT_EQ(static_cast<int*>(v.data_at(2)), base + 2); // past-the-end
}

TEST(SmallVector, ShrinkToFitFallbackToStatic)
{
    auto v = make_int_sv({});

    for(int i = 0; i < 64; ++i)
        v.push_back(&i);
    ASSERT_GT(v.capacity(), v.static_capacity());

    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), v.size());

    // After clearing, shrink_to_fit() falls back to the static storage
    v.clear();
    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), v.static_capacity());
}

TEST(SmallVector, ClearKeepsCapacity)
{
    auto v = make_int_sv({});
    for(int i = 0; i < 16; ++i)
        v.push_back(&i);
    std::size_t cap = v.capacity();
    ASSERT_GT(cap, v.static_capacity());

    v.clear();
    EXPECT_THAT(v, ::testing::IsEmpty());
    EXPECT_EQ(v.capacity(), cap);
}

TEST(SmallVector, PopBackEmpty)
{
    auto v = make_int_sv({});
    v.pop_back(); // must be a no-op
    EXPECT_THAT(v, ::testing::IsEmpty());
}

TEST(SmallVector, EraseAll)
{
    auto v = make_int_sv({0, 1, 2});

    v.erase(0, v.size());
    EXPECT_THAT(v, ::testing::IsEmpty());
}

TEST(SmallVector, EraseZeroCount)
{
    auto v = make_int_sv({0, 1, 2});

    v.erase(0, 0); // n == 0 -> no effect
    ASSERT_EQ(v.size(), 3);
    for(std::size_t i = 0; i < v.size(); ++i)
    {
        SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
        EXPECT_EQ(*static_cast<int*>(v[i]), static_cast<int>(i));
    }
}

TEST(SmallVector, PushBackN)
{
    auto v = make_int_sv({});
    int val = 42;
    v.push_back_n(3, &val);
    ASSERT_EQ(v.size(), 3);
    for(std::size_t i = 0; i < v.size(); ++i)
    {
        SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
        EXPECT_EQ(*static_cast<int*>(v[i]), 42);
    }
}

TEST(SmallVector, EmplaceBackN)
{
    auto v = make_int_sv({});
    v.emplace_back_n(5);
    ASSERT_EQ(v.size(), 5);
    for(std::size_t i = 0; i < v.size(); ++i)
    {
        SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
        EXPECT_EQ(*static_cast<int*>(v[i]), 0);
    }
}

TEST(SmallVector, OperatorIndexOutOfRangeReturnsNull)
{
    auto v = make_int_sv({0, 1});
    EXPECT_THAT(v[0], ::testing::NotNull());
    EXPECT_EQ(v[2], nullptr);   // past-the-end
    EXPECT_EQ(v[100], nullptr); // far out of range
}

TEST(SmallVector, VisitPrimitive)
{
    auto v = make_int_sv({0, 1, 2, 3, 4});
    const auto& cv = std::as_const(v);

    // The visitor must accept all pointer kinds because every branch of
    // visit_script_type is instantiated at compile time.
    auto sum_visitor = [](int& sum)
    {
        return [&sum]<typename T>(T* start, T* stop)
        {
            if constexpr(std::same_as<T, int>)
            {
                for(int* it = start; it != stop; ++it)
                    sum += *it;
            }
        };
    };

    // visit(start, count): elements [1, 4)
    int sum = 0;
    v.visit(sum_visitor(sum), 1, 3);
    EXPECT_EQ(sum, 1 + 2 + 3);

    // visit(iterator, iterator): elements [2, end)
    int sum2 = 0;
    v.visit(sum_visitor(sum2), cv.cbegin() + 2, cv.cend());
    EXPECT_EQ(sum2, 2 + 3 + 4);
}
