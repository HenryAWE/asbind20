#include <asbind_test/framework.hpp>
#include <gmock/gmock.h>

namespace test_bind
{
// Multipurpose
class test_aux_factory
{
public:
    test_aux_factory() = default;

    test_aux_factory(int val)
        : value(val) {}

    test_aux_factory(int initial_val, asbind20::script_init_list_repeat list)
        : value(initial_val)
    {
        int* start = (int*)list.data();
        int* sentinel = start + list.size();

        for(int* p = start; p != sentinel; ++p)
            value += *p;
    }

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

struct mock_create
{
    MOCK_METHOD(void, on_create, (), ());
    MOCK_METHOD(void, on_list_create, (), ());
};

struct aux_factory_helper
{
    using mock_type = ::testing::StrictMock<mock_create>;
    std::shared_ptr<mock_type> mock = std::make_shared<mock_type>();
    int predefined_value = 0;

    aux_factory_helper() = default;
    explicit aux_factory_helper(int predefined_val)
        : predefined_value(predefined_val) {}

    aux_factory_helper(const aux_factory_helper&) = delete;
    aux_factory_helper(aux_factory_helper&&) = delete;

    test_aux_factory* create_aux_as_global(int additional)
    {
        mock->on_create();
        return new test_aux_factory(predefined_value + additional);
    }

    test_aux_factory* create_aux_as_global_list(void* list_buf)
    {
        mock->on_list_create();
        return new test_aux_factory(
            predefined_value, asbind20::script_init_list_repeat(list_buf)
        );
    };
};

static test_aux_factory* create_aux_auxfirst(aux_factory_helper& helper, int additional)
{
    helper.mock->on_create();
    return new test_aux_factory(helper.predefined_value + additional);
}

static test_aux_factory* create_aux_auxfirst_list(aux_factory_helper& helper, void* list_buf)
{
    helper.mock->on_list_create();
    return new test_aux_factory(
        helper.predefined_value, asbind20::script_init_list_repeat(list_buf)
    );
}

static test_aux_factory* create_aux_auxlast(int additional, aux_factory_helper& helper)
{
    helper.mock->on_create();
    return new test_aux_factory(helper.predefined_value + additional);
}

static test_aux_factory* create_aux_auxlast_list(void* list_buf, aux_factory_helper& helper)
{
    helper.mock->on_list_create();
    return new test_aux_factory(
        helper.predefined_value, asbind20::script_init_list_repeat(list_buf)
    );
}

static void setup_env(asbind20::engine_pointer engine)
{
    asbind_test::setup_message_callback(engine);
    asbind_test::setup_script_assertion(engine);
}

struct register_refcount_helper
{
    template <typename AutoRegister>
    void operator()(AutoRegister& ar) const
    {
        using namespace asbind20;
        using class_type = typename AutoRegister::class_type;
        ar
            .addref(fp<&class_type::addref>)
            .release(fp<&class_type::release>);
    }
};

template <bool UseGeneric>
static auto register_test_class(asbind20::engine_pointer engine)
{
    using namespace asbind20;
    return asbind20::ref_class<test_aux_factory, UseGeneric>(engine, "test_aux_factory")
        .use(register_refcount_helper{})
        .property("int val", &test_aux_factory::value);
}

static void check_aux_factory(asbind20::engine_pointer engine, int expected_val, int arg)
{
    auto* m = asbind20::create_module(engine, "test_aux_factory");
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

static void check_aux_factory_list(asbind20::engine_pointer engine, int expected_val)
{
    auto* m = asbind20::create_module(engine, "test_aux_factory");
    m->AddScriptSection(
        "test_aux_factory",
        "int get() { test_aux_factory f = {10, 3}; return f.val; }"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("get");
    ASSERT_NE(f, nullptr);

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<int>(ctx, f);
    ASSERT_TRUE(asbind_test::result_has_value(result));

    EXPECT_EQ(result.value(), expected_val);
}
} // namespace test_bind

TEST(AuxFactoryNative, AsGlobal)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_bind::aux_factory_helper helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class<false>(engine)
        .factory_function("int", use_explicit, &test_bind::aux_factory_helper::create_aux_as_global, auxiliary(helper))
        .list_factory_function("repeat int", &test_bind::aux_factory_helper::create_aux_as_global_list, auxiliary(helper));

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    test_bind::check_aux_factory_list(engine, 1013);
}

TEST(AuxFactoryGeneric, AsGlobal)
{
    using namespace asbind20;

    test_bind::aux_factory_helper helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class<true>(engine)
        .factory_function("int", use_explicit, fp<&test_bind::aux_factory_helper::create_aux_as_global>, auxiliary(helper))
        .list_factory_function("repeat int", fp<&test_bind::aux_factory_helper::create_aux_as_global_list>, auxiliary(helper));

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    test_bind::check_aux_factory_list(engine, 1013);
}

TEST(AuxFactoryNative, AuxFirst)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_bind::aux_factory_helper helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class<false>(engine)
        .factory_function("int", use_explicit, &test_bind::create_aux_auxfirst, auxiliary(helper))
        .list_factory_function("repeat int", &test_bind::create_aux_auxfirst_list, auxiliary(helper));

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    test_bind::check_aux_factory_list(engine, 1013);
}

TEST(AuxFactoryGeneric, AuxFirst)
{
    using namespace asbind20;

    test_bind::aux_factory_helper helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class<true>(engine)
        .factory_function("int", use_explicit, fp<&test_bind::create_aux_auxfirst>, auxiliary(helper))
        .list_factory_function("repeat int", fp<&test_bind::create_aux_auxfirst_list>, auxiliary(helper));

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    test_bind::check_aux_factory_list(engine, 1013);
}

TEST(AuxFactoryNative, AuxLast)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_bind::aux_factory_helper helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class<false>(engine)
        .factory_function("int", use_explicit, &test_bind::create_aux_auxlast, auxiliary(helper))
        .list_factory_function("repeat int", &test_bind::create_aux_auxlast_list, auxiliary(helper));

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    test_bind::check_aux_factory_list(engine, 1013);
}

