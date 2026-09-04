#include <asbind_test/framework.hpp>
#include <asbind20/util/script_result.hpp>

TEST(ScriptResult, NonNegative)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    {
        script_result<std::string> result(bad_script_result, -1);
        EXPECT_FALSE(result);
        EXPECT_EQ(result.error(), -1);

        result.emplace_value("val");
        EXPECT_TRUE(result);
        EXPECT_EQ(result.value(), "val");
        EXPECT_NE(result.error(), -1);
    }

    {
        script_result<std::string> result(
            std::piecewise_construct,
            std::forward_as_tuple(3, 'A'),
            std::forward_as_tuple(0)
        );
        EXPECT_TRUE(result);
        EXPECT_EQ(result.value(), "AAA");
        EXPECT_EQ(result.error(), 0);
        EXPECT_EQ(result.error_description(), "0");

        result.emplace_value("val");
        EXPECT_TRUE(result);
        EXPECT_EQ(result.value(), "val");
        EXPECT_GE(result.error(), 0);
    }
}

TEST(ScriptResult, ReturnCode)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;
    using asbind20::script_result_policy;

    {
        script_result<std::string, script_result_policy::return_code> result(
            bad_script_result, AS_NAMESPACE_QUALIFIER asERROR
        );
        EXPECT_FALSE(result);
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asERROR);

        result.emplace_value("val");
        EXPECT_TRUE(result);
        EXPECT_EQ(result.value(), "val");
        EXPECT_NE(result.error(), AS_NAMESPACE_QUALIFIER asERROR);
    }

    {
        script_result<std::string, script_result_policy::return_code> result(
            std::piecewise_construct,
            std::forward_as_tuple(3, 'A'),
            std::forward_as_tuple(0) // asSUCCESS
        );
        EXPECT_TRUE(result);
        EXPECT_EQ(result.value(), "AAA");
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);

        using asbind20::to_string;
        EXPECT_EQ(result.error_description(), to_string(AS_NAMESPACE_QUALIFIER asSUCCESS));

        result.emplace_value("val");
        EXPECT_TRUE(result);
        EXPECT_EQ(result.value(), "val");
    }
}
