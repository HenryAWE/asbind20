#include <gtest/gtest.h>
#include "shared.hpp"
#include <asbind20/asbind.hpp>

using asbind_test::asbind_test_suite;

TEST_F(asbind_test_suite, load_file)
{
    asIScriptEngine* engine = get_engine();
    asIScriptModule* m = engine->GetModule("test_load_file", asGM_ALWAYS_CREATE);

    ASSERT_TRUE(std::filesystem::exists("test_build_1.as"));

    ASSERT_GE(
        asbind20::load_file(m, "test_build_1.as", std::ios_base::binary),
        0
    );
    ASSERT_GE(m->Build(), 0);

    auto ma = asbind20::script_function<int(int, int, int)>(
        m->GetFunctionByName("ma")
    );
    ASSERT_TRUE(ma);

    asIScriptContext* ctx = engine->CreateContext();

    EXPECT_EQ(ma(ctx, 1, 2, 3), 1 * 2 + 3);

    ctx->Release();
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
