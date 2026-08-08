#include <asbind_test/framework.hpp>

namespace test_bind
{
static int f_global()
{
    return 3;
}

static int f_ns0()
{
    return 4;
}

static int f_ns1()
{
    return 7;
}

static void check_func_with_ns(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "test");
    m->SetDefaultNamespace("");
    m->AddScriptSection(
        "test",
        "void run()\n"
        "{\n"
        "    assert(f() == 3);\n"
        "    assert(ns0::f() == 4);\n"
        "    assert(ns0::ns1::f() == 7);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* run = m->GetFunctionByName("run");
    ASSERT_THAT(run, ::testing::NotNull());
    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<void>(ctx, run);
    ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
}
} // namespace test_bind

TEST(ScriptNamespace, SetDefault)
{
    using asbind20::fp;
    using asbind20::namespace_;

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);
    asbind_test::setup_script_assertion(engine);

    namespace_ unused_ns(engine, "unused");
    {
        namespace_ global_ns(engine);
        EXPECT_STREQ(engine->GetDefaultNamespace(), "");
        asbind20::global<true>(engine)
            .function("int f()", fp<&test_bind::f_global>);

        namespace_ ns0(engine, "ns0");
        asbind20::global<true>(engine)
            .function("int f()", fp<&test_bind::f_ns0>);

        {
            // ns0::ns1
            namespace_ ns1(engine, "ns1");
            asbind20::global<true>(engine)
                .function("int f()", fp<&test_bind::f_ns1>);
            EXPECT_STREQ(engine->GetDefaultNamespace(), "ns0::ns1");
        }
        EXPECT_STREQ(engine->GetDefaultNamespace(), "ns0");
    }
    EXPECT_STREQ(engine->GetDefaultNamespace(), "unused");

    test_bind::check_func_with_ns(engine);
}
