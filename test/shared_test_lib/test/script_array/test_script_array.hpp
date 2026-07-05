#pragma once

#include <asbind_test/framework.hpp>
#include <asbind_test/array.hpp>
#include <asbind_test/assertion.hpp>

namespace test_script_array
{
constexpr char helper_module_name[] = "test_ext_array";

constexpr char helper_module_script[] = R"AngelScript(class script_ipair
{
    int x;
    int y;

    script_ipair()
    {
        x = 0;
        y = 0;
    }

    script_ipair(int x, int y)
    {
        this.x = x;
        this.y = y;
    }

    bool opEquals(const script_ipair&in other) const
    {
        return this.x == other.x && this.y == other.y;
    }
};
)AngelScript";

template <bool UseGeneric>
class basic_array_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        using namespace asbind20;

        if constexpr(!UseGeneric)
            ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();
        m_engine = make_script_engine();

        asbind_test::setup_message_callback(m_engine, true);
        asbind_test::setup_exception_translator(m_engine);
        asbind_test::register_instantly_throw<UseGeneric>(m_engine);
        asbind_test::register_throw_on_copy<UseGeneric>(m_engine);
        asbind_test::setup_script_assertion(m_engine);
        asbind_test::register_script_array(m_engine, true, UseGeneric);

        build_helper_module();
    }

    void TearDown() override
    {
        m_engine.reset();
    }

    auto get_engine() const
        -> asbind20::engine_pointer
    {
        return m_engine.get();
    }

private:
    asbind20::script_engine m_engine;

    void build_helper_module()
    {
        auto* m = m_engine->GetModule(
            helper_module_name, AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        ASSERT_NE(m, nullptr);
        m->AddScriptSection(
            "test_ext_array_helper_module",
            helper_module_script
        );
        EXPECT_GE(m->Build(), 0);
    }
};

template <typename Return>
Return run_string(
    asbind20::engine_pointer engine,
    const char* section,
    std::string_view code,
    std::string_view return_decl
)
{
    int r = 0;

    std::string func_code = asbind20::string_concat(
        return_decl,
        " test_ext_array(){\n",
        code,
        "\n;}"
    );

    auto* m = engine->GetModule(
        helper_module_name, AS_NAMESPACE_QUALIFIER asGM_ONLY_IF_EXISTS
    );
    AS_NAMESPACE_QUALIFIER asIScriptFunction* f = nullptr;
    r = m->CompileFunction(
        section,
        func_code.c_str(),
        -1,
        0, // Not add to module
        &f
    );
    if(r < 0)
    {
        ADD_FAILURE() << "Failed to compile section \"" << section << '\"';
        std::terminate();
    }

    EXPECT_TRUE(f != nullptr);
    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<Return>(ctx, f);
    f->Release();

    if(result.error() == AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION)
    {
        ADD_FAILURE()
            << "GetExceptionString: " << ctx->GetExceptionString() << '\n'
            << "section: " << asbind20::debugging::get_function_section_name(ctx->GetExceptionFunction());
    }

    return result.value();
}

void run_string(
    asbind20::engine_pointer engine,
    const char* section,
    std::string_view code
);
} // namespace test_script_array
