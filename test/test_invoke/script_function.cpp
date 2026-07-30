#include <gtest/gtest.h>
#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>

TEST(ScriptFunction, Ownership)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    auto* m = asbind20::create_module(engine, "test");
    m->AddScriptSection("test", "int test() { return 42; }");
    ASSERT_GE(m->Build(), 0);

    script_function<int()> f;
    EXPECT_THAT(f, ::testing::IsFalse());

    f.reset(m->GetFunctionByName("test"));
    EXPECT_THAT(f, ::testing::IsTrue());

    m->Discard();

    {
        request_context ctx(engine);
        auto result = f(ctx);

        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 42);
    }

    // By reference
    {
        script_function_ref<int()> rf = f;
        EXPECT_EQ(f.target(), rf.target());

        request_context ctx(engine);
        auto result = rf(ctx);

        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 42);

        script_function<int()> another = rf;
        EXPECT_EQ(another.target(), rf.target());
    }

    {
        auto another = f;
        EXPECT_EQ(another.target(), f.target());

        f.reset();
        EXPECT_THAT(f, ::testing::IsFalse());
        EXPECT_THAT(f.target(), ::testing::IsNull());
    }
}

TEST(ScriptMethod, Ownership)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    auto* m = asbind20::create_module(engine, "test");
    m->AddScriptSection(
        "test",
        "class foo\n"
        "{\n"
        "    int test() const { return 42; }\n"
        "};\n"
    );
    ASSERT_GE(m->Build(), 0);

    auto foo_t = script_typeinfo(m->GetTypeInfoByName("foo"));
    ASSERT_THAT(foo_t, ::testing::NotNull());

    request_context ctx(engine);
    auto foo = instantiate_class(ctx, foo_t);
    EXPECT_TRUE(foo);

    script_method<int()> test(foo_t->GetMethodByName("test"));
    ASSERT_THAT(test, ::testing::NotNull());

    m->Discard();

    {
        auto result = test(ctx, foo);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 42);
    }

    {
        script_method_ref<int()> rf = test;
        EXPECT_EQ(test.target(), rf.target());

        auto result = rf(ctx, foo);

        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 42);

        script_method<int()> another = rf;
        EXPECT_EQ(another.target(), rf.target());
    }

    {
        auto another = test;
        EXPECT_EQ(another.target(), test.target());

        test.reset();
        EXPECT_THAT(test, ::testing::IsFalse());
        EXPECT_THAT(test.target(), ::testing::IsNull());
    }
}
