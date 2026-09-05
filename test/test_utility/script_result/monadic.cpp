#include <asbind_test/framework.hpp>
#include <asbind20/util/script_result.hpp>

TEST(ScriptResult, AndThenNonNegative)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    {
        script_result<std::string> result(
            std::piecewise_construct,
            std::forward_as_tuple("abc"),
            std::forward_as_tuple(2)
        );
        auto mapped = result.and_then(
            [](const std::string& value)
            { return script_result<std::size_t>(value.size(), 5); }
        );
        static_assert(
            std::same_as<decltype(mapped), script_result<std::size_t>>
        );
        EXPECT_TRUE(mapped);
        EXPECT_EQ(*mapped, 3u);
        EXPECT_EQ(mapped.error(), 5);
        EXPECT_EQ(result.value(), "abc");
    }

    {
        bool invoked = false;
        script_result<std::string> result(bad_script_result, -1);
        auto mapped = result.and_then(
            [&](const std::string&)
            {
                invoked = true;
                return script_result<std::size_t>(0u);
            }
        );
        EXPECT_FALSE(invoked);
        EXPECT_FALSE(mapped);
        EXPECT_EQ(mapped.error(), -1);
    }

    {
        script_result<std::string> result("abc");
        auto mapped = std::move(result).and_then(
            [](std::string&& value)
            { return script_result<std::size_t>(value.size()); }
        );
        EXPECT_TRUE(mapped);
        EXPECT_EQ(*mapped, 3u);
    }
}

TEST(ScriptResult, AndThenReturnCode)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;
    using asbind20::script_result_policy;
    using return_code_result_t =
        script_result<std::string, script_result_policy::return_code>;

    {
        bool invoked = false;
        return_code_result_t result(
            bad_script_result, AS_NAMESPACE_QUALIFIER asERROR
        );
        auto mapped = result.and_then(
            [&](const std::string&)
            {
                invoked = true;
                return script_result<
                    std::size_t,
                    script_result_policy::return_code>(0u);
            }
        );
        static_assert(
            std::same_as<
                decltype(mapped),
                script_result<
                    std::size_t,
                    script_result_policy::return_code>>
        );
        EXPECT_FALSE(invoked);
        EXPECT_FALSE(mapped);
        EXPECT_EQ(mapped.error(), AS_NAMESPACE_QUALIFIER asERROR);
    }

    {
        return_code_result_t result(
            std::piecewise_construct,
            std::forward_as_tuple("abc"),
            std::forward_as_tuple(AS_NAMESPACE_QUALIFIER asSUCCESS)
        );
        auto mapped = result.and_then(
            [](const std::string& value)
            {
                return script_result<
                    std::size_t,
                    script_result_policy::return_code>(value.size());
            }
        );
        EXPECT_TRUE(mapped);
        EXPECT_EQ(*mapped, 3u);
        EXPECT_EQ(mapped.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    }
}

TEST(ScriptResult, OrElseNonNegative)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    {
        bool invoked = false;
        script_result<std::string> result(
            std::piecewise_construct,
            std::forward_as_tuple("abc"),
            std::forward_as_tuple(2)
        );
        auto mapped = result.or_else(
            [&](int)
            {
                invoked = true;
                return script_result<std::string>(bad_script_result, -1);
            }
        );
        static_assert(
            std::same_as<decltype(mapped), script_result<std::string>>
        );
        EXPECT_FALSE(invoked);
        EXPECT_TRUE(mapped);
        EXPECT_EQ(*mapped, "abc");
        EXPECT_EQ(mapped.error(), 2);
        EXPECT_EQ(result.value(), "abc");
    }

    {
        int received = 0;
        script_result<std::string> result(bad_script_result, -1);
        auto mapped = result.or_else(
            [&](int e)
            {
                received = e;
                return script_result<std::string>("recovered", 5);
            }
        );
        EXPECT_EQ(received, -1);
        EXPECT_TRUE(mapped);
        EXPECT_EQ(*mapped, "recovered");
        EXPECT_EQ(mapped.error(), 5);
    }

    {
        bool invoked = false;
        script_result<std::string> result("abc", 2);
        auto mapped = std::move(result).or_else(
            [&](int&&)
            {
                invoked = true;
                return script_result<std::string>("recovered");
            }
        );
        EXPECT_FALSE(invoked);
        EXPECT_TRUE(mapped);
        EXPECT_EQ(*mapped, "abc");
        EXPECT_EQ(mapped.error(), 2);
    }
}

