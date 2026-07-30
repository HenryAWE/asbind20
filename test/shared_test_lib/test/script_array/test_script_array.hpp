#pragma once

#include <asbind_test/framework.hpp>
#include <asbind_test/array.hpp>
#include <asbind_test/assertion.hpp>
#include <asbind20/debugging/stacktrace.hpp>

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

    asbind20::engine_pointer get_engine() const
    {
        return m_engine.get();
    }

private:
    asbind20::script_engine m_engine;

    void build_helper_module() const
    {
        auto* m = asbind20::create_module(m_engine, helper_module_name);
        ASSERT_THAT(m, ::testing::NotNull());
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
    asbind20::cstring_ref section,
    std::string_view code,
    std::string_view return_decl
)
{
    std::string func_code = asbind20::string_concat(
        return_decl,
        " test_ext_array(){\n",
        code,
        "\n;}"
    );

    auto* m = asbind20::get_module(engine, helper_module_name);
    ASSERT_THAT(m, ::testing::NotNull());
    auto comp_result = asbind20::compile_function<Return()>(
        *m,
        section,
        func_code,
        -1
    );
    if(!comp_result)
    {
        ADD_FAILURE()
            << "Failed to compile section " << std::quoted(section.safe_c_str())
            << ", r = " << asbind20::to_string(comp_result.error());
        std::terminate();
    }

    auto& f = comp_result.get();
    asbind20::request_context ctx(engine);
    auto result = f(ctx);

    if(result.error() == AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION)
    {
        ADD_FAILURE()
            << "GetExceptionString: " << std::quoted(ctx->GetExceptionString()) << '\n'
            << "Script stack trace:\n"
            << asbind20::debugging::stacktrace::current();
    }

    return result.value();
}

void run_string(
    asbind20::engine_pointer engine,
    asbind20::cstring_ref section,
    std::string_view code
);
} // namespace test_script_array
