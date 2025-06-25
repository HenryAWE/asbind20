#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/operators.hpp>
#include <asbind20/ext/stdstring.hpp>
#include <iostream>

namespace test_operators
{
template <bool UseGeneric>
static void register_ostream(std::ostream& os, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;
    using std::ostream;

    os << std::boolalpha;

    ref_class<std::ostream&(*)(std::ostream&)> endl_t(
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

static void run_ostream_test_script(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "test_ostream", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
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
    ASSERT_TRUE(f);

    request_context ctx(engine);

    ::testing::internal::CaptureStdout();
    auto result = script_invoke<void>(ctx, f);
    std::string output = testing::internal::GetCapturedStdout();
    ASSERT_TRUE(asbind_test::result_has_value(result));

    EXPECT_EQ(
        output,
        "true\n"
        "1013\n"
        "3.14\n"
        "hello"
    );
}
} // namespace test_operators

TEST(test_operators, ostream_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    ext::register_std_string(engine, true, false);

    test_operators::register_ostream<false>(std::cout, engine);
    test_operators::run_ostream_test_script(engine);
}

TEST(test_operators, ostream_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    ext::register_std_string(engine, true, true);

    test_operators::register_ostream<true>(std::cout, engine);
    test_operators::run_ostream_test_script(engine);
}
