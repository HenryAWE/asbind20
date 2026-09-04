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

        EXPECT_EQ(
            result.transform(std::identity{}).error(),
            AS_NAMESPACE_QUALIFIER asERROR
        );

        result.emplace_value("val");
        EXPECT_TRUE(result);
        EXPECT_EQ(result.value(), "val");
        EXPECT_NE(result.error(), AS_NAMESPACE_QUALIFIER asERROR);

        auto transform_result = result.transform(std::identity{});
        EXPECT_TRUE(transform_result);
        EXPECT_EQ(*result, "val");
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

TEST(ScriptResult, VoidNonNegative)
{
    using asbind20::bad_script_result;
    using asbind20::bad_script_result_access;
    using asbind20::script_result;

    static_assert(std::same_as<script_result<void>::value_type, void>);
    static_assert(
        std::same_as<script_result<void>::rebind<int>, script_result<int>>
    );

    {
        script_result<void> result;
        EXPECT_TRUE(result);
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.error(), 0);

        result.value();
        (void)*result;
    }

    {
        script_result<void> result(bad_script_result, -1);
        EXPECT_FALSE(result);
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), -1);
        EXPECT_EQ(result.error_description(), "-1");

#ifndef ASBIND20_NO_EXCEPTIONS
        EXPECT_THROW(result.value(), bad_script_result_access);
#endif

        result.emplace_value();
        EXPECT_TRUE(result);
        EXPECT_EQ(result.error(), 0);
        result.value();
    }

    {
        // A success status is normalized to a bad status
        // when passed to the bad result constructor
        script_result<void> result(bad_script_result, 0);
        EXPECT_FALSE(result);
        EXPECT_EQ(result.error(), -1);
    }

    {
        script_result<void> result(3);
        EXPECT_TRUE(result);
        EXPECT_EQ(result.error(), 3);
    }
}

TEST(ScriptResult, VoidReturnCode)
{
    using asbind20::bad_script_result;
    using asbind20::bad_script_result_access;
    using asbind20::script_result;
    using asbind20::script_result_policy;

    {
        script_result<void, script_result_policy::return_code> result(
            bad_script_result, AS_NAMESPACE_QUALIFIER asERROR
        );
        EXPECT_FALSE(result);
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asERROR);

        using asbind20::to_string;
        EXPECT_EQ(
            result.error_description(),
            to_string(AS_NAMESPACE_QUALIFIER asERROR)
        );

#ifndef ASBIND20_NO_EXCEPTIONS
        EXPECT_THROW(result.value(), bad_script_result_access);
#endif

        result.emplace_value();
        EXPECT_TRUE(result);
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
        result.value();
    }

    {
        script_result<void, script_result_policy::return_code> result(
            AS_NAMESPACE_QUALIFIER asSUCCESS
        );
        EXPECT_TRUE(result);
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    }
}
