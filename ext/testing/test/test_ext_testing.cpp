#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/ext/testing.hpp>

TEST(testing_framework, bool_val)
{
    using namespace asbind20;

    auto engine = asbind20::make_script_engine();

    ext::testing::suite s("bool_val_suite");
    EXPECT_FALSE(s.failed());

    asbind_test::setup_message_callback(engine, true);
    ext::register_script_test_framework(engine, s);

    auto* m = engine->GetModule(
        "bool_val", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "bool_val.as",
        "void good()\n"
        "{\n"
        "    testing::expect_true(true);\n"
        "    testing::expect_false(false);\n"
        "}\n"
        "void bad()\n"
        "{\n"
        "    testing::expect_false(true);\n"
        "    testing::expect_true(false);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* good = m->GetFunctionByName("good");
        ASSERT_TRUE(good);

        request_context ctx(engine);
        auto result = script_invoke<void>(ctx, good);

        EXPECT_TRUE(asbind_test::result_has_value(result));

        EXPECT_FALSE(s.failed());
    }

    {
        auto* bad = m->GetFunctionByName("bad");
        ASSERT_TRUE(bad);

        ::testing::internal::CaptureStdout();
        request_context ctx(engine);
        auto result = script_invoke<void>(ctx, bad);
        std::string output = testing::internal::GetCapturedStdout();

        EXPECT_TRUE(asbind_test::result_has_value(result));

        EXPECT_TRUE(s.failed());

        EXPECT_EQ(
            output,
            "[bool_val_suite] Expected: false\n"
            "[bool_val_suite] Actual: true\n"
            "[bool_val_suite] Func: void bad() (bool_val.as: 6:1)\n"
            "[bool_val_suite] Expected: true\n"
            "[bool_val_suite] Actual: false\n"
            "[bool_val_suite] Func: void bad() (bool_val.as: 6:1)\n"
        );
    }
}

TEST(testing_framework, overwriting)
{
    using namespace asbind20;

    auto engine = asbind20::make_script_engine();

    class my_suite : public ext::testing::suite
    {
    public:
        using suite::suite;

    protected:
        void write_message(std::string_view msg) override
        {
            std::ostream& os = get_ostream();

            os << '[' << "my_suite:" << get_name() << "]\n";

            os << msg << '\n';
        }
    };

    my_suite s("bool_val_suite");
    EXPECT_FALSE(s.failed());

    asbind_test::setup_message_callback(engine, true);
    ext::register_script_test_framework(engine, s);

    auto* m = engine->GetModule(
        "bool_val", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "bool_val.as",
        "void good()\n"
        "{\n"
        "    testing::expect_true(true);\n"
        "    testing::expect_false(false);\n"
        "}\n"
        "void bad()\n"
        "{\n"
        "    testing::expect_false(true);\n"
        "    testing::expect_true(false);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* good = m->GetFunctionByName("good");
        ASSERT_TRUE(good);

        request_context ctx(engine);
        auto result = script_invoke<void>(ctx, good);

        EXPECT_TRUE(asbind_test::result_has_value(result));

        EXPECT_FALSE(s.failed());
    }

    {
        auto* bad = m->GetFunctionByName("bad");
        ASSERT_TRUE(bad);

        ::testing::internal::CaptureStdout();
        request_context ctx(engine);
        auto result = script_invoke<void>(ctx, bad);
        std::string output = testing::internal::GetCapturedStdout();

        EXPECT_TRUE(asbind_test::result_has_value(result));

        EXPECT_TRUE(s.failed());

        EXPECT_EQ(
            output,
            "[my_suite:bool_val_suite]\n"
            "Expected: false\n"
            "Actual: true\n"
            "Func: void bad() (bool_val.as: 6:1)\n"
            "[my_suite:bool_val_suite]\n"
            "Expected: true\n"
            "Actual: false\n"
            "Func: void bad() (bool_val.as: 6:1)\n"
        );
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
