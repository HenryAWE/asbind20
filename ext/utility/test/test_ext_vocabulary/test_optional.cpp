#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/ext/vocabulary.hpp>

namespace test_ext_vocabulary
{
template <bool UseGeneric>
class basic_script_optional_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        using namespace asbind20;

        if constexpr(!UseGeneric)
        {
            if(has_max_portability())
                GTEST_SKIP() << "AS_MAX_PORTABILITY";
        }

        m_engine = make_script_engine();

        asbind_test::setup_message_callback(m_engine, true);
        asbind_test::setup_exception_translator(m_engine);
        asbind_test::register_instantly_throw<UseGeneric>(m_engine);
        asbind_test::register_throw_on_copy<UseGeneric>(m_engine);
        ext::register_script_optional(m_engine, UseGeneric);
    }

    auto get_engine() const
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine;
    }

private:
    asbind20::script_engine m_engine;
};
} // namespace test_ext_vocabulary

using optional_native = test_ext_vocabulary::basic_script_optional_suite<false>;
using optional_generic = test_ext_vocabulary::basic_script_optional_suite<true>;

namespace test_ext_vocabulary
{
void optional_ex_safety(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "optional_ex_safety", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "optional_ex_safety",
        "bool test0()\n"
        "{\n"
        "    optional<instantly_throw> op(nullopt);\n"
        "    try\n"
        "    { op.emplace(); }\n"
        "    catch { return true; }\n"
        "    return false;\n"
        "}\n"
        "bool test1()\n"
        "{\n"
        "    try\n"
        "    { optional<instantly_throw> op; }\n"
        "    catch { return true; }\n"
        "    return false;\n"
        "}\n"
        "bool test2()\n"
        "{\n"
        "    throw_on_copy val;\n"
        "    try\n"
        "    { optional<throw_on_copy> op(val); }\n"
        "    catch { return true; }\n"
        "    return false;\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    asbind20::request_context ctx(engine);

    {
        auto result = asbind20::script_invoke<bool>(
            ctx, m->GetFunctionByName("test0")
        );
        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_TRUE(result.value());
    }

    {
        auto result = asbind20::script_invoke<bool>(
            ctx, m->GetFunctionByName("test1")
        );
        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_TRUE(result.value());
    }

    {
        auto result = asbind20::script_invoke<bool>(
            ctx, m->GetFunctionByName("test2")
        );
        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_TRUE(result.value());
    }
}
} // namespace test_ext_vocabulary

TEST_F(optional_native, exception_safety)
{
    test_ext_vocabulary::optional_ex_safety(get_engine());
}

TEST_F(optional_generic, exception_safety)
{
    test_ext_vocabulary::optional_ex_safety(get_engine());
}
