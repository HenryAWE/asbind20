#include <asbind_test/framework.hpp>
#include <asbind20/meta.hpp>
#include "test_meta.hpp"

namespace test_meta
{
void placeholder_gfn(asbind20::generic_pointer gen)
{
    // Add a simple testcase here, so "gen" is not unused
    EXPECT_THAT(gen, ::testing::NotNull());
    asbind20::set_script_exception("unreachable");
}
} // namespace test_meta

namespace
{
enum my_enum
{
    val_1 = 1,
    val_2 = 2
};
} // namespace

TEST(Meta, EnumNameOf)
{
#ifndef ASBIND20_HAS_ENUM_NAME_OF
    GTEST_SKIP() << "enum_name_of not supported";

#else

    using asbind20::meta::enum_name_of;

    {
        EXPECT_EQ(enum_name_of<val_1>(), "val_1");
        EXPECT_EQ(enum_name_of<val_2>(), "val_2");
    }

    {
        enum class my_scoped_enum
        {
            abc = 1,
            def = 2
        };

        EXPECT_EQ(enum_name_of<my_scoped_enum::abc>(), "abc");
        EXPECT_EQ(enum_name_of<my_scoped_enum::def>(), "def");
    }

#endif
}
