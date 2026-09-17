#include <asbind_test/framework.hpp>
#include <asbind20/meta/name_of.hpp>

namespace
{
struct my_type
{};
} // namespace

TEST(NameOf, TypeNameOf)
{
    EXPECT_EQ(
        asbind20::meta::typename_of<bool>(),
        "bool"
    );
    EXPECT_EQ(
        asbind20::meta::typename_of<my_type>(),
        "my_type"
    );
}

TEST(NameOf, FixedStringTypeNameOf)
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
