#include <cstring>
#include <sstream>
#include <gtest/gtest.h>
#include <asbind20/detail/strutil.hpp>

static_assert(!std::is_constructible_v<asbind20::cstring_ref, std::nullptr_t>);
static_assert(std::is_constructible_v<asbind20::cstring_ref, const char*>);

TEST(CStringView, Constructor)
{
    using namespace asbind20;

    {
        cstring_ref csv;
        EXPECT_EQ(csv.c_str(), nullptr);
        EXPECT_EQ(csv.size(), 0);
    }

    {
        cstring_ref csv = "";
        EXPECT_STREQ(csv.c_str(), "");
        EXPECT_EQ(csv.size(), 0);
    }

    {
        cstring_ref csv("");
        EXPECT_STREQ(csv.c_str(), "");
        EXPECT_EQ(csv.size(), 0);
    }

    {
        cstring_ref csv("test");
        EXPECT_STREQ(csv.c_str(), "test");
        EXPECT_EQ(csv.size(), 4);
    }

    {
        cstring_ref csv = "test";
        EXPECT_STREQ(csv.c_str(), "test");
        EXPECT_EQ(csv.size(), 4);
    }
}

TEST(CStringView, RemovePrefix)
{
    using namespace asbind20;

    {
        cstring_ref csv = "test";
        csv.remove_prefix(2);
        EXPECT_EQ(csv, "st");
    }

    {
        cstring_ref csv = "test";
        auto csv_trimmed = csv.trim_prefix(2);
        EXPECT_EQ(csv, "test");
        EXPECT_EQ(csv_trimmed, "st");
    }
}

TEST(CStringView, Output)
{
    using namespace asbind20;

    {
        cstring_ref csv = "test";

        std::stringstream ss;
        ss << csv;

        EXPECT_EQ(ss.str(), "test");
    }

    {
        cstring_ref csv;

        std::stringstream ss;
        ss << csv;

        EXPECT_EQ(ss.str(), "");
    }
}

TEST(CStringView, CAPI)
{
    using namespace asbind20;

    {
        cstring_ref csv = "test";
        csv.remove_prefix(2);

        EXPECT_EQ(std::strlen(csv.c_str()), csv.size());
        EXPECT_EQ(std::strcmp(csv.c_str(), "st"), 0);
    }
}
