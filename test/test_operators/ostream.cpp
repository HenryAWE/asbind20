#include <asbind_test/framework.hpp>
#include <gmock/gmock.h>
#include <asbind20/operators.hpp>
#include <sstream>

namespace test_operators
{
template <bool UseGeneric>
static void register_ostream(std::ostream& os, asbind20::engine_pointer engine)
{
    using namespace asbind20;
    using std::ostream;

    os << std::boolalpha;

    ref_class<std::ostream& (*)(std::ostream&)> endl_t(
        engine,
        "endl_t",
        AS_NAMESPACE_QUALIFIER asOBJ_NOCOUNT
    );

    ref_class<ostream, UseGeneric>(
        engine,
        "ostream",
        AS_NAMESPACE_QUALIFIER asOBJ_NOCOUNT
    )
        .use(_this << param<bool>)
        .use(_this << param<int>)
        .use(_this << param<float>)
        .use(_this << param<decltype(endl_t)::class_type>("const endl_t&in"))
        .use(_this << param<const std::string&>("const string&in"));

    global<UseGeneric>(engine)
        .property("ostream cout", os)
        .property("endl_t endl", std::endl<char, std::char_traits<char>>);
}

static void run_ostream_test_script(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "test_ostream");
    m->AddScriptSection(
        "test_ostream",
        "void main()\n"
        "{\n"
        "    cout << true << endl;\n"
        "    cout << 10 << 13 << endl;\n"
        "    cout << 3.14f << endl;\n"
        "    cout << \"hello\";\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    using namespace asbind20;

    auto* f = m->GetFunctionByName("main");
    ASSERT_THAT(f, ::testing::NotNull());

    request_context ctx(engine);

    auto result = script_invoke<void>(ctx, f);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
}

static void check_ostream_output(std::string_view output)
{
    using ::testing::HasSubstr;

    EXPECT_THAT(output, HasSubstr("true"));
    EXPECT_THAT(output, HasSubstr("1013"));
    EXPECT_THAT(output, HasSubstr("3.14"));
    // Check for endl
    EXPECT_THAT(output, HasSubstr("\n"));
}
} // namespace test_operators

TEST(TestOperators, OStreamNative)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    asbind_test::setup_script_string(engine, false);

    // Use "static" to guarantee lifetime
    static std::ostringstream oss;
    oss.str(std::string());
    test_operators::register_ostream<false>(oss, engine);
    test_operators::run_ostream_test_script(engine);

    test_operators::check_ostream_output(oss.str());
}

TEST(TestOperators, OStreamGeneric)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    asbind_test::setup_script_string(engine, true);

    // Use "static" to guarantee lifetime
    static std::ostringstream oss;
    oss.str(std::string());
    test_operators::register_ostream<true>(oss, engine);
    test_operators::run_ostream_test_script(engine);

    test_operators::check_ostream_output(oss.str());
}
