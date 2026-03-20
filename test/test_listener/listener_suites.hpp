#pragma once

#include <asbind_test/framework.hpp>

namespace test_listener
{
class general_listener_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        engine = asbind20::make_script_engine();
        ASSERT_TRUE(engine);

        // Error will be reported by listeners
        asbind_test::setup_message_callback(engine, false);
    }

    void TearDown() override
    {
        engine.reset();
    }

    asbind20::script_engine engine;
};
} // namespace test_listener
