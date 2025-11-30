#include <asbind_test/framework.hpp>
#include <asbind_test/utf8.hpp>

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
