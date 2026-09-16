#include <asbind_test/framework.hpp>
#include <asbind20/meta/type_name.hpp>

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
