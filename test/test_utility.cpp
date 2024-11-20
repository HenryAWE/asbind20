#include <gtest/gtest.h>
#include <asbind20/asbind.hpp>

TEST(utility, refptr_wrapper)
{
    using asbind20::refptr_wrapper;

    {
        int val = 42;
        refptr_wrapper r(val);

        static_assert(std::is_same_v<int, decltype(r)::type>);

        EXPECT_EQ(r.get(), 42);

        [](int& v)
        {
            EXPECT_EQ(v, 42);
        }(r);

        [&val](int* v)
        {
            EXPECT_EQ(*v, 42);
            EXPECT_EQ(&val, v);
        }(r);
    }
}

TEST(utility, string_concat)
{
    using asbind20::string_concat;

    {
        const char* name = "my_name";
        EXPECT_EQ(string_concat("void f(", name, ')'), "void f(my_name)");
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
