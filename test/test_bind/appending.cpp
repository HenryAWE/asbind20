#include <asbind_test/framework.hpp>

namespace test_bind
{
struct val_class_for_appending
{
    int data = 0;

    int get_data() const
    {
        return data;
    }
};

enum class enum_for_appending
{
    a = 1,
    b = 2
};

struct ref_class_for_appending
{
    void addref()
    {
        ++m_counter;
    }

    void release()
    {
        ASSERT_GE(m_counter, 0);
        --m_counter;
        if(m_counter == 0)
            delete this;
    }

    int data = 0;

    int get_data() const
    {
        return data;
    }

private:
    ~ref_class_for_appending()
    {
        EXPECT_EQ(m_counter, 0);
    }

    int m_counter = 1;
};
} // namespace test_bind

TEST(Appending, ValueClass)
{
    using test_bind::val_class_for_appending;
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    value_class<val_class_for_appending, true>(engine, "val")
        .behaviours_by_traits()
        .method("int get_data() const", fp<&val_class_for_appending::get_data>);

    value_class<val_class_for_appending, true>(appending, engine, "val")
        .property("int data", &val_class_for_appending::data);

    auto* m = engine->GetModule(
        "appending", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    ASSERT_NE(m, nullptr);
    m->AddScriptSection(
        "appending",
        "int f()\n"
        "{\n"
        "    val v;\n"
        "    v.data = 1013;\n"
        "    return v.get_data();\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByDecl("int f()");
    ASSERT_NE(f, nullptr);

    request_context ctx(engine);
    auto result = script_invoke<int>(ctx, f);
    ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 1013);
}

namespace test_bind
{
static void check_enum_appending(asbind20::engine_pointer engine)
{
    auto* m = engine->GetModule(
        "appending", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    ASSERT_NE(m, nullptr);
    m->AddScriptSection(
        "appending",
        "int f()\n"
        "{\n"
        "    return int(e::b) + int(e::a);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByDecl("int f()");
    ASSERT_NE(f, nullptr);

    asbind20::request_context ctx(engine);
    auto result = script_invoke<int>(ctx, f);
    ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 1 + 2);
}
} // namespace test_bind

TEST(Appending, Enum)
{
    using test_bind::enum_for_appending;
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    enum_<enum_for_appending>(engine, "e")
        .value(enum_for_appending::a, "a");

    enum_<enum_for_appending>(appending, engine, "e")
        .value(enum_for_appending::b, "b");

    test_bind::check_enum_appending(engine);
}

TEST(TryAppending, Enum)
{
    using namespace std::literals;
    using test_bind::enum_for_appending;
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    enum_<enum_for_appending>(try_appending, engine, "e"sv)
        .value(enum_for_appending::a, "a");

    enum_<enum_for_appending>(appending, engine, "e"sv)
        .value(enum_for_appending::b, "b");

    test_bind::check_enum_appending(engine);
}

namespace test_bind
{
void check_ref_class(asbind20::engine_pointer engine)
{
    auto* m = engine->GetModule(
        "appending", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    ASSERT_NE(m, nullptr);
    m->AddScriptSection(
        "appending",
        "int f()\n"
        "{\n"
        "    rc r;\n"
        "    r.data = 1013;\n"
        "    return r.get_data();\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByDecl("int f()");
    ASSERT_NE(f, nullptr);

    asbind20::request_context ctx(engine);
    auto result = script_invoke<int>(ctx, f);
    ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 1013);
}
} // namespace test_bind

TEST(Appending, RefClass)
{
    using test_bind::ref_class_for_appending;
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    ref_class<ref_class_for_appending, true>(engine, "rc")
        .addref(fp<&ref_class_for_appending::addref>)
        .release(fp<&ref_class_for_appending::release>)
        .default_factory()
        .method("int get_data() const", fp<&ref_class_for_appending::get_data>);

    ref_class<ref_class_for_appending>(appending, engine, "rc")
        .property("int data", &ref_class_for_appending::data);

    test_bind::check_ref_class(engine);
}

TEST(TryAppending, RefClass)
{
    using test_bind::ref_class_for_appending;
    using namespace asbind20;
    using namespace std::literals;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    ref_class<ref_class_for_appending, true>(try_appending, engine, "rc"sv)
        .addref(fp<&ref_class_for_appending::addref>)
        .release(fp<&ref_class_for_appending::release>)
        .default_factory()
        .method("int get_data() const", fp<&ref_class_for_appending::get_data>);

    ref_class<ref_class_for_appending>(appending, engine, "rc"sv)
        .property("int data", &ref_class_for_appending::data);

    test_bind::check_ref_class(engine);
}

namespace test_bind
{
static void check_interface_appending(asbind20::engine_pointer engine)
{
    auto* ti = engine->GetTypeInfoByName("intf");
    ASSERT_NE(ti, nullptr);
    EXPECT_EQ(ti->GetMethodCount(), 2);

    auto* m = engine->GetModule(
        "appending", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    ASSERT_NE(m, nullptr);
    m->AddScriptSection(
        "appending",
        "class impl : intf\n"
        "{\n"
        "    int a() { return 7; }\n"
        "    int b(int x) { return x + 7; }\n"
        "}\n"
        "int f()\n"
        "{\n"
        "    intf@ h = impl();\n"
        "    return h.b(h.a());\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByDecl("int f()");
    ASSERT_NE(f, nullptr);

    asbind20::request_context ctx(engine);
    auto result = script_invoke<int>(ctx, f);
    ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 7 + 7);
}
} // namespace test_bind

TEST(Appending, Interface)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    interface i(engine, "intf");
    i.method("int a()");

    interface(appending, engine, "intf")
        .method("int b(int)");

    test_bind::check_interface_appending(engine);
}

TEST(TryAppending, Interface)
{
    using namespace asbind20;
    using namespace std::literals;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    interface i(try_appending, engine, "intf"sv);
    i.method("int a()");
    EXPECT_EQ(i.get_name(), "intf");
    EXPECT_EQ(i.get_engine(), engine);

    interface(appending, engine, "intf"sv)
        .method("int b(int)");

    test_bind::check_interface_appending(engine);
}
