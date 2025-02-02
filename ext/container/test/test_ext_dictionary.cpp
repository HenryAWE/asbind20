#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/dictionary.hpp>

using namespace asbind_test;

namespace test_ext_dictionary
{
void check_emplace(asIScriptEngine* engine)
{
    asIScriptModule* m = engine->GetModule("dictionary_emplace", asGM_ALWAYS_CREATE);
    m->AddScriptSection(
        "dictionary_emplace.as",
        "void check1()\n"
        "{\n"
        "dictionary d;\n"
        "d.try_emplace(\"val\", 42);\n"
        "assert(d.contains(\"val\"));\n"
        "int val = -1;\n"
        "assert(d.get(\"val\", val));\n"
        "assert(val == 42);\n"
        "assert(d.erase(\"val\"));\n"
        "assert(!d.contains(\"val\"));\n"
        "}\n"
        "void check2()\n"
        "{\n"
        "dictionary d;\n"
        "d.try_emplace(\"val\", \"hello\");\n"
        "assert(d.contains(\"val\"));\n"
        "string val = \"old\";\n"
        "assert(d.get(\"val\", val));\n"
        "print(\"out: \" + val);\n"
        "assert(val == \"hello\");\n"
        "assert(d.erase(\"val\"));\n"
        "assert(!d.contains(\"val\"));\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* f = m->GetFunctionByName("check1");
        ASSERT_TRUE(f);

        auto result = asbind20::script_invoke<void>(ctx, f);
        EXPECT_TRUE(result_has_value(result));
    }

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* f = m->GetFunctionByName("check2");
        ASSERT_TRUE(f);

        auto result = asbind20::script_invoke<void>(ctx, f);
        std::cerr << ctx.get()->GetExceptionString() << std::endl;
        EXPECT_TRUE(result_has_value(result));
    }
}
} // namespace test_ext_dictionary

TEST_F(asbind_test_suite, ext_dictionary_emplace)
{
    asIScriptEngine* engine = get_engine();
    asbind20::ext::register_script_dictionary(engine);

    test_ext_dictionary::check_emplace(engine);
}

TEST_F(asbind_test_suite_generic, ext_dictionary_emplace)
{
    asIScriptEngine* engine = get_engine();
    asbind20::ext::register_script_dictionary(engine, true);

    test_ext_dictionary::check_emplace(engine);
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
