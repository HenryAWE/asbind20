#include <asbind_test/framework.hpp>
#include <asbind20/ranges/typeinfo_views.hpp>
#include <gmock/gmock.h>

namespace test_bind
{
class lambda_factory_class
{
public:
    lambda_factory_class() = default;

    explicit lambda_factory_class(int val)
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
    ~lambda_factory_class() = default;

    int m_counter = 1;
};

struct lambda_factory_helper
{
    int predefined_value = 0;
};

// Register the same set of lambda factories for both calling conventions.
//
// The factories must have different parameter lists,
// so different primitive types are used on purpose.
template <bool UseGeneric>
auto register_lambda_factory_class(
    asbind20::engine_pointer engine, lambda_factory_helper& helper
)
{
    using namespace asbind20;
    using class_type = lambda_factory_class;

    return ref_class<class_type, UseGeneric>(engine, "lambda_factory_class")
        .addref(fp<&class_type::addref>)
        .release(fp<&class_type::release>)
        .property("int value", &class_type::value)
        // Ordinary factory
        .factory_function(
            "int",
            [](int val) -> class_type*
            { return new class_type(val); }
        )
        // Explicit factory
        .factory_function(
            "int, int",
            use_explicit,
            [](int lhs, int rhs) -> class_type*
            { return new class_type(lhs + rhs); }
        )
        // Auxiliary object at the first position, located by deduction
        .factory_function(
            "int64",
            [](lambda_factory_helper& h, int64_t val) -> class_type*
            {
                return new class_type(
                    h.predefined_value + static_cast<int>(val)
                );
            },
            auxiliary(helper)
        )
        // Auxiliary object at the last position, located by deduction
        .factory_function(
            "uint",
            [](unsigned int val, lambda_factory_helper& h) -> class_type*
            {
                return new class_type(
                    h.predefined_value + static_cast<int>(val)
                );
            },
            auxiliary(helper)
        )
        // Auxiliary object with a manually specified position
        .factory_function(
            "int16",
            [](lambda_factory_helper& h, int16_t val) -> class_type*
            {
                return new class_type(
                    h.predefined_value + static_cast<int>(val)
                );
            },
            auxiliary(helper),
            objfirst
        )
        .factory_function(
            "uint16",
            [](uint16_t val, lambda_factory_helper& h) -> class_type*
            {
                return new class_type(
                    h.predefined_value + static_cast<int>(val)
                );
            },
            auxiliary(helper),
            objlast
        );
}

const char* lambda_factory_test_script = R"(
int plain()
{
    lambda_factory_class val(42);
    return val.value;
}
int explicit_ctor()
{
    lambda_factory_class val(1, 2);
    return val.value;
}
int aux_first()
{
    lambda_factory_class val(int64(3));
    return val.value;
}
int aux_last()
{
    lambda_factory_class val(uint(4));
    return val.value;
}
int aux_first_tag()
{
    lambda_factory_class val(int16(5));
    return val.value;
}
int aux_last_tag()
{
    lambda_factory_class val(uint16(6));
    return val.value;
}
)";

void check_lambda_factory(
    asbind20::engine_pointer engine, const char* func_name, int expected
)
{
    auto* m = asbind20::create_module(engine, "test_lambda_factory");
    m->AddScriptSection("test_lambda_factory", lambda_factory_test_script);
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName(func_name);
    ASSERT_THAT(f, ::testing::NotNull()) << func_name;

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<int>(ctx, f);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), expected) << func_name;
}
} // namespace test_bind

TEST(LambdaFactory, Native)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_script_assertion(engine);

    test_bind::lambda_factory_helper helper;
    helper.predefined_value = 1000;

    test_bind::register_lambda_factory_class<false>(engine.get(), helper);

    test_bind::check_lambda_factory(engine.get(), "plain", 42);
    test_bind::check_lambda_factory(engine.get(), "explicit_ctor", 3);
    test_bind::check_lambda_factory(engine.get(), "aux_first", 1003);
    test_bind::check_lambda_factory(engine.get(), "aux_last", 1004);
    test_bind::check_lambda_factory(engine.get(), "aux_first_tag", 1005);
    test_bind::check_lambda_factory(engine.get(), "aux_last_tag", 1006);
}

