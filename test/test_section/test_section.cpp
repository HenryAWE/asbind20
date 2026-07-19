#include <asbind_test/framework.hpp>
#include <asbind20/io/section.hpp>

namespace test_load
{
class test_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(m_engine);
    }

    void TearDown() override
    {
        m_engine.reset();
    }

    asbind20::engine_pointer get_engine() const
    {
        return m_engine.get();
    }

    asbind20::function_pointer get_func(
        asbind20::module_pointer m
    ) const
    {
        if(m == nullptr)
        {
            ADD_FAILURE() << "module is nullptr";
            return nullptr;
        }

        auto* f = m->GetFunctionByDecl("int func()");
        EXPECT_NE(f, nullptr)
            << "\"int func()\" not found";

        return f;
    }

    void check_result(asbind20::function_pointer f, int expected) const
    {
        ASSERT_TRUE(f);

        asbind20::request_context ctx(m_engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);

        EXPECT_EQ(result.value(), expected);
    }

private:
    asbind20::script_engine m_engine;
};
} // namespace test_load

using TestLoad = test_load::test_suite;

TEST_F(TestLoad, LoadStringView)
{
    auto engine = get_engine();

    using asbind20::io::load_string;

    auto* m = asbind20::create_module(engine, "TestLoad");
    std::string_view sv = "int func() { return 1013; }";

    int r = load_string(m, "LoadStringView", sv);
    EXPECT_GE(r, 0)
        << "r = " << asbind20::to_string(AS_NAMESPACE_QUALIFIER asERetCodes(r));
    ASSERT_GE(m->Build(), 0);

    auto f = get_func(m);
    EXPECT_STREQ(
        asbind20::debugging::get_function_section_name(f).safe_c_str(),
        "LoadStringView"
    );
    check_result(f, 1013);
}

TEST_F(TestLoad, LoadString)
{
    auto engine = get_engine();

    using asbind20::io::load_string;

    auto* m = asbind20::create_module(engine, "TestLoad");
    std::string str = "int func() { return " + std::to_string(1013) + "; }";

    int r = load_string(m, "LoadString", str);
    EXPECT_GE(r, 0)
        << "r = " << asbind20::to_string(AS_NAMESPACE_QUALIFIER asERetCodes(r));
    ASSERT_GE(m->Build(), 0);

    auto f = get_func(m);
    EXPECT_STREQ(
        asbind20::debugging::get_function_section_name(f).safe_c_str(),
        "LoadString"
    );
    check_result(f, 1013);
}

TEST_F(TestLoad, LoadFile)
{
    auto engine = get_engine();

    using asbind20::io::load_file;

    auto* m = asbind20::create_module(engine, "TestLoad");
    std::string str = "int func() { return " + std::to_string(1013) + "; }";

    int r = load_file(m, "script/func.as");
    EXPECT_GE(r, 0)
        << "r = " << asbind20::to_string(AS_NAMESPACE_QUALIFIER asERetCodes(r));
    ASSERT_GE(m->Build(), 0);

    auto f = get_func(m);
    EXPECT_STREQ(
        asbind20::debugging::get_function_section_name(f).safe_c_str(),
        "script/func.as"
    );
    check_result(f, 1013);
}
