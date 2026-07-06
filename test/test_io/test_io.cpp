#include <gtest/gtest.h>
#include <asbind20/asbind.hpp>
#include <asbind_test/framework.hpp>
#include <asbind20/io/stream.hpp>
#include <sstream>

TEST(TestIO, IOStreamWrapper)
{
    std::stringstream ss;

    {
        auto engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(engine);

        auto* m = asbind20::create_module(engine, "test");
        m->AddScriptSection("test.as", "int getval() { return 1013; }");
        ASSERT_GE(m->Build(), 0);

        ASSERT_GE(asbind20::save_byte_code(ss, m, false), 0);
    }

    {
        auto engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(engine);

        auto* m = asbind20::create_module(engine, "test");
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

TEST(TestIO, MemoryWrapper)
{
    std::vector<std::byte> buf;

    {
        auto engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(engine);

        auto* m = asbind20::create_module(engine, "test");
        m->AddScriptSection("test.as", "int f(int add) { return 1000 + add; }");
        ASSERT_GE(m->Build(), 0);

        ASSERT_GE(
            asbind20::save_byte_code(std::back_inserter(buf), m, true), 0
        );
    }

    {
        auto engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(engine);

        auto* m = asbind20::create_module(engine, "test");
        {
            auto result = asbind20::load_byte_code(buf, m);
            auto [r, debug_info_stripped] = result;
            ASSERT_GE(r, 0);
            EXPECT_TRUE(result);
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
