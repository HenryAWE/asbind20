#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <asbind20/asbind.hpp>

namespace test_utility
{
static int f1()
{
    return 1013;
}
} // namespace test_utility

TEST(Utility, FpWrapper)
{
    using namespace asbind20;

    constexpr auto wrapper = fp<&test_utility::f1>;
    static constexpr auto f1 = wrapper.get();
    EXPECT_EQ(f1(), 1013);
}

namespace test_utility
{
enum my_enum
{
    val_1 = 1,
    val_2 = 2
};
} // namespace test_utility

TEST(Utility, StaticEnumName)
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

TEST(Utility, Version)
{
    {
        std::string ver_str;
        ver_str += std::to_string(ASBIND20_VERSION_MAJOR);
        ver_str += '.';
        ver_str += std::to_string(ASBIND20_VERSION_MINOR);
        ver_str += '.';
        ver_str += std::to_string(ASBIND20_VERSION_PATCH);

        EXPECT_EQ(ver_str, ASBIND20_VERSION_STRING);
        EXPECT_THAT(asbind20::library_version(), ::testing::StartsWith(ver_str));
    }

    {
        std::string_view sv = AS_NAMESPACE_QUALIFIER asGetLibraryOptions();
        bool max_portability_found = sv.find("AS_MAX_PORTABILITY") != sv.npos;

        EXPECT_EQ(asbind20::has_max_portability(), max_portability_found);
    }
}

TEST(NameOf, Arithmetic)
{
    using namespace asbind20;
    using namespace std::literals;

    EXPECT_EQ(name_of<bool>(), "bool"sv);

    EXPECT_EQ(name_of<AS_NAMESPACE_QUALIFIER asINT8>(), "int8"sv);
    EXPECT_EQ(name_of<AS_NAMESPACE_QUALIFIER asINT16>(), "int16"sv);
    EXPECT_EQ(name_of<AS_NAMESPACE_QUALIFIER asINT32>(), "int"sv);
    EXPECT_EQ(name_of<AS_NAMESPACE_QUALIFIER asINT64>(), "int64"sv);

    EXPECT_EQ(name_of<AS_NAMESPACE_QUALIFIER asBYTE>(), "uint8"sv);
    EXPECT_EQ(name_of<AS_NAMESPACE_QUALIFIER asWORD>(), "uint16"sv);
    EXPECT_EQ(name_of<AS_NAMESPACE_QUALIFIER asDWORD>(), "uint"sv);
    EXPECT_EQ(name_of<AS_NAMESPACE_QUALIFIER asQWORD>(), "uint64"sv);

    EXPECT_EQ(name_of<float>(), "float"sv);
    EXPECT_EQ(name_of<double>(), "double"sv);
}

TEST(Meta, FixedString)
{
    using namespace asbind20;

    static_assert(util::fixed_string().empty());
    static_assert(util::fixed_string("int").size() == 3);
    static_assert(util::fixed_string("int").view() == "int");

    {
        util::fixed_string s = "int";
        EXPECT_EQ(s.size(), 3);
        EXPECT_STREQ(s.c_str(), "int");

        std::string str(s);
        EXPECT_EQ(str, "int");
    }

    {
        auto result = string_concat("void f()", util::fixed_string("{int,int}"));
        EXPECT_EQ(result, "void f(){int,int}");
    }

    {
        constexpr util::fixed_string hello = "hello";
        constexpr util::fixed_string world = " world";
        static_assert((hello + world).view() == "hello world");
        static_assert((hello + util::fixed_string()).view() == "hello");
        static_assert((util::fixed_string() + hello).view() == "hello");

        auto result = hello + world;
        EXPECT_EQ(result.size(), 11);
        EXPECT_STREQ(result.c_str(), "hello world");
    }

    {
        constexpr auto decl = meta::full_fixed_name_of<int&>();
        static_assert(decl.view() == "int&"); // Only for testing
    }

    {
        constexpr auto decl = meta::full_fixed_name_of<const int&>();
        static_assert(decl.view() == "const int&in");
    }
}

