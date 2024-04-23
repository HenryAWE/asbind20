#include <gtest/gtest.h>
#include <asbind20/meta.hpp>

TEST(static_string, constructor)
{
    using asbind20::static_string;

    {
        constexpr static_string ss("static string");
        EXPECT_EQ(ss.length(), 13);
        constexpr auto first_word = ss.substr<0, 6>();
        EXPECT_EQ(first_word.length(), 6);
        EXPECT_STREQ(first_word.c_str(), "static");

        constexpr auto last_word = ss.substr<7>();
        static_assert(last_word.to_string_view() == "string");
        EXPECT_STREQ(last_word.c_str(), "string");
    }

    {
        constexpr static_string empty_str;
        static_assert(empty_str.empty());
        EXPECT_TRUE(empty_str.empty());
    }

    {
        constexpr static_string ch('c');
        static_assert(ch.length() == 1);
        static_assert(ch.get<0>() == 'c');
        EXPECT_STREQ(ch.c_str(), "c");
    }
}

TEST(static_string, append)
{
    using asbind20::static_string;
    constexpr static_string ss1("hello");
    constexpr static_string ss2(" world");
    constexpr auto ss3 = ss1 + ss2;

    static_assert(ss3.to_string_view() == "hello world");
    EXPECT_STREQ(ss3.c_str(), "hello world");
}

TEST(static_string, concat)
{
    using asbind20::static_concat;
    using asbind20::static_string;

    {
        constexpr auto ss = static_concat<static_string("a")>();
        static_assert(ss.to_string_view() == "a");
        EXPECT_STREQ(ss.c_str(), "a");
    }

    {
        constexpr auto ss = static_concat<static_string("a"), static_string("b")>();
        static_assert(ss.to_string_view() == "ab");
        EXPECT_STREQ(ss.c_str(), "ab");

        static_assert(ss.get<0>() == 'a');
        static_assert(ss.get<1>() == 'b');
    }

    {
        constexpr auto ss = static_concat<static_string("a"), static_string("b"), static_string("c")>();
        static_assert(ss.to_string_view() == "abc");
        EXPECT_STREQ(ss.c_str(), "abc");

        static_assert(ss.get<0>() == 'a');
        static_assert(ss.get<1>() == 'b');
        static_assert(ss.get<2>() == 'c');
    }
}

TEST(static_string, join)
{
    using asbind20::static_join;
    using asbind20::static_string;

    {
        constexpr auto ss = static_join<static_string(","), static_string("a")>();
        static_assert(ss.to_string_view() == "a");
        EXPECT_STREQ(ss.c_str(), "a");
    }

    {
        constexpr auto ss = static_join<static_string(","), static_string("a"), static_string("b")>();
        static_assert(ss.to_string_view() == "a,b");
        EXPECT_STREQ(ss.c_str(), "a,b");
    }

    {
        constexpr auto ss = static_join<static_string(","), static_string("a"), static_string("b"), static_string("c")>();
        static_assert(ss.to_string_view() == "a,b,c");
        EXPECT_STREQ(ss.c_str(), "a,b,c");
    }
}

TEST(meta, name_of)
{
    using namespace asbind20;

    {
        constexpr auto name = name_of<int>();
        static_assert(name.to_string_view() == "int");
        EXPECT_STREQ(name.c_str(), "int");
    }

    {
        constexpr auto name = name_of<std::string>();
        static_assert(name.to_string_view() == "string");
        EXPECT_STREQ(name.c_str(), "string");
    }
}

void check_compile_fp()
{
    using asbind20::function_traits;

    unsigned int (*fp)(float, int) = nullptr;

    using func_t = function_traits<decltype(fp)>;
    static_assert(!func_t::is_method());
    static_assert(!func_t::is_noexcept());
    static_assert(std::is_same_v<func_t::return_type, unsigned int>);
    static_assert(std::is_same_v<func_t::arg_type<0>, float>);
    static_assert(std::is_same_v<func_t::arg_type<1>, int>);
    static_assert(func_t::arg_count() == 2);
}

void check_compile_fp_noexcept()
{
    using asbind20::function_traits;

    unsigned int (*fp)(float, int) noexcept = nullptr;

    using func_t = function_traits<decltype(fp)>;
    static_assert(!func_t::is_method());
    static_assert(func_t::is_noexcept());
    static_assert(std::is_same_v<func_t::return_type, unsigned int>);
    static_assert(std::is_same_v<func_t::arg_type<0>, float>);
    static_assert(std::is_same_v<func_t::arg_type<1>, int>);
    static_assert(func_t::arg_count() == 2);
}

void check_compile_member_fp()
{
    using asbind20::function_traits;

    struct my_type
    {
        int func(float arg)
        {
            return 0;
        }
    };

    using func_t = function_traits<decltype(&my_type::func)>;
    static_assert(func_t::is_method());
    static_assert(std::is_same_v<func_t::class_type, my_type>);
    static_assert(std::is_same_v<func_t::return_type, int>);
    static_assert(std::is_same_v<func_t::arg_type<0>, float>);
    static_assert(func_t::arg_count() == 1);
}

void check_compile_lambda()
{
    using asbind20::function_traits;

    auto lambda = [](int, float, double) -> unsigned int
    {
        return 0;
    };
    using lambda_t = decltype(lambda);

    using func_t = function_traits<lambda_t>;
    static_assert(func_t::is_method());
    static_assert(std::is_same_v<func_t::class_type, lambda_t>);
    static_assert(std::is_same_v<func_t::return_type, unsigned int>);
    static_assert(std::is_same_v<func_t::arg_type<0>, int>);
    static_assert(std::is_same_v<func_t::arg_type<1>, float>);
    static_assert(func_t::arg_count() == 3);
    static_assert(std::is_same_v<func_t::first_arg_type, int>);
    static_assert(std::is_same_v<func_t::last_arg_type, double>);
}

void check_func_empty_arg()
{
    using func_t = asbind20::function_traits<int()>;

    static_assert(func_t::arg_count() == 0);
    static_assert(std::is_void_v<func_t::first_arg_type>);
    static_assert(std::is_void_v<func_t::last_arg_type>);
}

namespace test_meta
{
int my_func(int)
{
    return 0;
}
} // namespace test_meta

TEST(function_tratis, static_decl)
{
    using asbind20::function_traits;
    using asbind20::static_string;

    {
        using func_t = function_traits<int()>;

        constexpr auto decl = func_t::static_decl<static_string("f")>();

        static_assert(decl.to_string_view() == "int f()");
        EXPECT_STREQ(decl.c_str(), "int f()");
    }

    {
        using func_t = function_traits<int(int)>;

        constexpr auto decl = func_t::static_decl<static_string("f")>();

        static_assert(decl.to_string_view() == "int f(int)");
        EXPECT_STREQ(decl.c_str(), "int f(int)");
    }

    {
        using func_t = function_traits<int(int, int)>;

        constexpr auto decl = func_t::static_decl<static_string("f")>();

        static_assert(decl.to_string_view() == "int f(int,int)");
        EXPECT_STREQ(decl.c_str(), "int f(int,int)");
    }

    {
        using test_meta::my_func;

        constexpr auto decl = ASBIND_FUNC_DECL(my_func);

        static_assert(decl.to_string_view() == "int my_func(int)");
        EXPECT_STREQ(decl.c_str(), "int my_func(int)");
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
