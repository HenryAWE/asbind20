#include <asbind_test/framework.hpp>

namespace test_bind
{
static int global_counter = 0;

class dtor_tester
{
    [[maybe_unused]]
    int dummy[4];

public:
    dtor_tester()
        : dummy{}
    {}

    dtor_tester(const dtor_tester&) = default;

    dtor_tester& operator=(const dtor_tester&) = default;

    ~dtor_tester()
    {
        ++scoped_counter;
    }

    static inline int scoped_counter = 0;
};

static void external_dtor(dtor_tester* this_)
{
    ++global_counter;
    this_->~dtor_tester();
}

template <bool UseGeneric>
class custom_dtor_suite : public ::testing::Test
{
    static void reset_counters()
    {
        global_counter = 0;
        dtor_tester::scoped_counter = 0;
    }

public:
    void SetUp() override
    {
        reset_counters();

        if constexpr(!UseGeneric)
            ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();
        engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(engine, true);

        asbind20::value_class<dtor_tester, UseGeneric>(
            engine, "dtor_tester", AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS
        )
            .default_constructor()
            .copy_constructor()
            .opAssign()
            .destructor_function(asbind20::fp<&external_dtor>);
    }

    void TearDown() override
    {
        engine.reset();
    }

    asbind20::script_engine engine;

    void run_dtor_test() const
    {
        auto* m = engine->GetModule(
            "dtor_test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        m->AddScriptSection(
            "dtor_test",
            "void test()\n"
            "{\n"
            "    dtor_tester instance;"
            "}"
        );
        ASSERT_GE(m->Build(), 0);

        auto* f = m->GetFunctionByName("test");
        ASSERT_NE(f, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(global_counter, 1);
        EXPECT_EQ(dtor_tester::scoped_counter, 1);
    }
};
} // namespace test_bind

using CustomDestructorNative = test_bind::custom_dtor_suite<false>;
using CustomDestructorGeneric = test_bind::custom_dtor_suite<true>;

TEST_F(CustomDestructorNative, RunDtorTest)
{
    run_dtor_test();
}

TEST_F(CustomDestructorGeneric, RunDtorTest)
{
    run_dtor_test();
}
