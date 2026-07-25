#include <asbind_test/framework.hpp>
#include <gmock/gmock.h>
#include <map>
#include <asbind20/io/to_string.hpp>

namespace test_bind
{
class refcount_aux
{
public:
    int value = 0;

    void update()
    {
        ++value;
    }
};

struct mock_refcount
{
    MOCK_METHOD(void, on_addref, (refcount_aux*), ());
    MOCK_METHOD(void, on_release, (refcount_aux*), ());
};

class refcount_aux_helper
{
public:
    using mock_type = ::testing::StrictMock<mock_refcount>;
    std::shared_ptr<mock_type> mock = std::make_shared<mock_type>();

    refcount_aux_helper() = default;

    refcount_aux* create()
    {
        auto* ptr = new refcount_aux{};
        addref(ptr);
        return ptr;
    }

    refcount_aux* create_by_int(int val)
    {
        auto* ptr = create();
        ptr->value = val;
        return ptr;
    }

    void addref(refcount_aux* this_)
    {
        mock->on_addref(this_);
        m_counts[static_cast<void*>(this_)] += 1;
    }

    void release(refcount_aux* this_)
    {
        mock->on_release(this_);

        auto it = m_counts.find(static_cast<void*>(this_));
        if(it == m_counts.end())
            return;
        ASSERT_GE(it->second, 1);
        it->second -= 1;

        delete this_;
    }

    using data_type = std::map<
        void*,
        AS_NAMESPACE_QUALIFIER asUINT>;

    const data_type& get_counts() const noexcept
    {
        return m_counts;
    }

    void check_and_clear()
    {
        for(auto [addr, c] : m_counts)
        {
            EXPECT_EQ(c, 0)
                << "unmatched addref/release, address: " << addr;
        }
        m_counts.clear();
    }

private:
    data_type m_counts;
};

template <bool UseGeneric>
class refcount_aux_suite : public ::testing::Test
{
public:
    asbind20::script_engine engine;
    asbind20::context_pointer ctx = nullptr;
    refcount_aux_helper helper;

    void SetUp() override
    {
        // Reset helper with a fresh mock for test isolation
        helper = refcount_aux_helper{};

        using namespace asbind20;
        engine = make_script_engine();
        asbind_test::setup_message_callback(engine, true);

        if constexpr(!UseGeneric)
            ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

        ref_class<refcount_aux, UseGeneric>(engine, "refcount_aux")
            .factory_function("", fp<&refcount_aux_helper::create>, auxiliary(helper))
            .factory_function("int val", use_explicit, fp<&refcount_aux_helper::create_by_int>, auxiliary(helper))
            .addref(fp<&refcount_aux_helper::addref>, auxiliary(helper))
            .release(fp<&refcount_aux_helper::release>, auxiliary(helper))
            .method("void update()", fp<&refcount_aux::update>)
            .property("int value", &refcount_aux::value);

        ctx = engine->RequestContext();
    }

    void TearDown() override
    {
        if(ctx)
        {
            engine->ReturnContext(ctx);
            ctx = nullptr;
        }
        // GMock: verify call count expectations
        ::testing::Mock::VerifyAndClearExpectations(helper.mock.get());
        // Manual counting: verify balanced addref/release
        helper.check_and_clear();
        engine.reset();
    }

    asbind20::module_pointer compile_module() const
    {
        auto* m = asbind20::create_module(engine, "refcount_aux");
        if(!m)
        {
            ADD_FAILURE() << "Failed to create module";
            return nullptr;
        }
        m->AddScriptSection(
            "refcount_aux",
            "refcount_aux@ test0()\n"
            "{\n"
            "    refcount_aux@ r = null;\n"
            "    return r;\n"
            "}\n"
            "int test1()\n"
            "{\n"
            "    refcount_aux c;\n"
            "    c.update();\n"
            "    return c.value;\n"
            "}\n"
            "int test2()\n"
            "{\n"
            "    refcount_aux c(2);\n"
            "    return c.value;\n"
            "}"
        );
        if(m->Build() < 0)
        {
            ADD_FAILURE() << "Failed to create script module";
            return nullptr;
        }

        return m;
    }

