#include <asbind20/ext/assert.hpp>
#include <shared_test_lib.hpp>

// Template function is added in AngelScript 2.38
#if ANGELSCRIPT_VERSION >= 23800

namespace test_bind
{
static void temp_f(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
{
    int tid = gen->GetArgTypeId(0);

    switch(tid)
    {
    case AS_NAMESPACE_QUALIFIER asTYPEID_INT32:
        asbind20::set_generic_return<int>(gen, 42);
        break;

    case AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT:
        asbind20::set_generic_return<float>(gen, 3.14f);
        break;

    default:
        asbind20::set_script_exception("unreachable");
    }
}
} // namespace test_bind

TEST(test_bind, temp_func)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    // Some code will intentionally trigger exception
    asbind_test::setup_message_callback(engine, false);

    global(engine)
        .function("T temp_f<T>(T val)", &test_bind::temp_f);

    auto* m = engine->GetModule(
        "temp_func", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "temp_func",
        "int test0() { return temp_f<int>(0); }\n"
        "float test1() { return temp_f<float>(0.0f); }\n"
        "double test2() { return temp_f<double>(0.0); }\n"
    );
    ASSERT_GE(m->Build(), 0);

    {
        request_context ctx(engine);
        auto* f = m->GetFunctionByName("test0");
        ASSERT_TRUE(f);

        auto result = script_invoke<int>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 42);
    }

    {
        request_context ctx(engine);
        auto* f = m->GetFunctionByName("test1");
        ASSERT_TRUE(f);

        auto result = script_invoke<float>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_FLOAT_EQ(result.value(), 3.14f);
    }

    {
        request_context ctx(engine);
        auto* f = m->GetFunctionByName("test2");
        ASSERT_TRUE(f);

        // Will trigger exception
        auto result = script_invoke<double>(ctx, f);
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);
        EXPECT_STREQ(ctx->GetExceptionString(), "unreachable");
    }
}

#endif
