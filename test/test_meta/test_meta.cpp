#include <asbind_test/framework.hpp>
#include <asbind20/meta/type_name.hpp>

namespace
{
enum my_enum
{
    val_1 = 1,
    val_2 = 2
};
} // namespace

TEST(Meta, StaticEnumName)
{
#ifndef ASBIND20_HAS_STATIC_ENUM_NAME
    GTEST_SKIP() << "static_enum_name not supported";

#else

    using asbind20::meta::enum_name_of;

    {
        EXPECT_EQ(enum_name_of<my_enum::val_1>(), "val_1");
        EXPECT_EQ(enum_name_of<my_enum::val_2>(), "val_2");
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

namespace
{
struct my_type
{};
} // namespace

TEST(Meta, TypeName)
{
    using namespace asbind20;

    {
        constexpr auto name = meta::fixed_string_typename_of<int>();
        EXPECT_STREQ(name.c_str(), "int");
    }

    {
        constexpr auto name = meta::fixed_string_typename_of<my_type>();
        EXPECT_STREQ(name.c_str(), "my_type");
    }
}
