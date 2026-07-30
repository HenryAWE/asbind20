#include <asbind_test/framework.hpp>
#include <gmock/gmock.h>

namespace test_bind
{
// Injectable spies for external and lambda destructor tests.
// Declared here so the base class TearDown can clean them up.
static std::shared_ptr<::testing::MockFunction<void()>> mfn_spy;
static std::shared_ptr<::testing::MockFunction<void()>> ext_spy;
static std::shared_ptr<::testing::MockFunction<void()>> lambda_spy;

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

    void self_delete()
    {
        if(mfn_spy)
            mfn_spy->Call();
        std::destroy_at(this);
    }
};

template <bool UseGeneric>
class custom_dtor_suite_base : public ::testing::Test
{
protected:
    virtual void reset_counters_and_spies() const
    {
        dtor_tester::dtor_counter = 0;
        mfn_spy.reset();
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
        reset_counters_and_spies();

        if constexpr(!UseGeneric)
            ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();
        engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(engine);
    }

    void TearDown() override
    {
        using ::testing::Mock;
        // Verify and clean up all mock spies before engine teardown,
        // so mock verification failures are reported.
        if(mfn_spy)
        {
            Mock::VerifyAndClearExpectations(mfn_spy.get());
            mfn_spy.reset();
        }
        if(ext_spy)
        {
            Mock::VerifyAndClearExpectations(ext_spy.get());
            ext_spy.reset();
        }
        if(lambda_spy)
        {
            Mock::VerifyAndClearExpectations(lambda_spy.get());
            lambda_spy.reset();
        }
        engine.reset();
    }

    asbind20::script_engine engine;

    asbind20::module_pointer compile_module() const
    {
        auto* m = asbind20::create_module(engine, "dtor_test");
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
        ASSERT_THAT(f, ::testing::NotNull());

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(dtor_tester::dtor_counter, 1);
    }
};
} // namespace test_bind

using CustomDestructorMFNNative = test_bind::custom_dtor_mfn_suite<false>;
using CustomDestructorMFNGeneric = test_bind::custom_dtor_mfn_suite<true>;

TEST_F(CustomDestructorMFNNative, RunDtorTest)
{
    using test_bind::dtor_tester;
    using test_bind::mfn_spy;

    mfn_spy = std::make_shared<::testing::MockFunction<void()>>();
    EXPECT_CALL(*mfn_spy, Call()).Times(1);

    setup_dtor_tester()
        .destructor_function(asbind20::fp<&dtor_tester::self_delete>);
    run_dtor_test();
}

TEST_F(CustomDestructorMFNGeneric, RunDtorTest)
{
    using test_bind::dtor_tester;
    using test_bind::mfn_spy;

    // Set up spy: expect self_delete() to be called exactly once
    mfn_spy = std::make_shared<::testing::MockFunction<void()>>();
    EXPECT_CALL(*mfn_spy, Call()).Times(1);

    setup_dtor_tester()
        .destructor_function(asbind20::fp<&dtor_tester::self_delete>);
    run_dtor_test();
}

namespace test_bind
{
static void external_dtor(dtor_tester* this_)
{
    if(ext_spy)
        ext_spy->Call();
    this_->~dtor_tester();
}

template <bool UseGeneric>
class custom_dtor_external_suite : public custom_dtor_suite_base<UseGeneric>
{
    using my_base = custom_dtor_suite_base<UseGeneric>;

protected:
    void reset_counters_and_spies() const override
    {
        my_base::reset_counters_and_spies();
        ext_spy.reset();
    }

public:
    using my_base::compile_module;
    using my_base::engine;

    void run_dtor_test() const
    {
        auto* m = compile_module();

        auto* f = m->GetFunctionByName("test");
        ASSERT_THAT(f, ::testing::NotNull());

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(dtor_tester::dtor_counter, 1);
    }
};
} // namespace test_bind

using CustomDestructorExternalNative = test_bind::custom_dtor_external_suite<false>;
using CustomDestructorExternalGeneric = test_bind::custom_dtor_external_suite<true>;