TEST(AuxFactoryGeneric, AuxLast)
{
    using namespace asbind20;

    test_bind::aux_factory_helper helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class<true>(engine)
        .factory_function("int", use_explicit, fp<&test_bind::create_aux_auxlast>, auxiliary(helper))
        .list_factory_function("repeat int", fp<&test_bind::create_aux_auxlast_list>, auxiliary(helper));

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    test_bind::check_aux_factory_list(engine, 1013);
}

TEST(AuxFactoryNative, AuxFirstManual)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_bind::aux_factory_helper helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class<false>(engine)
        .factory_function("int", use_explicit, &test_bind::create_aux_auxfirst, auxiliary(helper), objfirst)
        .list_factory_function("repeat int", &test_bind::create_aux_auxfirst_list, auxiliary(helper), objfirst);

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    test_bind::check_aux_factory_list(engine, 1013);
}

TEST(AuxFactoryGeneric, AuxFirstManual)
{
    using namespace asbind20;

    test_bind::aux_factory_helper helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class<true>(engine)
        .factory_function("int", use_explicit, fp<&test_bind::create_aux_auxfirst>, auxiliary(helper), objfirst)
        .list_factory_function("repeat int", fp<&test_bind::create_aux_auxfirst_list>, auxiliary(helper), objfirst);

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    test_bind::check_aux_factory_list(engine, 1013);
}

TEST(AuxFactoryNative, AuxLastManual)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_bind::aux_factory_helper helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class<false>(engine)
        .factory_function("int", use_explicit, &test_bind::create_aux_auxlast, auxiliary(helper), objlast)
        .list_factory_function("repeat int", &test_bind::create_aux_auxlast_list, auxiliary(helper), objlast);

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    test_bind::check_aux_factory_list(engine, 1013);
}

TEST(AuxFactoryGeneric, AuxLastManual)
{
    using namespace asbind20;

    test_bind::aux_factory_helper helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class<true>(engine)
        .factory_function("int", use_explicit, fp<&test_bind::create_aux_auxlast>, auxiliary(helper), objlast)
        .list_factory_function("repeat int", fp<&test_bind::create_aux_auxlast_list>, auxiliary(helper), objlast);

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory(engine, 1013, 13);
    test_bind::check_aux_factory_list(engine, 1013);
}

namespace test_bind
{
// Multipurpose
struct test_aux_factory_template
{
    test_aux_factory_template() = default;

    test_aux_factory_template(asbind20::typeinfo_pointer ti, int val)
        : value(val)
    {
        EXPECT_EQ(ti->GetSubTypeId(), AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    }

    test_aux_factory_template(asbind20::typeinfo_pointer ti, int initial_val, asbind20::script_init_list_repeat list)
        : value(initial_val)
    {
        EXPECT_EQ(ti->GetSubTypeId(), AS_NAMESPACE_QUALIFIER asTYPEID_INT32);

        int* start = (int*)list.data();
        int* sentinel = start + list.size();
        EXPECT_EQ(list.size(), 2);

        for(int* p = start; p != sentinel; ++p)
            value += *p;
    }

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
    ~test_aux_factory_template() = default;

    int m_counter = 1;
};

static bool aux_factory_helper_template_callback(asbind20::typeinfo_pointer ti, bool& no_gc)
{
    int subtype_id = ti->GetSubTypeId();
    if(asbind20::is_void_type(subtype_id))
        return false;

    no_gc = true;
    return true;
}

struct aux_factory_helper_template
{
    using mock_type = mock_create;
    int predefined_value = 0;
    std::shared_ptr<mock_type> mock = std::make_shared<mock_type>();