    template <typename Return>
    auto run_test(int idx)
    {
        std::string decl = "test" + std::to_string(idx);
        SCOPED_TRACE("test #" + std::to_string(idx));

        auto* m = compile_module();
        if(!m) // Test failure should have already been set compile_module()
            std::abort();
        auto* f = m->GetFunctionByName(decl.c_str());
        if(!f)
        {
            ADD_FAILURE() << "Function not found";
            std::abort();
        }

        auto result = asbind20::script_invoke<Return>(ctx, f);
        if(!result.has_value())
        {
            using asbind20::to_string;
            ADD_FAILURE()
                << "Bad result: " << to_string(result.error());
            std::abort();
        }
        return result;
    }
};
} // namespace test_bind

using RefcountAuxNative = test_bind::refcount_aux_suite<false>;
using RefcountAuxGeneric = test_bind::refcount_aux_suite<true>;

TEST_F(RefcountAuxNative, RunTest0)
{
    using test_bind::refcount_aux;
    using ::testing::_;

    // Returning a null handle should not trigger any addref/release
    EXPECT_CALL(*helper.mock, on_addref(_)).Times(0);
    EXPECT_CALL(*helper.mock, on_release(_)).Times(0);

    auto result = run_test<refcount_aux*>(0);
    EXPECT_EQ(result.value(), nullptr);
}

TEST_F(RefcountAuxGeneric, RunTest0)
{
    using test_bind::refcount_aux;
    using ::testing::_;

    // Returning a null handle should not trigger any addref/release
    EXPECT_CALL(*helper.mock, on_addref(_)).Times(0);
    EXPECT_CALL(*helper.mock, on_release(_)).Times(0);

    auto result = run_test<refcount_aux*>(0);
    EXPECT_EQ(result.value(), nullptr);
}

#ifdef ASBIND20_HAS_THISCALL_OBJ_FOR_REF_BEHAVIOURS

TEST_F(RefcountAuxNative, RunTest1)
{
    using test_bind::refcount_aux;
    using ::testing::_;

    // Creating and using a refcount_aux object triggers one addref/release cycle
    EXPECT_CALL(*helper.mock, on_addref(_)).Times(1);
    EXPECT_CALL(*helper.mock, on_release(_)).Times(1);

    auto result = run_test<int>(1);
    EXPECT_EQ(result.value(), 1);
}

TEST_F(RefcountAuxGeneric, RunTest1)
{
    using test_bind::refcount_aux;
    using ::testing::_;

    // Creating and using a refcount_aux object triggers one addref/release cycle
    EXPECT_CALL(*helper.mock, on_addref(_)).Times(1);
    EXPECT_CALL(*helper.mock, on_release(_)).Times(1);

    auto result = run_test<int>(1);
    EXPECT_EQ(result.value(), 1);
}

TEST_F(RefcountAuxNative, RunTest2)
{
    using test_bind::refcount_aux;
    using ::testing::_;

    // Creating a refcount_aux with initial value triggers one addref/release cycle
    EXPECT_CALL(*helper.mock, on_addref(_)).Times(1);
    EXPECT_CALL(*helper.mock, on_release(_)).Times(1);

    auto result = run_test<int>(2);
    EXPECT_EQ(result.value(), 2);
}

#endif

TEST_F(RefcountAuxGeneric, RunTest2)
{
    using test_bind::refcount_aux;
    using ::testing::_;

    // Creating a refcount_aux with initial value triggers one addref/release cycle
    EXPECT_CALL(*helper.mock, on_addref(_)).Times(1);
    EXPECT_CALL(*helper.mock, on_release(_)).Times(1);

    auto result = run_test<int>(2);
    EXPECT_EQ(result.value(), 2);
}