TEST_F(CustomDestructorExternalNative, RunDtorTest)
{
    using test_bind::ext_spy;
    using test_bind::external_dtor;

    ext_spy = std::make_shared<::testing::MockFunction<void()>>();
    EXPECT_CALL(*ext_spy, Call()).Times(1);

    setup_dtor_tester()
        .destructor_function(asbind20::fp<&external_dtor>);
    run_dtor_test();
}

TEST_F(CustomDestructorExternalGeneric, RunDtorTest)
{
    using test_bind::ext_spy;
    using test_bind::external_dtor;

    ext_spy = std::make_shared<::testing::MockFunction<void()>>();
    EXPECT_CALL(*ext_spy, Call()).Times(1);

    setup_dtor_tester()
        .destructor_function(asbind20::fp<&external_dtor>);
    run_dtor_test();
}

namespace test_bind
{
class aux_counter
{
public:
    struct mock_dtor_call
    {
        MOCK_METHOD(void, on_custom_dtor, (), ());
    };

    using mock_type = ::testing::StrictMock<mock_dtor_call>;
    std::shared_ptr<mock_type> mock = std::make_shared<mock_type>();

    void aux_dtor(dtor_tester* obj)
    {
        mock->on_custom_dtor();
        obj->~dtor_tester();
    }
};

template <bool UseGeneric>
class custom_dtor_suite_aux : public custom_dtor_suite_base<UseGeneric>
{
    using my_base = custom_dtor_suite_base<UseGeneric>;

public:
    using my_base::compile_module;
    using my_base::engine;

    void run_dtor_test()
    {
        auto* m = compile_module();

        auto* f = m->GetFunctionByName("test");
        ASSERT_THAT(f, ::testing::NotNull());

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(dtor_tester::dtor_counter, 1);
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
    EXPECT_CALL(*instance.mock, on_custom_dtor()).Times(1);

    setup_dtor_tester()
        .destructor_function(fp<&aux_counter::aux_dtor>, auxiliary(instance));
    run_dtor_test();
}

TEST_F(CustomDestructorAuxGeneric, RunDtorTest)
{
    using namespace asbind20;
    using test_bind::aux_counter;

    aux_counter instance{};
    EXPECT_CALL(*instance.mock, on_custom_dtor()).Times(1);

    setup_dtor_tester()
        .destructor_function(fp<&aux_counter::aux_dtor>, auxiliary(instance));
    run_dtor_test();
}

namespace test_bind
{
template <bool UseGeneric>
class custom_dtor_suite_lambda : public custom_dtor_suite_base<UseGeneric>
{
    using my_base = custom_dtor_suite_base<UseGeneric>;

    void reset_counters_and_spies() const override
    {
        my_base::reset_counters_and_spies();
        lambda_spy.reset();
    }

public:
    using my_base::compile_module;
    using my_base::engine;

    void run_dtor_test()
    {
        auto* m = compile_module();

        auto* f = m->GetFunctionByName("test");
        ASSERT_THAT(f, ::testing::NotNull());

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(dtor_tester::dtor_counter, 1);
    }
};
} // namespace test_bind

using CustomDestructorLambdaNative = test_bind::custom_dtor_suite_lambda<false>;
using CustomDestructorLambdaGeneric = test_bind::custom_dtor_suite_lambda<true>;

TEST_F(CustomDestructorLambdaNative, RunDtorTest)
{
    using namespace asbind20;
    using test_bind::lambda_spy;

    // Set up spy: expect lambda destructor to be called exactly once
    lambda_spy = std::make_shared<::testing::MockFunction<void()>>();
    EXPECT_CALL(*lambda_spy, Call()).Times(1);

    setup_dtor_tester()
        .destructor_function(
            [](test_bind::dtor_tester* this_)
            {
                lambda_spy->Call();
                std::destroy_at(this_);
            }
        );
    run_dtor_test();
}

TEST_F(CustomDestructorLambdaGeneric, RunDtorTest)
{
    using namespace asbind20;
    using test_bind::lambda_spy;

    lambda_spy = std::make_shared<::testing::MockFunction<void()>>();
    EXPECT_CALL(*lambda_spy, Call()).Times(1);

    setup_dtor_tester()
        .destructor_function(
            [](test_bind::dtor_tester* this_)
            {
                lambda_spy->Call();
                std::destroy_at(this_);
            }
        );
    run_dtor_test();
}
