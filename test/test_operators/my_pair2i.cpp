#include <asbind_test/framework.hpp>
#include <asbind20/operators.hpp>

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

    std::strong_ordering operator<=>(const my_pair2i& rhs) const = default;

    friend std::strong_ordering operator<=>(const my_pair2i& lhs, int rhs)
    {
        return lhs <=> my_pair2i(rhs, rhs);
    }

    friend std::strong_ordering operator<=>(int lhs, const my_pair2i& rhs)
    {
        return my_pair2i(lhs, lhs) <=> rhs;
    }

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

    int operator[](int idx) const
    {
        return idx;
    }

    my_pair2i operator[](const my_pair2i& p)
    {
        my_pair2i result = *this;
        result.first += p.first;
        result.second += p.second;

        return result;
    }

    int first;
    int second;
};

static void run_pair2i_test_script(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "test_pair2i");

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
        "int test10() { pair2i p = {1, 2}; return p -= -42; }\n"
        "int test11() { const pair2i p = {1, 2}; return p[1013]; }\n"
        "pair2i test12() { pair2i p1 = {1, 2}; pair2i p2 = {3, 4}; return p1[p2]; }"
        "bool test13() { pair2i p1 = {1, 3}; return p1 < 2; }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* f = m->GetFunctionByName("test0");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 7);
    }

    {
        auto* f = m->GetFunctionByName("test1");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 9);
    }

    {
        auto* f = m->GetFunctionByName("test2");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 17);
    }

    {
        auto* f = m->GetFunctionByName("test3");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::string>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), "(1, 2): str");
    }

    {
        auto* f = m->GetFunctionByName("test4");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::string>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), "str: (1, 2)");
    }

    {
        auto* f = m->GetFunctionByName("test5");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 11);
    }

    {
        auto* f = m->GetFunctionByName("test6");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), -1);
    }

    {
        auto* f = m->GetFunctionByName("test7");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 5);
    }

    {
        auto* f = m->GetFunctionByName("test8");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), -3);
    }

    {
        auto* f = m->GetFunctionByName("test9");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<my_pair2i>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value().first, 2);
        EXPECT_EQ(result.value().second, 3);
    }

    {
        auto* f = m->GetFunctionByName("test10");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 42);
    }

    {
        auto* f = m->GetFunctionByName("test11");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value(), 1013);
    }

    {
        auto* f = m->GetFunctionByName("test12");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<my_pair2i>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_EQ(result.value().first, 4);
        EXPECT_EQ(result.value().second, 6);
    }

    {
        auto* f = m->GetFunctionByName("test13");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<bool>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));

        EXPECT_TRUE(result.value());
    }
}
} // namespace test_operators

using std::string;

static const AS_NAMESPACE_QUALIFIER asQWORD pair2i_flags =
    AS_NAMESPACE_QUALIFIER asOBJ_POD |
    AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS |
    AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS;

TEST(TestOperators, MyPair2iNative)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    asbind_test::setup_script_string(engine, false);

    value_class<test_operators::my_pair2i>(
        engine,
        "pair2i",
        pair2i_flags
    )
        .behaviours_by_traits(pair2i_flags)
        .list_constructor<int>("int,int", use_policy<policies::apply_to<2>>)
        .use(-_this)
        .use(~const_this)
        .use(const_this++)
        .use(const_this <=> param<int>)
        .use(_this += param<int>)
        .use(_this -= param<int>)
        .use(const_this[param<int>])
        .use(_this[const_this])
        .use(const_this + param<int>)
        .use(param<int> + const_this)
        .use(const_this + const_this)
        .use(const_this * const_this)
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

    test_operators::run_pair2i_test_script(engine);
}

TEST(TestOperators, MyPair2iGeneric)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    asbind_test::setup_script_string(engine, true);

    value_class<test_operators::my_pair2i, true>(engine, "pair2i", pair2i_flags)
        .behaviours_by_traits(pair2i_flags)
        .list_constructor<int>("int,int", use_policy<policies::apply_to<2>>)
        .use(-_this)
        .use(~const_this)
        .use(const_this++)
        .use(const_this <=> param<int>)
        .use(_this += param<int>)
        .use(_this -= param<int>)
        .use(const_this[param<int>])
        .use(_this[const_this])
        .use(const_this + param<int>)
        .use(param<int> + const_this)
        .use(const_this + const_this)
        .use(const_this * const_this)
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

    test_operators::run_pair2i_test_script(engine);
}

TEST(TestOperators, MyPair2iNativeWithDecl)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    asbind_test::setup_script_string(engine, false);

    value_class<test_operators::my_pair2i>(engine, "pair2i", pair2i_flags)
        .behaviours_by_traits(pair2i_flags)
        .list_constructor<int>("int,int", use_policy<policies::apply_to<2>>)
        .use((-_this)->return_<int>("int"))
        .use((~const_this)->return_<int>("int"))
        .use((const_this++)->return_<int>("int"))
        .use((const_this <=> param<int>)->return_<int>())
        .use((_this += param<int>)->return_<test_operators::my_pair2i&>("pair2i&"))
        .use((_this -= param<int>)->return_<int>("int"))
        .use(const_this[param<int>("int")]->return_<int>("int"))
        .use(_this[const_this]->return_<test_operators::my_pair2i>("pair2i"))
        .use((const_this + param<int>("int"))->return_<int>("int"))
        .use((param<int>("int") + const_this)->return_<int>("int"))
        .use((const_this + const_this)->return_<int>("int"))
        .use((const_this * const_this)->return_<int>("int"))
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

    test_operators::run_pair2i_test_script(engine);
}

TEST(TestOperators, MyPair2iGenericWithDecl)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    asbind_test::setup_script_string(engine, true);

    value_class<test_operators::my_pair2i, true>(engine, "pair2i", pair2i_flags)
        .behaviours_by_traits(pair2i_flags)
        .list_constructor<int>("int,int", use_policy<policies::apply_to<2>>)
        .use((-_this)->return_<int>("int"))
        .use((~const_this)->return_<int>("int"))
        .use((const_this++)->return_<int>("int"))
        .use((const_this <=> param<int>)->return_<int>())
        .use((_this += param<int>)->return_<test_operators::my_pair2i&>("pair2i&"))
        .use((_this -= param<int>)->return_<int>("int"))
        .use(const_this[param<int>("int")]->return_<int>("int"))
        .use(_this[const_this]->return_<test_operators::my_pair2i>("pair2i"))
        .use((const_this + param<int>("int"))->return_<int>("int"))
        .use((param<int>("int") + const_this)->return_<int>("int"))
        .use((const_this + const_this)->return_<int>("int"))
        .use((const_this * const_this)->return_<int>("int"))
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

    test_operators::run_pair2i_test_script(engine);
}
