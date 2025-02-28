#include <gtest/gtest.h>
#include <asbind20/asbind.hpp>
#include <shared_test_lib.hpp>
#include <asbind20/io/stream.hpp>
#include <sstream>

TEST(test_io, iostream_wrapper)
{
    std::stringstream ss;

    {
        auto engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(engine);

        auto* m = engine->GetModule("test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);
        m->AddScriptSection("test.as", "int getval() { return 1013; }");
        ASSERT_GE(m->Build(), 0);

        ASSERT_GE(asbind20::save_byte_code(ss, m, false), 0);
    }

    {
        auto engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(engine);

        auto* m = engine->GetModule("test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);
        {
            auto [r, debug_info_stripped] = asbind20::load_byte_code(ss, m);
            ASSERT_GE(r, 0);
            EXPECT_FALSE(debug_info_stripped);
        }

        auto* getval = m->GetFunctionByDecl("int getval()");
        ASSERT_NE(getval, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, getval);
        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 1013);
    }
}

TEST(test_io, memory_wrapper)
{
    std::vector<std::byte> buf;

    {
        auto engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(engine);

        auto* m = engine->GetModule("test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);
        m->AddScriptSection("test.as", "int f(int add) { return 1000 + add; }");
        ASSERT_GE(m->Build(), 0);

        ASSERT_GE(
            asbind20::save_byte_code(std::back_inserter(buf), m, true), 0
        );
    }

    {
        auto engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(engine);

        auto* m = engine->GetModule("test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);
        {
            auto [r, debug_info_stripped] = asbind20::load_byte_code(buf.data(), buf.size(), m);
            ASSERT_GE(r, 0);
            EXPECT_TRUE(debug_info_stripped);
        }

        auto* getval = m->GetFunctionByDecl("int f(int)");
        ASSERT_NE(getval, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, getval, 13);
        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 1013);
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
