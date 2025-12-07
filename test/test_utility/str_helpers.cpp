#include <gtest/gtest.h>
#include <asbind20/utility.hpp>

TEST(Utility, StringConcat)
{
    using asbind20::string_concat;

    EXPECT_EQ(string_concat(), "");

    {
        const char* name = "my_name";
        EXPECT_EQ(string_concat("void f(", name, ')'), "void f(my_name)");
    }

    {
        using namespace std::string_view_literals;
        const char* name = "my_name";
        EXPECT_EQ(string_concat("void f("sv, name, ')'), "void f(my_name)");
    }
}

TEST(WithCStr, WithCStr)
{
    using namespace asbind20;
    using namespace std::string_view_literals;

    {
        std::string result = util::with_cstr(
            [](const char* a, char ch, const char* b, const char* c, const char* d)
            {
                return string_concat(a, ch, b, c, d);
            },
            "hello",
            ' ',
            "hello world"sv.substr(0, 5),
            cstring_ref("!"),
            util::fixed_string("!!")
        );
        EXPECT_EQ(result, "hello hello!!!");
    }

    {
        std::string result = util::with_cstr(
            [](const char* a, char ch, const char* b, const char* c)
            {
                return std::string(a) + ch + std::string(b) + c;
            },
            "hello",
            ' ',
            "hello world"sv.substr(0, 5),
            std::string(" world")
        );
        EXPECT_EQ(result, "hello hello world");
    }
}
