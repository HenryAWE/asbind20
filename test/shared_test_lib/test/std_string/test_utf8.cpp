#include <asbind_test/framework.hpp>
#include <asbind_test/std_string.hpp>
#include <asbind_test/utf8.hpp>
#include <vector>

namespace utf8 = asbind_test::utf8;

TEST(UTF8, RemovePrefixAndSuffix)
{
    using namespace std::literals;

    EXPECT_EQ(utf8::u8_remove_prefix("hello world!", 6), "world!"sv);
    EXPECT_EQ(utf8::u8_remove_suffix("hello world!", 1), "hello world"sv);
}

TEST(UTF8, Substring)
{
    EXPECT_EQ(utf8::u8_substr("hello world", 7, 2), "or");
    EXPECT_EQ(utf8::u8_substr_r("hello world", 4, 2), "or");
}

TEST(UTF8, ScriptStringSubstr)
{
    using asbind_test::script_string::string_substr;

    EXPECT_EQ(string_substr("hello world", 7, 2), "or");
    EXPECT_EQ(string_substr("hello world", -4, 2), "or");
}

TEST(UTF8, ScriptStringOpIndex)
{
    using asbind_test::script_string::string_opIndex;

    EXPECT_EQ(string_opIndex("hello world", 0), U'h');
    EXPECT_EQ(string_opIndex("hello world", -1), U'd');
}

TEST(UTF8, ConstStringIterator)
{
    std::string_view str = "1234";

    std::vector<char32_t> out;
    for(auto it = utf8::string_cbegin(str); it != utf8::string_cend(str); ++it)
    {
        out.push_back(*it);
    }

    EXPECT_EQ(out, (std::vector<char32_t>{'1', '2', '3', '4'}));
}
