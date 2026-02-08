#include <asbind_test/framework.hpp>

namespace test_bind
{
class templ_cb_tester
{
public:
    explicit templ_cb_tester(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        : m_ti(ti) {}

    explicit templ_cb_tester(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, int val)
        : value(val), m_ti(ti) {}

    ~templ_cb_tester() = default;

    void addref()
    {
        ++m_refcount;
    }

    void release()
    {
        EXPECT_GE(m_refcount, 0);
        --m_refcount;
        if(m_refcount <= 0)
            delete this;
    }

    int value = 0;

private:
    asbind20::script_typeinfo m_ti;
    int m_refcount = 1;
};

class templ_cb_helper
{
public:
    bool validate(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, bool&) const
    {
        if(ti->GetSubTypeId() != expected_tid)
        {
            std::cerr << "ti->GetSubTypeId(): " << ti->GetSubTypeId() << std::endl;
            std::cerr << "expected_tid: " << expected_tid << std::endl;
            return false;
        }
        return true;
    }

    int expected_tid = AS_NAMESPACE_QUALIFIER asTYPEID_INT32;
};

template <bool UseGeneric>
class templ_cb_as_global_suite : public ::testing::Test
{
public:
    asbind20::script_engine engine;
    templ_cb_helper helper;

    void SetUp() override
    {
        using namespace asbind20;
        engine = make_script_engine();
        // We won't propagate error to GTest in the message callback for this suite,
        // because some tests will trigger error intentionally.
        asbind_test::setup_message_callback(engine, false);

        if constexpr(!UseGeneric)
            ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

        template_ref_class<templ_cb_tester, UseGeneric>(engine, "templ_cb_tester<T>")
            .template_callback(fp<&templ_cb_helper::validate>, auxiliary(helper))
            .addref(fp<&templ_cb_tester::addref>)
            .release(fp<&templ_cb_tester::release>)
            .default_factory()
            .template factory<int>("int")
            .property("int value", &templ_cb_tester::value);
    }

    void TearDown() override
    {
        engine.reset();
    }

    void run_test()
    {
        auto* m = engine->GetModule(
            "templ_cb_as_global", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        m->AddScriptSection(
            "templ_cb_as_global: run_test",
            "int run()\n"
            "{\n"
            "    templ_cb_tester<int> tester(42);\n"
            "    return tester.value;\n"
            "}"
        );
        ASSERT_GE(m->Build(), 0);

        auto* f = m->GetFunctionByName("run");
        ASSERT_NE(f, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 42);
    }

    void expect_to_fail()
    {
        auto* m = engine->GetModule(
            "templ_cb_as_global", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        m->AddScriptSection(
            "templ_cb_as_global: expect_to_fail",
            "void f()\n"
            "{\n"
            "    templ_cb_tester<float> tester(42);\n"
            "}"
        );
        EXPECT_LT(m->Build(), 0);
    }
};
} // namespace test_bind

using TemplCBAsGlobalGeneric = test_bind::templ_cb_as_global_suite<true>;
using TemplCBAsGlobalNative = test_bind::templ_cb_as_global_suite<false>;

TEST_F(TemplCBAsGlobalGeneric, RunTest)
{
    run_test();
}

// TODO: It seems that template callback with auxiliary pointer has problem in native mode.
// Enable this test after confirming the cause.
// TEST_F(TemplCBAsGlobalNative, RunTest)
// {
//     run_test();
// }

TEST_F(TemplCBAsGlobalGeneric, ExpectToFail)
{
    expect_to_fail();
}

TEST_F(TemplCBAsGlobalNative, ExpectToFail)
{
    expect_to_fail();
}
