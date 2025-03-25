#include <shared_test_lib.hpp>
#include <asbind20/operators.hpp>
#include <asbind20/ext/stdstring.hpp>

namespace test_bind
{
class my_pair2i
{
public:
    my_pair2i() = default;
    my_pair2i(const my_pair2i&) = default;

    my_pair2i(int a, int b)
        : first(a), second(b) {}

    my_pair2i& operator=(const my_pair2i&) = default;

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
        "int test5() { pair2i p1 = {1, 2}; pair2i p2 = {3, 4}; return p1 * p2; }"
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
}
} // namespace test_bind

using std::string;

TEST(test_operators, my_pair2i_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    ext::register_std_string(engine);

    value_class<test_bind::my_pair2i>(
        engine,
        "pair2i",
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS
    )
        .behaviours_by_traits()
        .list_constructor<int>("int,int", use_policy<policies::apply_to<2>>)
        .use((const_this + param<int>)->return_<int>())
        .use((param<int> + const_this)->return_<int>())
        .use((const_this + const_this)->return_<int>())
        .use((const_this * const_this)->return_<int>())
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

    test_bind::run_pair2i_test_script(engine);
}

TEST(test_operators, my_pair2i_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    ext::register_std_string(engine);

    value_class<test_bind::my_pair2i, true>(engine, "pair2i")
        .behaviours_by_traits()
        .list_constructor<int>("int,int", use_policy<policies::apply_to<2>>)
        .use((const_this + param<int>)->return_<int>())
        .use((param<int> + const_this)->return_<int>())
        .use((const_this + const_this)->return_<int>())
        .use((const_this * const_this)->return_<int>())
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

    test_bind::run_pair2i_test_script(engine);
}

TEST(test_operators, my_pair2i_native_with_decl)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    ext::register_std_string(engine);

    value_class<test_bind::my_pair2i>(engine, "pair2i")
        .behaviours_by_traits()
        .list_constructor<int>("int,int", use_policy<policies::apply_to<2>>)
        .use((const_this + param<int>("int"))->return_<int>("int"))
        .use((param<int>("int") + const_this)->return_<int>("int"))
        .use((const_this + const_this)->return_<int>("int"))
        .use((const_this * const_this)->return_<int>("int"))
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

    test_bind::run_pair2i_test_script(engine);
}

TEST(test_operators, my_pair2i_generic_with_decl)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    ext::register_std_string(engine);

    value_class<test_bind::my_pair2i, true>(engine, "pair2i")
        .behaviours_by_traits()
        .list_constructor<int>("int,int", use_policy<policies::apply_to<2>>)
        .use((const_this + param<int>("int"))->return_<int>("int"))
        .use((param<int>("int") + const_this)->return_<int>("int"))
        .use((const_this + const_this)->return_<int>("int"))
        .use((const_this * const_this)->return_<int>("int"))
        .use((const_this + param<const string&>("const string&in"))->return_<string>("string"))
        .use((param<const string&>("const string&in") + const_this)->return_<string>("string"));

    test_bind::run_pair2i_test_script(engine);
}
