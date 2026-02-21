#include <asbind_test/framework.hpp>
#include <asbind20/container/compare.hpp>

TEST(Compare, EmptyClass)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    auto* m = engine->GetModule(
        "foo", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "foo",
        "class foo{};"
    );
    ASSERT_GE(m->Build(), 0);
    auto* ti = m->GetTypeInfoByName("foo");
    ASSERT_NE(ti, nullptr);

    auto result = container::get_comparator(ti);
    {
        auto st = result.get_status();
        EXPECT_FALSE(st.good());
        EXPECT_FALSE(st);
        EXPECT_EQ(st.opCmp, AS_NAMESPACE_QUALIFIER asNO_FUNCTION);
        EXPECT_EQ(st.opEquals, AS_NAMESPACE_QUALIFIER asNO_FUNCTION);
    }
    EXPECT_FALSE(result);

    {
        auto cmp = result.get();
        EXPECT_EQ(cmp.get_opCmp(), nullptr);
        EXPECT_FALSE(cmp.has_opCmp());
        EXPECT_EQ(cmp.get_opEquals(), nullptr);
        EXPECT_FALSE(cmp.has_opEquals());
    }
}

TEST(Compare, ScriptClassEq)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    auto* m = engine->GetModule(
        "foo", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "foo",
        "class foo\n"
        "{\n"
        "    int value = 0;\n"
        "    foo(int v) { this.value = v; }\n"
        "    bool opEquals(const foo& rhs) const\n"
        "    { return this.value == rhs.value; }\n"
        "};\n"
        "foo@ create(int v) { return foo(v); }"
    );
    ASSERT_GE(m->Build(), 0);
    auto* ti = m->GetTypeInfoByName("foo");
    ASSERT_NE(ti, nullptr);

    auto result = container::get_comparator(ti);
    {
        auto st = result.get_status();
        EXPECT_TRUE(st.good());
        EXPECT_TRUE(st);
        EXPECT_EQ(st.opCmp, AS_NAMESPACE_QUALIFIER asNO_FUNCTION);
        EXPECT_EQ(st.opEquals, AS_NAMESPACE_QUALIFIER asSUCCESS);
    }
    EXPECT_TRUE(result);

    {
        auto* expected_func = ti->GetMethodByName("opEquals");
        ASSERT_NE(expected_func, nullptr);

        auto cmp = result.get();
        EXPECT_EQ(cmp.get_opCmp(), nullptr);
        EXPECT_EQ(cmp.get_opEquals(), expected_func);
    }

    script_function<AS_NAMESPACE_QUALIFIER asIScriptObject*(int)> create(
        m->GetFunctionByDecl("foo@ create(int)")
    );
    ASSERT_TRUE(create);

    // Case 1: Not equal
    {
        request_context ctx(engine);

        auto* lhs = create(ctx, -1).value();
        ASSERT_NE(lhs, nullptr);
        lhs->AddRef();
        auto* rhs = create(ctx, 1).value();
        ASSERT_NE(rhs, nullptr);
        rhs->AddRef();

        auto cmp = result.get();
        auto is_eq = cmp.get_opEquals()(ctx, lhs, rhs);

        ASBIND_TEST_EXPECT_INVOKE_RESULT(is_eq);
        EXPECT_FALSE(is_eq.value());
        lhs->Release();
        rhs->Release();
    }

    // Case 2: Equal
    {
        request_context ctx(engine);

        auto* lhs = create(ctx, 1).value();
        ASSERT_NE(lhs, nullptr);
        lhs->AddRef();
        auto* rhs = create(ctx, 1).value();
        ASSERT_NE(rhs, nullptr);
        rhs->AddRef();
        EXPECT_NE(lhs, rhs);

        auto cmp = result.get();

        {
            auto is_eq = cmp.get_opEquals()(ctx, lhs, rhs);
            ASBIND_TEST_EXPECT_INVOKE_RESULT(is_eq);
            EXPECT_TRUE(is_eq.value());
        }

        // Self compare
        {
            auto is_eq = cmp.get_opEquals()(ctx, lhs, lhs);
            ASBIND_TEST_EXPECT_INVOKE_RESULT(is_eq);
            EXPECT_TRUE(is_eq.value());
        }

        lhs->Release();
        rhs->Release();
    }
}

TEST(Compare, ScriptClassCmp)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    auto* m = engine->GetModule(
        "foo", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "foo",
        "class foo\n"
        "{\n"
        "    int value = 0;\n"
        "    foo(int v) { this.value = v; }\n"
        "    int opCmp(const foo& rhs) const\n"
        "    { if(this.value < rhs.value) return -1;\n"
        "    return this.value > rhs.value ? 1 : 0; }\n"
        "};\n"
        "foo@ create(int v) { return foo(v); }"
    );
    ASSERT_GE(m->Build(), 0);
    auto* ti = m->GetTypeInfoByName("foo");
    ASSERT_NE(ti, nullptr);

    auto result = container::get_comparator(ti);
    {
        auto st = result.get_status();
        EXPECT_TRUE(st.good());
        EXPECT_TRUE(st);
        EXPECT_EQ(st.opCmp, AS_NAMESPACE_QUALIFIER asSUCCESS);
        EXPECT_EQ(st.opEquals, AS_NAMESPACE_QUALIFIER asNO_FUNCTION);
    }
    EXPECT_TRUE(result);

    {
        auto* expected_func = ti->GetMethodByName("opCmp");
        ASSERT_NE(expected_func, nullptr);

        auto cmp = result.get();
        EXPECT_EQ(cmp.get_opCmp(), expected_func);
        EXPECT_EQ(cmp.get_opEquals(), nullptr);
    }

    script_function<AS_NAMESPACE_QUALIFIER asIScriptObject*(int)> create(
        m->GetFunctionByDecl("foo@ create(int)")
    );
    ASSERT_TRUE(create);

    // Case 1: Less than
    {
        request_context ctx(engine);

        auto* lhs = create(ctx, -1).value();
        ASSERT_NE(lhs, nullptr);
        lhs->AddRef();
        auto* rhs = create(ctx, 1).value();
        ASSERT_NE(rhs, nullptr);
        rhs->AddRef();

        auto cmp = result.get();
        auto is_lt = cmp.get_opCmp()(ctx, lhs, rhs);

        ASBIND_TEST_EXPECT_INVOKE_RESULT(is_lt);
        EXPECT_LT(is_lt.value(), 0);
        lhs->Release();
        rhs->Release();
    }

    // Case 2: Equal
    {
        request_context ctx(engine);

        auto* lhs = create(ctx, 1).value();
        ASSERT_NE(lhs, nullptr);
        lhs->AddRef();
        auto* rhs = create(ctx, 1).value();
        ASSERT_NE(rhs, nullptr);
        rhs->AddRef();
        EXPECT_NE(lhs, rhs);

        auto cmp = result.get();

        {
            auto is_eq = cmp.get_opCmp()(ctx, lhs, rhs);
            ASBIND_TEST_EXPECT_INVOKE_RESULT(is_eq);
            EXPECT_EQ(is_eq.value(), 0);
        }

        // Self compare
        {
            auto is_eq = cmp.get_opCmp()(ctx, lhs, lhs);
            ASBIND_TEST_EXPECT_INVOKE_RESULT(is_eq);
            EXPECT_EQ(is_eq.value(), 0);
        }

        lhs->Release();
        rhs->Release();
    }
}
