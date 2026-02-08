#include <asbind_test/framework.hpp>

namespace test_bind
{
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
        ++dtor_counter;
    }

    static inline int dtor_counter = 0;
    static inline int mfn_counter = 0;

    // Member function for self-deletion
    void self_delete()
    {
        ++mfn_counter;
        std::destroy_at(this);
    }
};

template <bool UseGeneric>
class custom_dtor_suite_base : public ::testing::Test
{
protected:
    virtual void reset_counters() const
    {
        dtor_tester::dtor_counter = 0;
        dtor_tester::mfn_counter = 0;
    }

public:
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
};

template <bool UseGeneric>
class custom_dtor_mfn_suite : public custom_dtor_suite_base<UseGeneric>
{
    using my_base = custom_dtor_suite_base<UseGeneric>;

public:
    using my_base::compile_module;
    using my_base::engine;

    void run_dtor_test() const
    {
        auto* m = compile_module();

        auto* f = m->GetFunctionByName("test");
        ASSERT_NE(f, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(dtor_tester::dtor_counter, 1);
        EXPECT_EQ(dtor_tester::mfn_counter, 1);
    }
};
} // namespace test_bind

using CustomDestructorMFNNative = test_bind::custom_dtor_mfn_suite<false>;
using CustomDestructorMFNGeneric = test_bind::custom_dtor_mfn_suite<true>;

TEST_F(CustomDestructorMFNNative, RunDtorTest)
{
    using test_bind::dtor_tester;
    setup_dtor_tester()
        .destructor_function(asbind20::fp<&dtor_tester::self_delete>);
    run_dtor_test();
}

TEST_F(CustomDestructorMFNGeneric, RunDtorTest)
{
    using test_bind::dtor_tester;
    setup_dtor_tester()
        .destructor_function(asbind20::fp<&dtor_tester::self_delete>);
    run_dtor_test();
}

namespace test_bind
{
static int global_counter = 0;

static void external_dtor(dtor_tester* this_)
{
    ++global_counter;
    this_->~dtor_tester();
}

template <bool UseGeneric>
class custom_dtor_external_suite : public custom_dtor_suite_base<UseGeneric>
{
    using my_base = custom_dtor_suite_base<UseGeneric>;

protected:
    void reset_counters() const override
    {
        my_base::reset_counters();
        global_counter = 0;
    }

public:
    using my_base::compile_module;
    using my_base::engine;

    void run_dtor_test() const
    {
        auto* m = compile_module();

        auto* f = m->GetFunctionByName("test");
        ASSERT_NE(f, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(global_counter, 1);
        EXPECT_EQ(dtor_tester::dtor_counter, 1);
        EXPECT_EQ(dtor_tester::mfn_counter, 0)
            << "This should be untouched";
    }
};
} // namespace test_bind

using CustomDestructorExternalNative = test_bind::custom_dtor_external_suite<false>;
using CustomDestructorExternalGeneric = test_bind::custom_dtor_external_suite<true>;

TEST_F(CustomDestructorExternalNative, RunDtorTest)
{
    using test_bind::external_dtor;
    setup_dtor_tester()
        .destructor_function(asbind20::fp<&external_dtor>);
    run_dtor_test();
}

TEST_F(CustomDestructorExternalGeneric, RunDtorTest)
{
    using test_bind::external_dtor;
    setup_dtor_tester()
        .destructor_function(asbind20::fp<&external_dtor>);
    run_dtor_test();
}

namespace test_bind
{
class aux_counter
{
public:
    int counter = 0;

    void aux_dtor(dtor_tester* obj)
    {
        ++counter;
        obj->~dtor_tester();
    }
};

template <bool UseGeneric>
class custom_dtor_suite_aux : public custom_dtor_suite_base<UseGeneric>
{
    using my_base = custom_dtor_suite_base<UseGeneric>;

    // Magic number to ensure the value is untouched
    static constexpr int magic_num = 42;

    void reset_counters() const override
    {
        my_base::reset_counters();
        // This value should be untouched
        global_counter = magic_num;
    }

public:
    using my_base::compile_module;
    using my_base::engine;

    void run_dtor_test(const aux_counter& instance)
    {
        auto* m = compile_module();

        auto* f = m->GetFunctionByName("test");
        ASSERT_NE(f, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(instance.counter, 1);
        EXPECT_EQ(dtor_tester::dtor_counter, 1);
        EXPECT_EQ(global_counter, magic_num)
            << "This should be untouched";
    }
};
} // namespace test_bind

using CustomDestructorAuxNative = test_bind::custom_dtor_suite_aux<false>;
using CustomDestructorAuxGeneric = test_bind::custom_dtor_suite_aux<true>;

TEST_F(CustomDestructorAuxNative, RunDtorTest)
{
    using namespace asbind20;
    using test_bind::aux_counter;
    aux_counter instance{};
    setup_dtor_tester()
        .destructor_function(fp<&aux_counter::aux_dtor>, auxiliary(instance));
    run_dtor_test(instance);
}

TEST_F(CustomDestructorAuxGeneric, RunDtorTest)
{
    using namespace asbind20;
    using test_bind::aux_counter;
    aux_counter instance{};
    setup_dtor_tester()
        .destructor_function(fp<&aux_counter::aux_dtor>, auxiliary(instance));
    run_dtor_test(instance);
}

namespace test_bind
{
static int counter_lambda = 0;

template <bool UseGeneric>
class custom_dtor_suite_lambda : public custom_dtor_suite_base<UseGeneric>
{
    using my_base = custom_dtor_suite_base<UseGeneric>;

    void reset_counters() const override
    {
        my_base::reset_counters();
        counter_lambda = 0;
    }

public:
    using my_base::compile_module;
    using my_base::engine;

    void run_dtor_test()
    {
        auto* m = compile_module();

        auto* f = m->GetFunctionByName("test");
        ASSERT_NE(f, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(counter_lambda, 1);
        EXPECT_EQ(dtor_tester::dtor_counter, 1);
        EXPECT_EQ(dtor_tester::mfn_counter, 0)
            << "This should be untouched";
    }
};
} // namespace test_bind

using CustomDestructorLambdaNative = test_bind::custom_dtor_suite_lambda<false>;
using CustomDestructorLambdaGeneric = test_bind::custom_dtor_suite_lambda<true>;

TEST_F(CustomDestructorLambdaNative, RunDtorTest)
{
    using namespace asbind20;
    setup_dtor_tester()
        .destructor_function(
            [](test_bind::dtor_tester* this_)
            {
                ++test_bind::counter_lambda;
                std::destroy_at(this_);
            }
        );
    run_dtor_test();
}

TEST_F(CustomDestructorLambdaGeneric, RunDtorTest)
{
    using namespace asbind20;
    setup_dtor_tester()
        .destructor_function(
            [](test_bind::dtor_tester* this_)
            {
                ++test_bind::counter_lambda;
                std::destroy_at(this_);
            }
        );
    run_dtor_test();
}