TEST(ScriptResult, OrElseReturnCode)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;
    using asbind20::script_result_policy;
    using return_code_result_t =
        script_result<std::string, script_result_policy::return_code>;

    {
        bool invoked = false;
        return_code_result_t result(
            std::piecewise_construct,
            std::forward_as_tuple("abc"),
            std::forward_as_tuple(AS_NAMESPACE_QUALIFIER asSUCCESS)
        );
        auto mapped = result.or_else(
            [&](AS_NAMESPACE_QUALIFIER asERetCodes)
            {
                invoked = true;
                return return_code_result_t(
                    bad_script_result, AS_NAMESPACE_QUALIFIER asERROR
                );
            }
        );
        static_assert(std::same_as<decltype(mapped), return_code_result_t>);
        EXPECT_FALSE(invoked);
        EXPECT_TRUE(mapped);
        EXPECT_EQ(*mapped, "abc");
        EXPECT_EQ(mapped.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    }

    {
        auto received = AS_NAMESPACE_QUALIFIER asSUCCESS;
        return_code_result_t result(
            bad_script_result, AS_NAMESPACE_QUALIFIER asERROR
        );
        auto mapped = result.or_else(
            [&](AS_NAMESPACE_QUALIFIER asERetCodes e)
            {
                received = e;
                return return_code_result_t(
                    "recovered", AS_NAMESPACE_QUALIFIER asSUCCESS
                );
            }
        );
        EXPECT_EQ(received, AS_NAMESPACE_QUALIFIER asERROR);
        EXPECT_TRUE(mapped);
        EXPECT_EQ(*mapped, "recovered");
        EXPECT_EQ(mapped.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    }
}

TEST(ScriptResult, VoidTransformNonNegative)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    {
        script_result<void> result(2);
        auto mapped = result.transform(
            []
            { return std::size_t{3}; }
        );
        static_assert(
            std::same_as<decltype(mapped), script_result<std::size_t>>
        );
        EXPECT_TRUE(mapped);
        EXPECT_EQ(*mapped, 3u);
        EXPECT_EQ(mapped.error(), 2);
    }

    {
        script_result<void> result(2);
        auto mapped = result.transform(
            [] {}
        );
        static_assert(
            std::same_as<decltype(mapped), script_result<void>>
        );
        EXPECT_TRUE(mapped);
        EXPECT_EQ(mapped.error(), 2);
    }

    {
        bool invoked = false;
        script_result<void> result(bad_script_result, -1);
        auto mapped = result.transform(
            [&]
            {
                invoked = true;
                return std::size_t{3};
            }
        );
        EXPECT_FALSE(invoked);
        EXPECT_FALSE(mapped);
        EXPECT_EQ(mapped.error(), -1);
    }
}

TEST(ScriptResult, VoidTransformReturnCode)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;
    using asbind20::script_result_policy;
    using void_result_t =
        script_result<void, script_result_policy::return_code>;

    {
        void_result_t result(AS_NAMESPACE_QUALIFIER asSUCCESS);
        auto mapped = result.transform(
            []
            { return std::size_t{3}; }
        );
        static_assert(
            std::same_as<
                decltype(mapped),
                script_result<
                    std::size_t,
                    script_result_policy::return_code>>
        );
        EXPECT_TRUE(mapped);
        EXPECT_EQ(*mapped, 3u);
        EXPECT_EQ(mapped.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    }

    {
        void_result_t result(AS_NAMESPACE_QUALIFIER asSUCCESS);
        auto mapped = result.transform(
            [] {}
        );
        static_assert(
            std::same_as<decltype(mapped), void_result_t>
        );
        EXPECT_TRUE(mapped);
        EXPECT_EQ(mapped.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    }

    {
        bool invoked = false;
        void_result_t result(
            bad_script_result, AS_NAMESPACE_QUALIFIER asERROR
        );
        auto mapped = result.transform(
            [&]
            {
                invoked = true;
                return std::size_t{3};
            }
        );
        EXPECT_FALSE(invoked);
        EXPECT_FALSE(mapped);
        EXPECT_EQ(mapped.error(), AS_NAMESPACE_QUALIFIER asERROR);
    }
}

TEST(ScriptResult, TransformToVoidNonNegative)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    {
        bool invoked = false;
        script_result<std::string> result(
            std::piecewise_construct,
            std::forward_as_tuple("abc"),
            std::forward_as_tuple(2)
        );
        auto mapped = result.transform(
            [&](const std::string& value)
            {
                invoked = true;
                EXPECT_EQ(value, "abc");
            }
        );
        static_assert(
            std::same_as<decltype(mapped), script_result<void>>
        );
        EXPECT_TRUE(invoked);
        EXPECT_TRUE(mapped);
        EXPECT_EQ(mapped.error(), 2);
    }

    {
        bool invoked = false;
        script_result<std::string> result("abc");
        auto mapped = std::move(result).transform(
            [&](std::string&& value)
            {
                invoked = true;
                EXPECT_EQ(value, "abc");
            }
        );
        static_assert(
            std::same_as<decltype(mapped), script_result<void>>
        );
        EXPECT_TRUE(invoked);
        EXPECT_TRUE(mapped);
    }

    {
        bool invoked = false;
        script_result<std::string> result(bad_script_result, -1);
        auto mapped = result.transform(
            [&](const std::string&)
            {
                invoked = true;
            }
        );
        EXPECT_FALSE(invoked);
        EXPECT_FALSE(mapped);
        EXPECT_EQ(mapped.error(), -1);
    }
}

