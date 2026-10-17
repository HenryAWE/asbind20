#include <asbind_test/framework.hpp>
#include <asbind_test/array.hpp>
#include <asbind_test/std_string.hpp>
#include <asbind20/io/section.hpp>
#include <iostream>

namespace test_script_string
{
static void test_print(const std::string& msg)
{
    std::cerr << msg << std::endl;
}

template <bool UseGeneric>
class script_string_file_suite : public ::testing::Test
{
protected:
    void SetUp() override
    {
        using namespace asbind20;

        if constexpr(!UseGeneric)
            ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

        m_engine = make_script_engine();
        asbind_test::setup_message_callback(m_engine);
        asbind_test::setup_exception_translator(m_engine);
        asbind_test::configure_engine_for_ext_string(m_engine);

        asbind_test::register_script_array(m_engine, true, UseGeneric);
        asbind_test::setup_script_char(m_engine, UseGeneric);
        asbind_test::setup_script_string(m_engine, UseGeneric, true);
        asbind_test::setup_string_utils(m_engine, UseGeneric);
        asbind_test::setup_script_assertion(m_engine);

        global<UseGeneric>(m_engine)
            .function("void print(const string&in msg)", fp<&test_print>);
    }

    void TearDown() override
    {
        m_engine.reset();
    }

public:
    [[nodiscard]]
    asbind20::engine_pointer get_engine() const
    {
        return m_engine.get();
    }

private:
    asbind20::script_engine m_engine;
};

static void run_script_file(asbind20::engine_pointer engine)
{
    using asbind20::io::load_file;

    auto* m = asbind20::create_module(engine, "test_string_script_file");
    ASSERT_THAT(m, ::testing::NotNull());

    int r = load_file(m, "script/test_string.as");
    ASSERT_GE(r, 0) << "Failed to load script file";
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByDecl("void main()");
    ASSERT_THAT(f, ::testing::NotNull());

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<void>(ctx, f);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
}
} // namespace test_script_string

using ScriptStringFileNative = test_script_string::script_string_file_suite<false>;
using ScriptStringFileGeneric = test_script_string::script_string_file_suite<true>;

TEST_F(ScriptStringFileNative, RunScriptFile)
{
    test_script_string::run_script_file(get_engine());
}

TEST_F(ScriptStringFileGeneric, RunScriptFile)
{
    test_script_string::run_script_file(get_engine());
}