    aux_factory_helper_template() = default;
    explicit aux_factory_helper_template(int predefined_val)
        : predefined_value(predefined_val) {}

    aux_factory_helper_template(const aux_factory_helper_template&) = delete;
    aux_factory_helper_template(aux_factory_helper_template&&) = delete;

    test_aux_factory_template* create_aux_template_as_global(asbind20::typeinfo_pointer ti, int additional)
    {
        mock->on_create();
        return new test_aux_factory_template(ti, predefined_value + additional);
    }

    test_aux_factory_template* create_aux_template_as_global_list(asbind20::typeinfo_pointer ti, void* list_buf)
    {
        mock->on_list_create();
        return new test_aux_factory_template(
            ti, predefined_value, asbind20::script_init_list_repeat(list_buf)
        );
    }
};

static test_aux_factory_template* create_aux_template_auxfirst(aux_factory_helper_template& helper, asbind20::typeinfo_pointer ti, int additional)
{
    helper.mock->on_create();
    return new test_aux_factory_template(ti, helper.predefined_value + additional);
}

static test_aux_factory_template* create_aux_template_auxfirst_list(aux_factory_helper_template& helper, asbind20::typeinfo_pointer ti, void* list_buf)
{
    helper.mock->on_list_create();
    return new test_aux_factory_template(
        ti, helper.predefined_value, asbind20::script_init_list_repeat(list_buf)
    );
}

static test_aux_factory_template* create_aux_template_auxlast(asbind20::typeinfo_pointer ti, int additional, aux_factory_helper_template& helper)
{
    helper.mock->on_create();
    return new test_aux_factory_template(ti, helper.predefined_value + additional);
}

static test_aux_factory_template* create_aux_template_auxlast_list(asbind20::typeinfo_pointer ti, void* list_buf, aux_factory_helper_template& helper)
{
    helper.mock->on_list_create();
    return new test_aux_factory_template(
        ti, helper.predefined_value, asbind20::script_init_list_repeat(list_buf)
    );
}

template <bool UseGeneric>
auto register_test_class_template(asbind20::engine_pointer engine)
{
    using namespace asbind20;
    return asbind20::template_ref_class<test_aux_factory_template, UseGeneric>(engine, "test_aux_factory_template<T>")
        .template_callback(fp<&aux_factory_helper_template_callback>)
        .use(register_refcount_helper{})
        .property("int val", &test_aux_factory_template::value);
}

static void check_aux_factory_template(asbind20::engine_pointer engine, int expected_val, int arg)
{
    auto* m = asbind20::create_module(engine, "test_aux_factory_template");
    m->AddScriptSection(
        "test_aux_factory_template",
        "int get(int arg) { test_aux_factory_template<int> f(arg); return f.val; }"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("get");
    ASSERT_NE(f, nullptr);

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<int>(ctx, f, arg);
    ASSERT_TRUE(asbind_test::result_has_value(result));

    EXPECT_EQ(result.value(), expected_val);
}

static void check_aux_factory_template_list(asbind20::engine_pointer engine, int expected_val)
{
    auto* m = asbind20::create_module(engine, "test_aux_factory_template");
    m->AddScriptSection(
        "test_aux_factory_template",
        "int get() { test_aux_factory_template<int> f = {10, 3}; return f.val; }"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("get");
    ASSERT_NE(f, nullptr);

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<int>(ctx, f);
    ASSERT_TRUE(asbind_test::result_has_value(result));

    EXPECT_EQ(result.value(), expected_val);
}
} // namespace test_bind

TEST(AuxFactoryTemplateNative, AsGlobal)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_bind::aux_factory_helper_template helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class_template<false>(engine)
        .factory_function("int", use_explicit, &test_bind::aux_factory_helper_template::create_aux_template_as_global, auxiliary(helper))
        .list_factory_function("repeat int", &test_bind::aux_factory_helper_template::create_aux_template_as_global_list, auxiliary(helper));

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory_template(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory_template(engine, 1013, 13);
    test_bind::check_aux_factory_template_list(engine, 1013);
}

TEST(AuxFactoryTemplateGeneric, AsGlobal)
{
    using namespace asbind20;

    test_bind::aux_factory_helper_template helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class_template<true>(engine)
        .factory_function("int", use_explicit, fp<&test_bind::aux_factory_helper_template::create_aux_template_as_global>, auxiliary(helper))
        .list_factory_function("repeat int", fp<&test_bind::aux_factory_helper_template::create_aux_template_as_global_list>, auxiliary(helper));

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory_template(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory_template(engine, 1013, 13);
    test_bind::check_aux_factory_template_list(engine, 1013);
}

TEST(AuxFactoryTemplateNative, AuxFirst)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_bind::aux_factory_helper_template helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class_template<false>(engine)
        .factory_function("int", use_explicit, &test_bind::create_aux_template_auxfirst, auxiliary(helper))
        .list_factory_function("repeat int", &test_bind::create_aux_template_auxfirst_list, auxiliary(helper));

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory_template(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory_template(engine, 1013, 13);
    test_bind::check_aux_factory_template_list(engine, 1013);
}

TEST(AuxFactoryTemplateGeneric, AuxFirst)
{
    using namespace asbind20;

    test_bind::aux_factory_helper_template helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class_template<true>(engine)
        .factory_function("int", use_explicit, fp<&test_bind::create_aux_template_auxfirst>, auxiliary(helper))
        .list_factory_function("repeat int", fp<&test_bind::create_aux_template_auxfirst_list>, auxiliary(helper));

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory_template(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory_template(engine, 1013, 13);
    test_bind::check_aux_factory_template_list(engine, 1013);
}

TEST(AuxFactoryTemplateNative, AuxLast)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_bind::aux_factory_helper_template helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class_template<false>(engine)
        .factory_function("int", use_explicit, &test_bind::create_aux_template_auxlast, auxiliary(helper))
        .list_factory_function("repeat int", &test_bind::create_aux_template_auxlast_list, auxiliary(helper));

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory_template(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory_template(engine, 1013, 13);
    test_bind::check_aux_factory_template_list(engine, 1013);
}

TEST(AuxFactoryTemplateGeneric, AuxLast)
{
    using namespace asbind20;

    test_bind::aux_factory_helper_template helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class_template<true>(engine)
        .factory_function("int", use_explicit, fp<&test_bind::create_aux_template_auxlast>, auxiliary(helper))
        .list_factory_function("repeat int", fp<&test_bind::create_aux_template_auxlast_list>, auxiliary(helper));

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory_template(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory_template(engine, 1013, 13);
    test_bind::check_aux_factory_template_list(engine, 1013);
}

TEST(AuxFactoryTemplateNative, AuxFirstManual)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_bind::aux_factory_helper_template helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class_template<false>(engine)
        .factory_function("int", use_explicit, &test_bind::create_aux_template_auxfirst, auxiliary(helper), objfirst)
        .list_factory_function("repeat int", &test_bind::create_aux_template_auxfirst_list, auxiliary(helper), objfirst);

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory_template(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory_template(engine, 1013, 13);
    test_bind::check_aux_factory_template_list(engine, 1013);
}

TEST(AuxFactoryTemplateGeneric, AuxFirstManual)
{
    using namespace asbind20;

    test_bind::aux_factory_helper_template helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class_template<true>(engine)
        .factory_function("int", use_explicit, fp<&test_bind::create_aux_template_auxfirst>, auxiliary(helper), objfirst)
        .list_factory_function("repeat int", fp<&test_bind::create_aux_template_auxfirst_list>, auxiliary(helper), objfirst);

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory_template(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory_template(engine, 1013, 13);
    test_bind::check_aux_factory_template_list(engine, 1013);
}

TEST(AuxFactoryTemplateNative, AuxLastManual)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_bind::aux_factory_helper_template helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class_template<false>(engine)
        .factory_function("int", use_explicit, &test_bind::create_aux_template_auxlast, auxiliary(helper), objlast)
        .list_factory_function("repeat int", &test_bind::create_aux_template_auxlast_list, auxiliary(helper), objlast);

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory_template(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory_template(engine, 1013, 13);
    test_bind::check_aux_factory_template_list(engine, 1013);
}

TEST(AuxFactoryTemplateGeneric, AuxLastManual)
{
    using namespace asbind20;

    test_bind::aux_factory_helper_template helper(0);

    auto engine = asbind20::make_script_engine();
    test_bind::setup_env(engine);

    test_bind::register_test_class_template<true>(engine)
        .factory_function("int", use_explicit, fp<&test_bind::create_aux_template_auxlast>, auxiliary(helper), objlast)
        .list_factory_function("repeat int", fp<&test_bind::create_aux_template_auxlast_list>, auxiliary(helper), objlast);

    auto& mock = *helper.mock;
    EXPECT_CALL(mock, on_create()).Times(2);
    EXPECT_CALL(mock, on_list_create()).Times(1);

    test_bind::check_aux_factory_template(engine, 0, 0);
    helper.predefined_value = 1000;
    test_bind::check_aux_factory_template(engine, 1013, 13);
    test_bind::check_aux_factory_template_list(engine, 1013);
}
