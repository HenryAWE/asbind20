#include <gtest/gtest.h>
#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>

namespace
{
class placeholder_type
{};

enum class placeholder_enum
{
    val = 0
};

struct my_sv
{
    constexpr operator std::string_view() const
    {
        return "my_sv";
    }
};
} // namespace

TEST(StringViewCompat, ClassType)
{
    using namespace asbind20;

    auto engine = make_script_engine();

    {
        ref_class<placeholder_type> c(
            engine,
            std::string_view("foo_bar").substr(0, 3)
        );
    }
    {
        value_class<placeholder_type> c(
            engine,
            std::string_view("bar_foo").substr(0, 3)
        );
    }
    {
        ref_class<placeholder_type> c(
            engine,
            my_sv{}
        );
    }

    EXPECT_TRUE(engine->GetTypeInfoByDecl("foo"));
    EXPECT_TRUE(engine->GetTypeInfoByDecl("bar"));
    EXPECT_TRUE(engine->GetTypeInfoByDecl("my_sv"));
}

TEST(StringViewCompat, Misc)
{
    using namespace asbind20;
    using namespace std::string_view_literals;

    static_assert(string_view_like<std::string_view>);

    auto engine = make_script_engine();

    enum_<placeholder_enum>(engine, "placeholder_enum"sv)
        .value(placeholder_enum::val, "val");
    interface(engine, "my_intf"sv)
        .method("void f()");

    auto placeholder_enum_t = engine->GetTypeInfoByDecl("placeholder_enum");
    ASSERT_TRUE(placeholder_enum_t);

    auto my_intf_t = engine->GetTypeInfoByDecl("my_intf");
    ASSERT_TRUE(my_intf_t);

    EXPECT_EQ(placeholder_enum_t->GetEnumValueCount(), 1);
    EXPECT_STREQ(my_intf_t->GetName(), "my_intf");
}
