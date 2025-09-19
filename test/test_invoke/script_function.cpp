#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>

TEST(script_function, ownership)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    auto* m = engine->GetModule(
        "test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection("test", "int test() { return 42; }");
    ASSERT_GE(m->Build(), 0);

    script_function<int()> f;
    EXPECT_FALSE(f);

    f.reset(m->GetFunctionByName("test"));
    EXPECT_TRUE(f);

    m->Discard();

    {
        request_context ctx(engine);
        auto result = f(ctx);

        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 42);
    }

    // By reference
    {
        script_function_ref<int()> rf = f;
        EXPECT_EQ(f.target(), rf.target());

        request_context ctx(engine);
        auto result = rf(ctx);

        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 42);

        script_function<int()> another = rf;
        EXPECT_EQ(another.target(), rf.target());
    }

    {
        auto another = f;
        EXPECT_EQ(another.target(), f.target());

        f.reset();
        EXPECT_FALSE(f);
        EXPECT_EQ(f.target(), nullptr);
    }
}

TEST(script_method, ownership)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    auto* m = engine->GetModule(
        "test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "test",
        "class foo\n"
        "{\n"
        "    int test() const { return 42; }\n"
        "};\n"
    );
    ASSERT_GE(m->Build(), 0);

    auto foo_t = script_typeinfo(m->GetTypeInfoByName("foo"));
    ASSERT_TRUE(foo_t);

    request_context ctx(engine);
    auto foo = instantiate_class(ctx, foo_t);
    EXPECT_TRUE(foo);

    script_method<int()> test(foo_t->GetMethodByName("test"));
    ASSERT_TRUE(test);

    m->Discard();

    {
        auto result = test(ctx, foo);
        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 42);
    }

    {
        script_method_ref<int()> rf = test;
        EXPECT_EQ(test.target(), rf.target());

        auto result = rf(ctx, foo);

        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 42);

        script_method<int()> another = rf;
        EXPECT_EQ(another.target(), rf.target());
    }

    {
        auto another = test;
        EXPECT_EQ(another.target(), test.target());

        test.reset();
        EXPECT_FALSE(test);
        EXPECT_EQ(test.target(), nullptr);
    }
}