TEST(LambdaFactory, Generic)
{
    auto engine = asbind20::make_script_engine();
    asbind_test::setup_script_assertion(engine);

    test_bind::lambda_factory_helper helper;
    helper.predefined_value = 1000;

    test_bind::register_lambda_factory_class<true>(engine.get(), helper);

    test_bind::check_lambda_factory(engine.get(), "plain", 42);
    test_bind::check_lambda_factory(engine.get(), "explicit_ctor", 3);
    test_bind::check_lambda_factory(engine.get(), "aux_first", 1003);
    test_bind::check_lambda_factory(engine.get(), "aux_last", 1004);
    test_bind::check_lambda_factory(engine.get(), "aux_first_tag", 1005);
    test_bind::check_lambda_factory(engine.get(), "aux_last_tag", 1006);
}

// Cover every `use_generic` overload of the lambda form.
TEST(LambdaFactory, UseGenericTag)
{
    using namespace asbind20;
    using class_type = test_bind::lambda_factory_class;

    auto engine = make_script_engine();
    asbind_test::setup_script_assertion(engine);

    test_bind::lambda_factory_helper helper;
    helper.predefined_value = 1000;

    ref_class<class_type> c(engine, "lambda_factory_class");
    c
        .addref(fp<&class_type::addref>)
        .release(fp<&class_type::release>)
        .property("int value", &class_type::value)
        .factory_function(
            use_generic,
            "int",
            [](int val) -> class_type*
            { return new class_type(val); }
        )
        .factory_function(
            use_generic,
            "int, int",
            use_explicit,
            [](int lhs, int rhs) -> class_type*
            { return new class_type(lhs + rhs); }
        )
        .factory_function(
            use_generic,
            "int64",
            [](test_bind::lambda_factory_helper& h, int64_t val) -> class_type*
            {
                return new class_type(
                    h.predefined_value + static_cast<int>(val)
                );
            },
            auxiliary(helper)
        )
        .factory_function(
            use_generic,
            "uint",
            use_explicit,
            [](unsigned int val, test_bind::lambda_factory_helper& h) -> class_type*
            {
                return new class_type(
                    h.predefined_value + static_cast<int>(val)
                );
            },
            auxiliary(helper)
        )
        .factory_function(
            use_generic,
            "int16",
            [](test_bind::lambda_factory_helper& h, int16_t val) -> class_type*
            {
                return new class_type(
                    h.predefined_value + static_cast<int>(val)
                );
            },
            auxiliary(helper),
            objfirst
        )
        .factory_function(
            use_generic,
            "uint16",
            use_explicit,
            [](uint16_t val, test_bind::lambda_factory_helper& h) -> class_type*
            {
                return new class_type(
                    h.predefined_value + static_cast<int>(val)
                );
            },
            auxiliary(helper),
            objlast
        );

    test_bind::check_lambda_factory(engine.get(), "plain", 42);
    test_bind::check_lambda_factory(engine.get(), "explicit_ctor", 3);
    test_bind::check_lambda_factory(engine.get(), "aux_first", 1003);
    test_bind::check_lambda_factory(engine.get(), "aux_last", 1004);
    test_bind::check_lambda_factory(engine.get(), "aux_first_tag", 1005);
    test_bind::check_lambda_factory(engine.get(), "aux_last_tag", 1006);
}

namespace
{
template <bool UseGeneric>
void check_lambda_factory_explicit(asbind20::engine_pointer engine)
{
    using namespace asbind20;
    using class_type = test_bind::lambda_factory_class;

    ref_class<class_type, UseGeneric> c(engine, "lambda_factory_class");
    c
        .addref(fp<&class_type::addref>)
        .release(fp<&class_type::release>)
        .factory_function(
            "int",
            use_explicit,
            [](int val) -> class_type*
            { return new class_type(val); }
        );

    auto* ti = engine->GetTypeInfoById(c.get_type_id());
    ASSERT_THAT(ti, ::testing::NotNull());

    std::size_t matched = 0;
    for(auto f : ranges::views::all_factories(ti))
    {
        ++matched;
        EXPECT_TRUE(f->IsExplicit()) << "id = " << f->GetId();
    }

    EXPECT_GT(matched, 0);
}
} // namespace

TEST(LambdaFactory, ExplicitNative)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);

    check_lambda_factory_explicit<false>(engine.get());
}

TEST(LambdaFactory, ExplicitGeneric)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);

    check_lambda_factory_explicit<true>(engine.get());
}
