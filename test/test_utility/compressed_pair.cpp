#include <asbind20/asbind.hpp>
#include <asbind_test/framework.hpp>

TEST(CompressedPair, Ordinary)
{
    using asbind20::compressed_pair;

    compressed_pair<int, int> p_1{0, 1};
    static_assert(sizeof(p_1) == sizeof(int) * 2);

    EXPECT_EQ(p_1.first(), 0);
    EXPECT_EQ(p_1.second(), 1);

    compressed_pair<int, int> p_2 = p_1;
    EXPECT_EQ(p_2.first(), 0);
    EXPECT_EQ(p_2.second(), 1);

    p_2.first() = 2;
    p_2.second() = 3;
    p_1.swap(p_2);
    EXPECT_EQ(p_1.first(), 2);
    EXPECT_EQ(p_1.second(), 3);
    EXPECT_EQ(p_2.first(), 0);
    EXPECT_EQ(p_2.second(), 1);

    {
        auto& [a, b] = p_1;
        EXPECT_EQ(a, 2);
        EXPECT_EQ(b, 3);
    }
}

namespace test_utility
{
static int counter_1 = 0;

class empty_1
{
public:
    empty_1()
    {
        ++counter_1;
    }
};

class empty_2
{};
} // namespace test_utility

TEST(CompressedPair, Optimized)
{
    using asbind20::compressed_pair;
    using asbind20::detail::select_compressed_pair_impl;
    using namespace test_utility;

    EXPECT_EQ(counter_1, 0);

    compressed_pair<std::string, empty_1> p_1;
    static_assert(select_compressed_pair_impl<std::string, empty_1>() == 2);
    static_assert(sizeof(p_1) == sizeof(std::string));
    EXPECT_EQ(counter_1, 1);
    p_1.first() = "hello";
    EXPECT_EQ(std::as_const(p_1).first(), "hello");

    compressed_pair<empty_1, std::string> p_2;
    static_assert(sizeof(p_2) == sizeof(std::string));
    p_2.second() = "hello";
    EXPECT_EQ(std::as_const(p_2).second(), "hello");

    compressed_pair<empty_1, empty_2> p_3;
    static_assert(sizeof(p_3) == 1);
    static_assert(std::is_empty_v<compressed_pair<empty_1, empty_2>>);
    EXPECT_EQ(counter_1, 3);

    compressed_pair<empty_1, empty_1> p_4{};
    static_assert(sizeof(p_4) <= 2);
    static_assert(!std::is_empty_v<compressed_pair<empty_1, empty_1>>);
    EXPECT_EQ(counter_1, 5);
}
