#include <gtest/gtest.h>
#include <asbind20/asbind.hpp>

consteval bool test_concepts()
{
    {
        struct callable_struct
        {
            void operator()() const {}
        };

        using asbind20::noncapturing_lambda;

        static_assert(!noncapturing_lambda<callable_struct>);

        int tmp = 0;
        auto lambda1 = [&tmp](int)
        {
            return tmp;
        };

        static_assert(!noncapturing_lambda<decltype(lambda1)>);

        auto lambda2 = [](int)
        {
            return 42;
        };

        static_assert(noncapturing_lambda<decltype(lambda2)>);
        static_assert((+lambda2)(0) == 42);
    }

    return true;
}

static_assert(test_concepts());

namespace test_utility
{
int f1()
{
    return 1013;
}
} // namespace test_utility

TEST(utility, fp_wrapper)
{
    using namespace asbind20;

    constexpr auto wrapper = fp<&test_utility::f1>;
    static constexpr auto f1 = wrapper.get();
    EXPECT_EQ(f1(), 1013);
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

namespace test_utility
{
enum my_enum
{
    val_1 = 1,
    val_2 = 2
};
} // namespace test_utility

TEST(utility, static_enum_name)
{
    using namespace asbind20;

#ifndef ASBIND20_HAS_STATIC_ENUM_NAME
    GTEST_SKIP() << "static_enum_name not supported";

#else

    {
        using test_utility::my_enum;
        EXPECT_EQ(static_enum_name<my_enum::val_1>(), "val_1");
        EXPECT_EQ(static_enum_name<my_enum::val_2>(), "val_2");
    }

    {
        enum class my_scoped_enum
        {
            abc = 1,
            def = 2
        };

        EXPECT_EQ(static_enum_name<my_scoped_enum::abc>(), "abc");
        EXPECT_EQ(static_enum_name<my_scoped_enum::def>(), "def");
    }

#endif
}

TEST(utility, asEContextState_to_string)
{
    using namespace asbind20;

    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED), "asEXECUTION_FINISHED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_SUSPENDED), "asEXECUTION_SUSPENDED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_ABORTED), "asEXECUTION_ABORTED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION), "asEXECUTION_EXCEPTION");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_PREPARED), "asEXECUTION_PREPARED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_UNINITIALIZED), "asEXECUTION_UNINITIALIZED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_ACTIVE), "asEXECUTION_ACTIVE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_ERROR), "asEXECUTION_ERROR");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEXECUTION_DESERIALIZATION), "asEXECUTION_DESERIALIZATION");

    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEContextState(-1)), "asEContextState(-1)");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asEContextState(-2)), "asEContextState(-2)");
}

TEST(utility, asERetCodes_to_string)
{
    using namespace asbind20;

    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asSUCCESS), "asSUCCESS");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asERROR), "asERROR");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asCONTEXT_ACTIVE), "asCONTEXT_ACTIVE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asCONTEXT_NOT_FINISHED), "asCONTEXT_NOT_FINISHED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asCONTEXT_NOT_PREPARED), "asCONTEXT_NOT_PREPARED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_ARG), "asINVALID_ARG");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asNO_FUNCTION), "asNO_FUNCTION");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asNOT_SUPPORTED), "asNOT_SUPPORTED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_NAME), "asINVALID_NAME");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asNAME_TAKEN), "asNAME_TAKEN");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_DECLARATION), "asINVALID_DECLARATION");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_OBJECT), "asINVALID_OBJECT");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_TYPE), "asINVALID_TYPE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asALREADY_REGISTERED), "asALREADY_REGISTERED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asMULTIPLE_FUNCTIONS), "asMULTIPLE_FUNCTIONS");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asNO_MODULE), "asNO_MODULE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asNO_GLOBAL_VAR), "asNO_GLOBAL_VAR");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_CONFIGURATION), "asINVALID_CONFIGURATION");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINVALID_INTERFACE), "asINVALID_INTERFACE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asCANT_BIND_ALL_FUNCTIONS), "asCANT_BIND_ALL_FUNCTIONS");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asLOWER_ARRAY_DIMENSION_NOT_REGISTERED), "asLOWER_ARRAY_DIMENSION_NOT_REGISTERED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asWRONG_CONFIG_GROUP), "asWRONG_CONFIG_GROUP");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asCONFIG_GROUP_IS_IN_USE), "asCONFIG_GROUP_IS_IN_USE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asILLEGAL_BEHAVIOUR_FOR_TYPE), "asILLEGAL_BEHAVIOUR_FOR_TYPE");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asWRONG_CALLING_CONV), "asWRONG_CALLING_CONV");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asBUILD_IN_PROGRESS), "asBUILD_IN_PROGRESS");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asINIT_GLOBAL_VARS_FAILED), "asINIT_GLOBAL_VARS_FAILED");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asOUT_OF_MEMORY), "asOUT_OF_MEMORY");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asMODULE_IS_IN_USE), "asMODULE_IS_IN_USE");

    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asERetCodes(1)), "asERetCodes(1)");
    EXPECT_EQ(to_string(AS_NAMESPACE_QUALIFIER asERetCodes(2)), "asERetCodes(2)");
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

TEST(name_of, arithmetic)
{
    using namespace asbind20;
    using namespace std::literals;

    EXPECT_EQ(name_of<bool>(), "bool"sv);

    EXPECT_EQ(name_of<asINT8>(), "int8"sv);
    EXPECT_EQ(name_of<asINT16>(), "int16"sv);
    EXPECT_EQ(name_of<asINT32>(), "int"sv);
    EXPECT_EQ(name_of<asINT64>(), "int64"sv);

    EXPECT_EQ(name_of<asBYTE>(), "uint8"sv);
    EXPECT_EQ(name_of<asWORD>(), "uint16"sv);
    EXPECT_EQ(name_of<asDWORD>(), "uint"sv);
    EXPECT_EQ(name_of<asQWORD>(), "uint64"sv);

    EXPECT_EQ(name_of<float>(), "float"sv);
    EXPECT_EQ(name_of<double>(), "double"sv);
}

TEST(meta, fixed_string)
{
    using namespace asbind20;

    static_assert(meta::fixed_string().empty());
    static_assert(meta::fixed_string("int").size() == 3);
    static_assert(meta::fixed_string("int").view() == "int");

    {
        meta::fixed_string s = "int";
        EXPECT_EQ(s.size(), 3);
        EXPECT_STREQ(s.c_str(), "int");

        std::string str(s);
        EXPECT_EQ(str, "int");
    }

    {
        auto result = string_concat("void f()", meta::fixed_string("{int,int}"));
        EXPECT_EQ(result, "void f(){int,int}");
    }

    {
        constexpr meta::fixed_string hello = "hello";
        constexpr meta::fixed_string world = " world";
        static_assert((hello + world).view() == "hello world");
        static_assert((hello + meta::fixed_string()).view() == "hello");
        static_assert((meta::fixed_string() + hello).view() == "hello");

        auto result = hello + world;
        EXPECT_EQ(result.size(), 11);
        EXPECT_STREQ(result.c_str(), "hello world");
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
#ifdef ASBIND20_HAS_STATIC_ENUM_NAME
    std::cerr << "ASBIND20_HAS_STATIC_ENUM_NAME: "
              << ASBIND20_HAS_STATIC_ENUM_NAME
              << std::endl;
#endif
    return RUN_ALL_TESTS();
}
