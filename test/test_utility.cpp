#include <gtest/gtest.h>
#include <asbind20/asbind.hpp>

TEST(detail, concat)
{
    using asbind20::detail::concat;

    {
        const char* name = "my_name";
        EXPECT_EQ(concat("void f(", name, ')'), "void f(my_name)");
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
