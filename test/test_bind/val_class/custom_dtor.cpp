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
protected:
   static void reset_counters()
    {
        global_counter = 0;
        dtor_tester::scoped_counter = 0;
    }

    auto setup_dtor_tester()
    {
        using namespace asbind20;

        value_class<dtor_tester, UseGeneric> c(
            engine,
            "dtor_tester",
            AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS
        );
        c
            .default_constructor()
            .copy_constructor()
            .opAssign();
        return c;
    }

public:
    void SetUp() override
    {
        reset_counters();

        if constexpr(!UseGeneric)
            ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();
        engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(engine, true);
    }

    void TearDown() override
    {
        engine.reset();
    }

    asbind20::script_engine engine;

    auto compile_module() const
        -> AS_NAMESPACE_QUALIFIER asIScriptModule*
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
        if(m->Build() > 0)
        {
            ADD_FAILURE() << "Failed to build script module";
            return nullptr;
        }

        return m;
    }

    void run_dtor_test() const
    {
        auto* m = compile_module();

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
    using test_bind::external_dtor;
    setup_dtor_tester()
        .destructor_function(asbind20::fp<&external_dtor>);
    run_dtor_test();
}

TEST_F(CustomDestructorGeneric, RunDtorTest)
{
    using test_bind::external_dtor;
    setup_dtor_tester()
        .destructor_function(asbind20::fp<&external_dtor>);
    run_dtor_test();
}

namespace test_bind
{
class counter_class
{
public:
    int counter = 0;

    void dtor_as_global(dtor_tester* obj)
    {
        ++counter;
        obj->~dtor_tester();
    }
};

template <bool UseGeneric>
class custom_dtor_suite_thiscall : public custom_dtor_suite<UseGeneric>
{
    using my_base = custom_dtor_suite<UseGeneric>;

public:
    using my_base::compile_module;
    using my_base::engine;

    void run_dtor_test(counter_class& instance)
    {
        auto* m = compile_module();

        auto* f = m->GetFunctionByName("test");
        ASSERT_NE(f, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(instance.counter, 1);
        EXPECT_EQ(dtor_tester::scoped_counter, 1);
    }
};
} // namespace test_bind

using CustomDestructorThiscallNative = test_bind::custom_dtor_suite_thiscall<false>;
using CustomDestructorThiscallGeneric = test_bind::custom_dtor_suite_thiscall<true>;

TEST_F(CustomDestructorThiscallNative, RunDtorTestThiscall)
{
    using namespace asbind20;
    using test_bind::counter_class;
    counter_class instance{};
    setup_dtor_tester()
        .destructor_function(fp<&counter_class::dtor_as_global>, auxiliary(instance));
    run_dtor_test(instance);
}

TEST_F(CustomDestructorThiscallGeneric, RunDtorTestThiscall)
{
    using namespace asbind20;
    using test_bind::counter_class;
    counter_class instance{};
    setup_dtor_tester()
        .destructor_function(fp<&counter_class::dtor_as_global>, auxiliary(instance));
    run_dtor_test(instance);
}
