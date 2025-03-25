#include <shared_test_lib.hpp>
#include <asbind20/operators.hpp>
#include <asbind20/ext/stdstring.hpp>

namespace test_operators
{
class my_pair2i
{
public:
    my_pair2i() = default;
    my_pair2i(const my_pair2i&) = default;

    my_pair2i(int a, int b)
        : first(a), second(b) {}

    my_pair2i& operator=(const my_pair2i&) = default;

    int operator-()
    {
        return -1;
    }

    int operator-() const
    {
        return -2; // To distinguish with the previous one (non-const version)
    }

    int operator~() const
    {
        return -3;
    }

    std::string to_str() const
    {
        return asbind20::string_concat(
            '(', std::to_string(first), ", ", std::to_string(second), ')'
        );
    }

    my_pair2i& operator+=(int val)
    {
        first += val;
        second += val;
        return *this;
    }

    int operator-=(int val) const
    {
        return -val;
    }

    friend int operator+(const my_pair2i& lhs, int val)
    {
        my_pair2i tmp = lhs;
        tmp += val;
        return tmp.first + tmp.second;
    }

    friend int operator+(int val, const my_pair2i& rhs)
    {
        my_pair2i tmp = rhs;
        tmp += val + 1; // Add 1 to distinguish this function with the previous one
        return tmp.first + tmp.second;
    }

    friend std::string operator+(const my_pair2i& lhs, const std::string& str)
    {
        return asbind20::string_concat(
            lhs.to_str(), ": ", str
        );
    }

    friend std::string operator+(const std::string& str, const my_pair2i& rhs)
    {
        return asbind20::string_concat(
            str, ": ", rhs.to_str()
        );
    }

    friend int operator+(const my_pair2i& lhs, const my_pair2i& rhs)
    {
        my_pair2i tmp = lhs;
        tmp += rhs.first + rhs.second;
        return tmp.first + tmp.second;
    }

    int operator++(int) const
    {
        return *this + 1;
    }

    int operator++(int)
    {
        return *this + 2; // To distinguish with the previous one (const version)
    }

    friend int operator*(const my_pair2i& lhs, const my_pair2i& rhs)
    {
        return lhs.first * rhs.first + lhs.second * rhs.second;
    }

    int first;
    int second;
};

static void run_pair2i_test_script(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "test_pair2i", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );

    m->AddScriptSection(
        "test_pair2i",
        "int test0() { pair2i p = {1, 2}; return p + 2; }\n"
        "int test1() { pair2i p = {1, 2}; return 2 + p; }\n"
        "int test2() { pair2i p1 = {1, 2}; pair2i p2 = {3, 4}; return p1 + p2; }\n"
        "string test3() { pair2i p = {1, 2}; return p + \"str\"; }\n"
        "string test4() { pair2i p = {1, 2}; return \"str\" + p; }"
        "int test5() { pair2i p1 = {1, 2}; pair2i p2 = {3, 4}; return p1 * p2; }\n"
        "int test6() { pair2i p = {1, 2}; return -p; }\n"
        "int test7() { pair2i p = {1, 2}; return p++; }\n"
        "int test8() { pair2i p = {1, 2}; return ~p; }\n"
        "pair2i test9() { pair2i p = {1, 2}; return p += 1; }\n"
        "int test10() { pair2i p = {1, 2}; return p -= -42; }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* f = m->GetFunctionByName("test0");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 7);
    }

    {
        auto* f = m->GetFunctionByName("test1");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 9);
    }

    {
        auto* f = m->GetFunctionByName("test2");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 17);
    }

    {
        auto* f = m->GetFunctionByName("test3");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::string>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), "(1, 2): str");
    }

    {
        auto* f = m->GetFunctionByName("test4");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::string>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), "str: (1, 2)");
    }

    {
        auto* f = m->GetFunctionByName("test5");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 11);
    }

    {
        auto* f = m->GetFunctionByName("test6");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), -1);
    }

    {
        auto* f = m->GetFunctionByName("test7");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 5);
    }

    {
        auto* f = m->GetFunctionByName("test8");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), -3);
    }

    {
        auto* f = m->GetFunctionByName("test9");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<my_pair2i>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value().first, 2);
        EXPECT_EQ(result.value().second, 3);
    }

    {
        auto* f = m->GetFunctionByName("test10");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 42);
    }
}
} // namespace test_operators

using std::string;

TEST(test_operators, my_pair2i_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    ext::register_std_string(engine);

    value_class<test_operators::my_pair2i>(
        engine,
        "pair2i",
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS
    )
        .behaviours_by_traits()
        .list_constructor<int>("int,int", use_policy<policies::apply_to<2>>)
        .use(-_this)
        .use(~const_this)
        .use(const_this++)
        .use(_this += param<int>)
        .use(_this -= param<int>)
        .use(const_this + param<int>)
        .use(param<int> + const_this)
        .use(const_this + const_this)
        .use(const_this * const_this)
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

    test_operators::run_pair2i_test_script(engine);
}

TEST(test_operators, my_pair2i_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    ext::register_std_string(engine);

    value_class<test_operators::my_pair2i, true>(engine, "pair2i")
        .behaviours_by_traits()
        .list_constructor<int>("int,int", use_policy<policies::apply_to<2>>)
        .use(-_this)
        .use(~const_this)
        .use(const_this++)
        .use(_this += param<int>)
        .use(_this -= param<int>)
        .use(const_this + param<int>)
        .use(param<int> + const_this)
        .use(const_this + const_this)
        .use(const_this * const_this)
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

    test_operators::run_pair2i_test_script(engine);
}

TEST(test_operators, my_pair2i_native_with_decl)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    ext::register_std_string(engine);

    value_class<test_operators::my_pair2i>(engine, "pair2i")
        .behaviours_by_traits()
        .list_constructor<int>("int,int", use_policy<policies::apply_to<2>>)
        .use((-_this)->return_<int>("int"))
        .use((~const_this)->return_<int>("int"))
        .use((const_this++)->return_<int>("int"))
        .use((_this += param<int>)->return_<test_operators::my_pair2i&>("pair2i&"))
        .use((_this -= param<int>)->return_<int>("int"))
        .use((const_this + param<int>("int"))->return_<int>("int"))
        .use((param<int>("int") + const_this)->return_<int>("int"))
        .use((const_this + const_this)->return_<int>("int"))
        .use((const_this * const_this)->return_<int>("int"))
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

    test_operators::run_pair2i_test_script(engine);
}

TEST(test_operators, my_pair2i_generic_with_decl)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    ext::register_std_string(engine);

    value_class<test_operators::my_pair2i, true>(engine, "pair2i")
        .behaviours_by_traits()
        .list_constructor<int>("int,int", use_policy<policies::apply_to<2>>)
        .use((-_this)->return_<int>("int"))
        .use((~const_this)->return_<int>("int"))
        .use((const_this++)->return_<int>("int"))
        .use((_this += param<int>)->return_<test_operators::my_pair2i&>("pair2i&"))
        .use((_this -= param<int>)->return_<int>("int"))
        .use((const_this + param<int>("int"))->return_<int>("int"))
        .use((param<int>("int") + const_this)->return_<int>("int"))
        .use((const_this + const_this)->return_<int>("int"))
        .use((const_this * const_this)->return_<int>("int"))
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

    test_operators::run_pair2i_test_script(engine);
}