TEST(ScriptResult, TransformToVoidReturnCode)
{
    using asbind20::script_result;
    using asbind20::script_result_policy;
    using string_result_t =
        script_result<std::string, script_result_policy::return_code>;
    using void_result_t =
        script_result<void, script_result_policy::return_code>;

    {
        string_result_t result(
            std::piecewise_construct,
            std::forward_as_tuple("abc"),
            std::forward_as_tuple(AS_NAMESPACE_QUALIFIER asSUCCESS)
        );
        auto mapped = result.transform(
            [](const std::string&) {}
        );
        static_assert(std::same_as<decltype(mapped), void_result_t>);
        EXPECT_TRUE(mapped);
        EXPECT_EQ(mapped.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    }
}

TEST(ScriptResult, VoidAndThenNonNegative)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    {
        script_result<void> result(2);
        auto mapped = result.and_then(
            []
            { return script_result<std::size_t>(3u, 5); }
        );
        static_assert(
            std::same_as<decltype(mapped), script_result<std::size_t>>
        );
        EXPECT_TRUE(mapped);
        EXPECT_EQ(*mapped, 3u);
        EXPECT_EQ(mapped.error(), 5);
    }

    {
        bool invoked = false;
        script_result<void> result(bad_script_result, -1);
        auto mapped = result.and_then(
            [&]
            {
                invoked = true;
                return script_result<std::size_t>(3u);
            }
        );
        EXPECT_FALSE(invoked);
        EXPECT_FALSE(mapped);
        EXPECT_EQ(mapped.error(), -1);
    }
}

TEST(ScriptResult, VoidAndThenReturnCode)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;
    using asbind20::script_result_policy;
    using mapped_result_t =
        script_result<std::size_t, script_result_policy::return_code>;

    {
        script_result<void, script_result_policy::return_code> result(
            AS_NAMESPACE_QUALIFIER asSUCCESS
        );
        auto mapped = result.and_then(
            []
            { return mapped_result_t(3u); }
        );
        static_assert(
            std::same_as<decltype(mapped), mapped_result_t>
        );
        EXPECT_TRUE(mapped);
        EXPECT_EQ(*mapped, 3u);
        EXPECT_EQ(mapped.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    }

    {
        bool invoked = false;
        script_result<void, script_result_policy::return_code> result(
            bad_script_result, AS_NAMESPACE_QUALIFIER asERROR
        );
        auto mapped = result.and_then(
            [&]
            {
                invoked = true;
                return mapped_result_t(3u);
            }
        );
        EXPECT_FALSE(invoked);
        EXPECT_FALSE(mapped);
        EXPECT_EQ(mapped.error(), AS_NAMESPACE_QUALIFIER asERROR);
    }
}

TEST(ScriptResult, VoidOrElseNonNegative)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    {
        bool invoked = false;
        script_result<void> result(2);
        auto mapped = result.or_else(
            [&](int)
            {
                invoked = true;
                return script_result<void>(5);
            }
        );
        static_assert(
            std::same_as<decltype(mapped), script_result<void>>
        );
        EXPECT_FALSE(invoked);
        EXPECT_TRUE(mapped);
        EXPECT_EQ(mapped.error(), 2);
    }

    {
        int received = 0;
        script_result<void> result(bad_script_result, -1);
        auto mapped = result.or_else(
            [&](int e)
            {
                received = e;
                return script_result<void>(5);
            }
        );
        EXPECT_EQ(received, -1);
        EXPECT_TRUE(mapped);
        EXPECT_EQ(mapped.error(), 5);
    }

    {
        bool invoked = false;
        script_result<void> result(2);
        auto mapped = std::move(result).or_else(
            [&](int&&)
            {
                invoked = true;
                return script_result<void>(5);
            }
        );
        EXPECT_FALSE(invoked);
        EXPECT_TRUE(mapped);
        EXPECT_EQ(mapped.error(), 2);
    }
}

TEST(ScriptResult, VoidOrElseReturnCode)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;
    using asbind20::script_result_policy;
    using void_result_t =
        script_result<void, script_result_policy::return_code>;

    {
        bool invoked = false;
        void_result_t result(AS_NAMESPACE_QUALIFIER asSUCCESS);
        auto mapped = result.or_else(
            [&](AS_NAMESPACE_QUALIFIER asERetCodes)
            {
                invoked = true;
                return void_result_t(AS_NAMESPACE_QUALIFIER asSUCCESS);
            }
        );
        static_assert(std::same_as<decltype(mapped), void_result_t>);
        EXPECT_FALSE(invoked);
        EXPECT_TRUE(mapped);
        EXPECT_EQ(mapped.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    }

    {
        auto received = AS_NAMESPACE_QUALIFIER asSUCCESS;
        void_result_t result(
            bad_script_result, AS_NAMESPACE_QUALIFIER asERROR
        );
        auto mapped = result.or_else(
            [&](AS_NAMESPACE_QUALIFIER asERetCodes e)
            {
                received = e;
                return void_result_t(AS_NAMESPACE_QUALIFIER asSUCCESS);
            }
        );
        EXPECT_EQ(received, AS_NAMESPACE_QUALIFIER asERROR);
        EXPECT_TRUE(mapped);
        EXPECT_EQ(mapped.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    }
}
