#include <asbind20/ext/assert.hpp>
#include <shared_test_lib.hpp>

namespace test_bind
{
// Multipurpose
struct test_aux_factory
{
public:
    test_aux_factory() = default;

    test_aux_factory(int val)
        : value(val) {}

    void addref()
    {
        ++m_counter;
    }

    void release()
    {
        assert(m_counter > 0);
        if(--m_counter == 0)
            delete this;
    }

    int value = 0;

private:
    ~test_aux_factory() = default;

    int m_counter = 1;
};

struct aux_factory_helper
{
    int predefined_value = 0;
    std::size_t created = 0;

    test_aux_factory* create_aux_as_global(int additional)
    {
        ++created;
        return new test_aux_factory(predefined_value + additional);
    }
};

test_aux_factory* create_aux_auxfirst(aux_factory_helper& helper, int additional)
{
    ++helper.created;
    return new test_aux_factory(helper.predefined_value + additional);
}

static void setup_env(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    asbind20::global(engine)
        .message_callback(
            +[](const asSMessageInfo* msg, void*)
            {
                std::cerr << msg->message << std::endl;
            }
        );

    asbind20::ext::register_script_assert(
        engine,
        [](std::string_view msg)
        {
            GTEST_FAIL() << "assertion failure: " << msg;
        }
    );
}

template <bool UseGeneric>
auto register_test_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;
    return asbind20::ref_class<test_aux_factory, UseGeneric>(engine, "test_aux_factory")
        .addref(fp<&test_aux_factory::addref>)
        .release(fp<&test_aux_factory::release>)
        .property("int val", &test_aux_factory::value);
}

void check_aux_factory(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int expected_val, int arg)
{
    auto* m = engine->GetModule("test_aux_factory", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);
    m->AddScriptSection(
        "test_aux_factory",
        "int get(int arg) { test_aux_factory f(arg); return f.val; }"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("get");
    ASSERT_NE(f, nullptr);

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<int>(ctx, f, arg);
    ASSERT_TRUE(asbind_test::result_has_value(result));

    EXPECT_EQ(result.value(), expected_val);
}

void print_factories(std::ostream& os, asITypeInfo* ti)
{
    os << "Factories of " << ti->GetName() << ":\n";
    auto count = ti->GetBehaviourCount();
    for(asUINT i = 0; i < count; ++i)
    {
        asEBehaviours beh;
        auto* factory = ti->GetBehaviourByIndex(i, &beh);
        if(beh != asBEHAVE_FACTORY)
            continue;

        os << "Factory: " << factory->GetDeclaration() << std::endl;
    }
}
} // namespace test_bind

TEST(aux_factory_native, as_global)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::aux_factory_helper helper{.predefined_value = 0};

    test_bind::register_test_class<false>(engine)
        .factory_function("int", use_explicit, &test_bind::aux_factory_helper::create_aux_as_global, auxiliary(helper));

    EXPECT_EQ(helper.created, 0);

    test_bind::check_aux_factory(engine, 0, 0);
    EXPECT_EQ(helper.created, 1);

    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    EXPECT_EQ(helper.created, 2);
}

TEST(aux_factory_generic, as_global)
{
    using namespace asbind20;

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::aux_factory_helper helper{.predefined_value = 0};

    test_bind::register_test_class<true>(engine)
        .factory_function("int", use_explicit, fp<&test_bind::aux_factory_helper::create_aux_as_global>, auxiliary(helper));

    EXPECT_EQ(helper.created, 0);

    test_bind::check_aux_factory(engine, 0, 0);
    EXPECT_EQ(helper.created, 1);

    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    EXPECT_EQ(helper.created, 2);
}

TEST(aux_factory_native, auxfirst)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::aux_factory_helper helper{.predefined_value = 0};

    test_bind::register_test_class<false>(engine)
        .factory_function("int", use_explicit, &test_bind::create_aux_auxfirst, auxiliary(helper));

    EXPECT_EQ(helper.created, 0);

    test_bind::check_aux_factory(engine, 0, 0);
    EXPECT_EQ(helper.created, 1);

    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    EXPECT_EQ(helper.created, 2);
}

TEST(aux_factory_generic, auxfirst)
{
    using namespace asbind20;

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::aux_factory_helper helper{.predefined_value = 0};

    test_bind::register_test_class<true>(engine)
        .factory_function("int", use_explicit, fp<&test_bind::create_aux_auxfirst>, auxiliary(helper));

    EXPECT_EQ(helper.created, 0);

    test_bind::check_aux_factory(engine, 0, 0);
    EXPECT_EQ(helper.created, 1);

    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    EXPECT_EQ(helper.created, 2);
}
