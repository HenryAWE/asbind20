#include <cstring>
#include <sstream>
#include <gtest/gtest.h>
#include <asbind20/detail/strutil.hpp>

static_assert(!std::is_constructible_v<asbind20::cstring_ref, std::nullptr_t>);
static_assert(std::is_constructible_v<asbind20::cstring_ref, const char*>);

TEST(CStringRef, Basic)
{
    using namespace asbind20;
    using namespace std::literals;

    {
        cstring_ref csr;
        EXPECT_EQ(csr.c_str(), nullptr);
        EXPECT_EQ(csr, "");
        EXPECT_EQ(csr, ""sv);
        EXPECT_EQ(csr, ""s);
        EXPECT_STREQ(csr.safe_c_str(), "");
        EXPECT_EQ(csr.size(), 0);
        EXPECT_FALSE(csr);
    }

    {
        cstring_ref csr = "";
        EXPECT_STREQ(csr.c_str(), "");
        EXPECT_STREQ(csr.safe_c_str(), "");
        EXPECT_EQ(csr, "");
        EXPECT_EQ(csr, ""sv);
        EXPECT_EQ(csr, ""s);
        EXPECT_EQ(csr.size(), 0);
        EXPECT_TRUE(csr);
    }

    {
        cstring_ref csr("");
        EXPECT_STREQ(csr.c_str(), "");
        EXPECT_EQ(csr.size(), 0);
    }

    {
        cstring_ref csr("test");
        EXPECT_STREQ(csr.c_str(), "test");
        EXPECT_EQ(csr, "test");
        EXPECT_EQ(csr, "test"sv);
        EXPECT_EQ(csr, "test"s);
        EXPECT_EQ(csr.size(), 4);
    }

    {
        cstring_ref csr = "test";
        EXPECT_STREQ(csr.c_str(), "test");
        EXPECT_EQ(csr.size(), 4);
    }
}

TEST(CStringRef, RemovePrefix)
{
    using namespace asbind20;

    {
        cstring_ref csr = "test";
        csr.remove_prefix(2);
        EXPECT_EQ(csr, "st");
    }

    {
        cstring_ref csr = "test";
        auto csr_trimmed = csr.trim_prefix(2);
        EXPECT_EQ(csr, "test");
        EXPECT_EQ(csr_trimmed, "st");
    }
}

TEST(CStringRef, Output)
{
    using namespace asbind20;

    {
        cstring_ref csr = "test";

        std::ostringstream ss;
        ss << csr;

        EXPECT_EQ(ss.str(), "test");
    }

    {
        cstring_ref csr;

        std::ostringstream ss;
        ss << csr;

        EXPECT_EQ(ss.str(), "");
    }
}

TEST(CStringRef, CAPI)
{
    using namespace asbind20;

    {
        cstring_ref csr = "test";
        csr.remove_prefix(2);

        EXPECT_EQ(std::strlen(csr.c_str()), csr.size());
        EXPECT_EQ(std::strcmp(csr.c_str(), "st"), 0);
    }
}
