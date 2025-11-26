#include <cstring>
#include <sstream>
#include <gtest/gtest.h>
#include <asbind20/detail/strutil.hpp>

TEST(CStringView, Constructor)
{
    using namespace asbind20::util;

    {
        cstring_view csv;
        EXPECT_EQ(csv.c_str(), nullptr);
        EXPECT_EQ(csv.size(), 0);
        EXPECT_TRUE(csv.empty());
    }

    {
        cstring_view csv = "";
        EXPECT_STREQ(csv.c_str(), "");
        EXPECT_EQ(csv.size(), 0);
        EXPECT_TRUE(csv.empty());
    }

    {
        cstring_view csv("");
        EXPECT_STREQ(csv.c_str(), "");
        EXPECT_EQ(csv.size(), 0);
        EXPECT_TRUE(csv.empty());
    }

    {
        cstring_view csv("test");
        EXPECT_STREQ(csv.c_str(), "test");
        EXPECT_EQ(csv.size(), 4);
        EXPECT_FALSE(csv.empty());
    }

    {
        auto csv = "test"_csv;
        EXPECT_STREQ(csv.c_str(), "test");
        EXPECT_EQ(csv.size(), 4);
        EXPECT_FALSE(csv.empty());
    }
}

TEST(CStringView, Compare)
{
    using namespace asbind20::util;

    {
        auto csv0 = "AAA"_csv;
        auto csv1 = "ABA"_csv;
        EXPECT_LT(csv0, csv1);
        EXPECT_GT(csv1, csv0);
        EXPECT_NE(csv0, csv1);
    }

    {
        auto csv0 = "AA"_csv;
        auto csv1 = "AAA"_csv;
        EXPECT_LT(csv0, csv1);
        EXPECT_GT(csv1, csv0);
        EXPECT_NE(csv0, csv1);
    }

    {
        auto csv0 = "AAA"_csv;
        auto csv1 = "AAA"_csv;
        EXPECT_EQ(csv0, csv1);
    }
}

TEST(CStringView, RemovePrefix)
{
    using namespace asbind20::util;

    {
        auto csv = "test"_csv;
        csv.remove_prefix(2);
        EXPECT_EQ(csv, "st");
    }
}

TEST(CStringView, Output)
{
    using namespace asbind20::util;

    {
        auto csv = "test"_csv;

        std::stringstream ss;
        ss << csv;

        EXPECT_EQ(ss.str(), "test");
    }

    {
        cstring_view csv;

        std::stringstream ss;
        ss << csv;

        EXPECT_EQ(ss.str(), "");
    }
}

TEST(CStringView, CAPI)
{
    using namespace asbind20::util;

    {
        auto csv = "test"_csv;
        csv.remove_prefix(2);

        EXPECT_EQ(std::strlen(csv.c_str()), csv.size());
        EXPECT_EQ(std::strcmp(csv.c_str(), "st"), 0);
    }
}
