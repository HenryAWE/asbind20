#include <gtest/gtest.h>
#include <asbind20/asbind.hpp>

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
