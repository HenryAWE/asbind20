#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>

TEST(string_view_compat, class_type)
{
    using namespace asbind20;

    auto engine = make_script_engine();

    class placeholder_type
    {};

    ref_class<placeholder_type>(engine, std::string_view("foo"));
    value_class<placeholder_type>(engine, std::string_view("bar"));

    EXPECT_TRUE(engine->GetTypeInfoByDecl("foo"));
    EXPECT_TRUE(engine->GetTypeInfoByDecl("bar"));
}

TEST(string_view_compat, misc)
{
    enum class placeholder_enum
    {
        val = 0
    };

    using namespace asbind20;
    using namespace std::string_view_literals;

    auto engine = make_script_engine();

    enum_<placeholder_enum>(engine, "placeholder_enum"sv)
        .value(placeholder_enum::val, "val"sv);
    interface(engine, "my_intf"sv);

    auto placeholder_enum_t = engine->GetTypeInfoByDecl("placeholder_enum");
    ASSERT_TRUE(placeholder_enum_t);

    auto my_intf_t = engine->GetTypeInfoByDecl("my_intf");
    ASSERT_TRUE(my_intf_t);

    EXPECT_EQ(placeholder_enum_t->GetEnumValueCount(), 1);
    EXPECT_STREQ(my_intf_t->GetName(), "my_intf");
}
