#include <gtest/gtest.h>
#include <asbind20/asbind.hpp>

TEST(utility, refptr_wrapper)
{
    using asbind20::refptr_wrapper;

    {
        int val = 42;
        refptr_wrapper r(val);

        static_assert(std::is_same_v<int, decltype(r)::type>);

        EXPECT_EQ(r.get(), 42);

        [](int& v)
        {
            EXPECT_EQ(v, 42);
        }(r);

        [&val](int* v)
        {
            EXPECT_EQ(*v, 42);
            EXPECT_EQ(&val, v);
        }(r);
    }
}

TEST(utility, string_concat)
{
    using asbind20::string_concat;

    EXPECT_EQ(string_concat(), "");

    {
        const char* name = "my_name";
        EXPECT_EQ(string_concat("void f(", name, ')'), "void f(my_name)");
    }
}

TEST(utility, asEContextState_to_string)
{
    using namespace asbind20;

    EXPECT_EQ(to_string(asEXECUTION_FINISHED), "asEXECUTION_FINISHED");
    EXPECT_EQ(to_string(asEXECUTION_SUSPENDED), "asEXECUTION_SUSPENDED");
    EXPECT_EQ(to_string(asEXECUTION_ABORTED), "asEXECUTION_ABORTED");
    EXPECT_EQ(to_string(asEXECUTION_EXCEPTION), "asEXECUTION_EXCEPTION");
    EXPECT_EQ(to_string(asEXECUTION_PREPARED), "asEXECUTION_PREPARED");
    EXPECT_EQ(to_string(asEXECUTION_UNINITIALIZED), "asEXECUTION_UNINITIALIZED");
    EXPECT_EQ(to_string(asEXECUTION_ACTIVE), "asEXECUTION_ACTIVE");
    EXPECT_EQ(to_string(asEXECUTION_ERROR), "asEXECUTION_ERROR");
    EXPECT_EQ(to_string(asEXECUTION_DESERIALIZATION), "asEXECUTION_DESERIALIZATION");

    EXPECT_EQ(to_string(asEContextState(-1)), "asEContextState(-1)");
    EXPECT_EQ(to_string(asEContextState(-2)), "asEContextState(-2)");
}

TEST(utility, version)
{
    {
        std::string ver_str;
        ver_str += std::to_string(ASBIND20_VERSION_MAJOR);
        ver_str += '.';
        ver_str += std::to_string(ASBIND20_VERSION_MINOR);
        ver_str += '.';
        ver_str += std::to_string(ASBIND20_VERSION_PATCH);

        EXPECT_EQ(ver_str, ASBIND20_VERSION_STRING);
    }

    {
        std::string_view sv = asGetLibraryOptions();
        bool max_portability_found = sv.find("AS_MAX_PORTABILITY") != sv.npos;

        EXPECT_EQ(asbind20::has_max_portability(), max_portability_found);
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