TEST(Utility, ScriptTypeTraits)
{
    using namespace asbind20;

    EXPECT_FALSE(is_floating_point(AS_NAMESPACE_QUALIFIER asTYPEID_VOID));
    EXPECT_FALSE(is_floating_point(AS_NAMESPACE_QUALIFIER asTYPEID_BOOL));
    EXPECT_TRUE(is_floating_point(AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT));
    EXPECT_TRUE(is_floating_point(AS_NAMESPACE_QUALIFIER asTYPEID_DOUBLE));

    EXPECT_EQ(
        is_integral(AS_NAMESPACE_QUALIFIER asTYPEID_BOOL),
        std::is_integral_v<bool>
    );
    EXPECT_EQ(
        is_signed(AS_NAMESPACE_QUALIFIER asTYPEID_BOOL),
        std::is_signed_v<bool> // false
    );
    EXPECT_EQ(
        is_unsigned(AS_NAMESPACE_QUALIFIER asTYPEID_BOOL),
        std::is_unsigned_v<bool> // true
    );
    EXPECT_TRUE(is_integral(AS_NAMESPACE_QUALIFIER asTYPEID_INT32));
    EXPECT_TRUE(is_integral(AS_NAMESPACE_QUALIFIER asTYPEID_UINT32));
    EXPECT_FALSE(is_integral(AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT));
    EXPECT_FALSE(is_integral(AS_NAMESPACE_QUALIFIER asTYPEID_DOUBLE));

    EXPECT_TRUE(is_signed(AS_NAMESPACE_QUALIFIER asTYPEID_INT8));
    EXPECT_TRUE(is_signed(AS_NAMESPACE_QUALIFIER asTYPEID_INT16));
    EXPECT_TRUE(is_signed(AS_NAMESPACE_QUALIFIER asTYPEID_INT32));
    EXPECT_TRUE(is_signed(AS_NAMESPACE_QUALIFIER asTYPEID_INT64));
    EXPECT_FALSE(is_signed(AS_NAMESPACE_QUALIFIER asTYPEID_UINT8));
    EXPECT_FALSE(is_signed(AS_NAMESPACE_QUALIFIER asTYPEID_UINT16));
    EXPECT_FALSE(is_signed(AS_NAMESPACE_QUALIFIER asTYPEID_UINT32));
    EXPECT_FALSE(is_signed(AS_NAMESPACE_QUALIFIER asTYPEID_UINT64));

    EXPECT_TRUE(is_unsigned(AS_NAMESPACE_QUALIFIER asTYPEID_UINT8));
    EXPECT_TRUE(is_unsigned(AS_NAMESPACE_QUALIFIER asTYPEID_UINT16));
    EXPECT_TRUE(is_unsigned(AS_NAMESPACE_QUALIFIER asTYPEID_UINT32));
    EXPECT_TRUE(is_unsigned(AS_NAMESPACE_QUALIFIER asTYPEID_UINT64));
    EXPECT_FALSE(is_unsigned(AS_NAMESPACE_QUALIFIER asTYPEID_INT8));
    EXPECT_FALSE(is_unsigned(AS_NAMESPACE_QUALIFIER asTYPEID_INT16));
    EXPECT_FALSE(is_unsigned(AS_NAMESPACE_QUALIFIER asTYPEID_INT32));
    EXPECT_FALSE(is_unsigned(AS_NAMESPACE_QUALIFIER asTYPEID_INT64));
}

namespace test_utility
{
struct pod_class
{};

// Self checks
static_assert(std::is_standard_layout_v<pod_class>);
static_assert(std::is_trivial_v<pod_class>);

class copyable_class
{
public:
    copyable_class(const copyable_class&) = default;

    int val;
};

class non_copyable
{
public:
    non_copyable(const non_copyable&) = delete;
};

union my_union
{
    int vi;
    float vf;
};

template <typename T>
void script_flags_test_helper()
{
    using namespace asbind20;
    EXPECT_EQ(
        meta::get_script_type_flags<T>(),
        AS_NAMESPACE_QUALIFIER asGetTypeTraits<T>()
    )
        << "T = " << ::testing::internal::GetTypeName<T>();
}
} // namespace test_utility

TEST(Utility, ScriptFlags)
{
    using namespace test_utility;


    script_flags_test_helper<pod_class>();
    script_flags_test_helper<copyable_class>();
    script_flags_test_helper<non_copyable>();
    script_flags_test_helper<my_union>();

    // Primitives
    script_flags_test_helper<int>();
    script_flags_test_helper<unsigned int>();
    script_flags_test_helper<float>();
    script_flags_test_helper<double>();

    // Arrays
    script_flags_test_helper<int[2]>();
    script_flags_test_helper<float[2]>();
}

static void output_info(std::ostream& os)
{
    if(asbind20::has_max_portability())
        os << "AS_MAX_PORTABILITY" << std::endl;

#ifdef ASBIND20_NO_EXCEPTIONS
    os << "ASBIND20_NO_EXCEPTIONS defined" << std::endl;
#else
    os << "ASBIND20_NO_EXCEPTIONS not defined" << std::endl;
#endif

#ifdef AS_USE_NAMESPACE
    os << "AS_USE_NAMESPACE defined" << std::endl;
#endif

#ifdef ASBIND20_HAS_STANDALONE_STDCALL
    os << "ASBIND20_HAS_STANDALONE_STDCALL defined" << std::endl;
#endif

#ifdef ASBIND20_HAS_STATIC_ENUM_NAME
    os << "ASBIND20_HAS_STATIC_ENUM_NAME: "
       << ASBIND20_HAS_STATIC_ENUM_NAME
       << std::endl;
#endif
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    // Output information so we can read them when CMake is configuring tests
    output_info(std::cerr);

    return RUN_ALL_TESTS();
}
