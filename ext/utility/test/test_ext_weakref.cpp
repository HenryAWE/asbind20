#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/assert.hpp>
#include <asbind20/ext/weakref.hpp>

namespace test_ext_weakref
{
static void setup_env(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    asbind_test::setup_message_callback(engine);
    asbind20::ext::register_script_assert(
        engine,
        [](std::string_view msg)
        {
            FAIL() << "weakref assertion failed: " << msg;
        }
    );
}

static void check_weakref(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule("test_weakref", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);
    m->AddScriptSection(
        "test_weakref",
        "class test{};\n"
        "void main()\n"
        "{\n"
        "    test@ t = test();\n"
        "    weakref<test> r(t);\n"
        "    assert(r.get() !is null);\n"
        "    const_weakref<test> cr;\n"
        "    @cr = r;\n"
        "    assert(cr.get() !is null);\n"
        "    @t = null;\n"
        "    assert(r.get() is null);\n"
        "    assert(cr.get() is null);\n"
        "    @t = test();\n"
        "    @cr = t;\n"
        "    assert(cr.get() !is null); \n"
        "    const test@ ct = cr; \n"
        "    assert(ct !is null); \n"
        "    assert(cr !is null); \n"
        "    assert(cr is ct); \n"
        "    @cr = null; \n"
        "    assert(cr is null);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByDecl("void main()");
    ASSERT_NE(f, nullptr);

    {
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));
    }
}
} // namespace test_ext_weakref

TEST(ext_weakref, weakref_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = make_script_engine();
    test_ext_weakref::setup_env(engine);
    ext::register_weakref(engine, false);

    test_ext_weakref::check_weakref(engine);
}

TEST(ext_weakref, weakref_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    test_ext_weakref::setup_env(engine);
    ext::register_weakref(engine, true);

    test_ext_weakref::check_weakref(engine);
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
