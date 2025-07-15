#include <shared_test_lib.hpp>
#include "c_api.hpp"

namespace test_bind
{
static void register_c_api_test(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    ref_class<opaque_structure>(engine, "opaque")
        .factory_function("", &create_opaque)
        .factory_function("int", &create_opaque_with)
        .addref(&opaque_addref)
        .release(&release_opaque)
        .method("int get_data() const property", &opaque_get_data)
        .method("void set_data(int) property", &opaque_set_data);
}

static void register_c_api_test(asbind20::use_generic_t, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    ref_class<opaque_structure, true>(engine, "opaque")
        .factory_function("", fp<&create_opaque>)
        .factory_function("int", fp<&create_opaque_with>)
        .addref(fp<&opaque_addref>)
        .release(fp<&release_opaque>)
        .method("int get_data() const property", fp<&opaque_get_data>)
        .method("void set_data(int) property", fp<&opaque_set_data>);
}

static void test_c_api(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "test_c_api", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    ASSERT_TRUE(m);

    m->AddScriptSection(
        "test_c_api",
        "int test0()\n"
        "{\n"
        "    opaque o;\n"
        "    opaque@ r = o;\n"
        "    o.data = 42;\n"
        "    return r.data;\n"
        "}\n"
        "int test1()\n"
        "{\n"
        "    opaque o(1013);\n"
        "    opaque@ r = o;\n"
        "    return r.data;\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* test0 = m->GetFunctionByName("test0");
        ASSERT_TRUE(test0);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, test0);

        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 42);
    }

    {
        auto* test1 = m->GetFunctionByName("test1");
        ASSERT_TRUE(test1);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, test1);

        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 1013);
    }
}
} // namespace test_bind

TEST(c_api, native)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    test_bind::register_c_api_test(engine);
    test_bind::test_c_api(engine);
}

TEST(c_api, generic)
{
    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    test_bind::register_c_api_test(asbind20::use_generic, engine);
    test_bind::test_c_api(engine);
}
