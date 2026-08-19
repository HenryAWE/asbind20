#include <asbind_test/framework.hpp>
#if __has_include(<stdfloat>)
#    include <stdfloat>
#endif

#ifdef __GNUC__
#    pragma GCC diagnostic ignored "-Wnarrowing"
#endif


#ifdef __STDCPP_FLOAT16_T__

namespace
{
float f16_to_float(std::float16_t val)
{
    return static_cast<float>(val);
}

void check_f16(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(
        engine, "check_f16"
    );
    ASSERT_THAT(m, ::testing::NotNull());
    m->AddScriptSection(
        "check_f16",
        "float16 get_val() { return float16(3.14); }\n"
        "float test0(float16 val)\n"
        "{\n"
        "    return f16_to_float(val);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        SCOPED_TRACE("script func: get_val");

        auto* get_val = m->GetFunctionByName("get_val");
        ASSERT_THAT(get_val, ::testing::NotNull());

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::float16_t>(ctx, get_val);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_NEAR(
            static_cast<float>(result.value()),
            3.14f,
            0.01f
        );
    }
}
} // namespace

TEST(TestBind, Float16Native)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    using namespace asbind20;
    using std::float16_t;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    value_class<float16_t>(
        engine,
        "float16",
        AS_NAMESPACE_QUALIFIER asOBJ_POD
    )
        .default_constructor()
        .copy_constructor()
        .constructor_function(
            "float",
            [](std::float16_t* mem, float val) -> void
            { new(mem) float16_t(static_cast<float>(val)); }
        )
        .opAdd()
        .opAddAssign()
        .opSub()
        .opSubAssign();

    global(engine)
        .function("float f16_to_float(float16 val)", fp<&f16_to_float>);

    check_f16(engine);
}

#endif
