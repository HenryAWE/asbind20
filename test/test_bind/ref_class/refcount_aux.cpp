#include <asbind_test/framework.hpp>
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

class refcount_aux_helper
{
public:
    refcount_aux_helper() = default;
    refcount_aux_helper(const refcount_aux_helper&) = default;

    refcount_aux_helper& operator=(const refcount_aux_helper&) = default;

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
        m_counts[static_cast<void*>(this_)] += 1;
    }

    void release(refcount_aux* this_)
    {
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
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx = nullptr;
    refcount_aux_helper helper;

    void SetUp() override
    {
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
        helper.check_and_clear();
        engine.reset();
    }

    auto compile_module() const
        -> AS_NAMESPACE_QUALIFIER asIScriptModule*
    {
        auto* m = engine->GetModule(
            "refcount_aux", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
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
    auto result = run_test<refcount_aux*>(0);

    EXPECT_EQ(result.value(), nullptr);
}

TEST_F(RefcountAuxGeneric, RunTest0)
{
    using test_bind::refcount_aux;
    auto result = run_test<refcount_aux*>(0);

    EXPECT_EQ(result.value(), nullptr);
}

#ifdef ASBIND20_HAS_THISCALL_OBJ_FOR_REF_BEHAVIOURS

TEST_F(RefcountAuxNative, RunTest1)
{
    using test_bind::refcount_aux;
    auto result = run_test<int>(1);

    EXPECT_EQ(result.value(), 1);
}

TEST_F(RefcountAuxGeneric, RunTest1)
{
    using test_bind::refcount_aux;
    auto result = run_test<int>(1);

    EXPECT_EQ(result.value(), 1);
}

TEST_F(RefcountAuxNative, RunTest2)
{
    using test_bind::refcount_aux;
    auto result = run_test<int>(2);

    EXPECT_EQ(result.value(), 2);
}

#endif

TEST_F(RefcountAuxGeneric, RunTest2)
{
    using test_bind::refcount_aux;
    auto result = run_test<int>(2);

    EXPECT_EQ(result.value(), 2);
}
