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

TEST(ScriptResult, RefNonNegative)
{
    using asbind20::bad_script_result;
    using asbind20::bad_script_result_access;
    using asbind20::script_result;

    {
        int val = 42;
        script_result<int&> result(
            std::piecewise_construct,
            std::forward_as_tuple(std::ref(val)),
            std::forward_as_tuple(0)
        );
        EXPECT_TRUE(result);
        EXPECT_EQ(result.error(), 0);
        EXPECT_EQ(result.value_or(0), 42);
        EXPECT_EQ(*result, 42);
        EXPECT_EQ(std::addressof(*result), &val);
    }

    {
        script_result<int&> result(
            bad_script_result, -1
        );
        EXPECT_FALSE(result);

#ifndef ASBIND20_NO_EXCEPTIONS
        EXPECT_THROW(result.value(), bad_script_result_access);
#endif
    }
}

TEST(ScriptResult, ContextState)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;
    using asbind20::script_result_policy;

    {
        script_result<std::string, script_result_policy::context_state> result(
            bad_script_result,
            AS_NAMESPACE_QUALIFIER asEXECUTION_ERROR
        );
        EXPECT_FALSE(result);
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_ERROR);

        result.emplace_value("val");
        EXPECT_TRUE(result);
        EXPECT_NE(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_ERROR);
    }

    {
        script_result<std::string, script_result_policy::context_state> result(
            std::piecewise_construct,
            std::forward_as_tuple(3, 'A'),
            std::forward_as_tuple(AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED)
        );
        EXPECT_TRUE(result);
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED);
    }
}

TEST(ScriptResult, MoveOnlyValue)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    script_result<std::unique_ptr<int>> source(bad_script_result, -1);
    source.emplace_value(std::make_unique<int>(42));
    EXPECT_TRUE(source);

    script_result<std::unique_ptr<int>> moved(std::move(source));
    EXPECT_TRUE(moved);
    ASSERT_NE(moved.value(), nullptr);
    EXPECT_EQ(*moved.value(), 42);

    script_result<std::unique_ptr<int>> assigned(bad_script_result, -2);
    assigned = std::move(moved);
    EXPECT_TRUE(assigned);
    ASSERT_NE(assigned.value(), nullptr);
    EXPECT_EQ(*assigned.value(), 42);
}

namespace
{
class tracked_value
{
public:
    static inline int copy_count = 0;
    static inline int move_count = 0;

    tracked_value() = default;

    explicit tracked_value(int value) noexcept
        : m_value(value) {}

    tracked_value(const tracked_value& other) noexcept
        : m_value(other.m_value)
    {
        ++copy_count;
    }

    tracked_value(tracked_value&& other) noexcept
        : m_value(other.m_value)
    {
        other.m_value = -1;
        ++move_count;
    }

    tracked_value& operator=(const tracked_value&) = default;
    tracked_value& operator=(tracked_value&&) = default;

    [[nodiscard]]
    int value() const noexcept
    {
        return m_value;
    }

private:
    int m_value = 0;
};
} // namespace

TEST(ScriptResult, RValueConstruction)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    static_assert(std::is_nothrow_constructible_v<
                  script_result<tracked_value>,
                  tracked_value&&>);
    static_assert(!std::is_nothrow_constructible_v<
                  script_result<tracked_value>,
                  const tracked_value&>);

    // The r-value constructor must move the value instead of copying it
    {
        tracked_value::copy_count = 0;
        tracked_value::move_count = 0;

        tracked_value value(42);
        script_result<tracked_value> result(std::move(value));

        EXPECT_TRUE(result);
        EXPECT_EQ(result.value().value(), 42);
        EXPECT_EQ(tracked_value::copy_count, 0);
        EXPECT_EQ(tracked_value::move_count, 1);
    }

    // The l-value constructor still copies
    {
        tracked_value::copy_count = 0;
        tracked_value::move_count = 0;

        const tracked_value value(7);
        script_result<tracked_value> result(value);

        EXPECT_TRUE(result);
        EXPECT_EQ(result.value().value(), 7);
        EXPECT_EQ(tracked_value::copy_count, 1);
        EXPECT_EQ(tracked_value::move_count, 0);
    }

    // Piecewise construction must forward the tuple elements
    {
        tracked_value::copy_count = 0;
        tracked_value::move_count = 0;

        script_result<tracked_value> result(
            std::piecewise_construct,
            std::forward_as_tuple(tracked_value(9)),
            std::forward_as_tuple(0)
        );

        EXPECT_TRUE(result);
        EXPECT_EQ(result.value().value(), 9);
        EXPECT_EQ(tracked_value::copy_count, 0);
        EXPECT_EQ(tracked_value::move_count, 1);
    }

    // l-values are still copied, leaving the source intact
    {
        tracked_value value(5);
        auto args = std::forward_as_tuple(value);

        tracked_value::copy_count = 0;
        tracked_value::move_count = 0;

        script_result<tracked_value> result(
            std::piecewise_construct, args, std::forward_as_tuple(0)
        );

        EXPECT_TRUE(result);
        EXPECT_EQ(result.value().value(), 5);
        EXPECT_EQ(tracked_value::copy_count, 1);
        EXPECT_EQ(tracked_value::move_count, 0);
        EXPECT_EQ(value.value(), 5);
    }

    // Move-only type through the r-value constructor
    {
        script_result<std::unique_ptr<int>> result(std::make_unique<int>(3));

        ASSERT_TRUE(result);
        ASSERT_NE(result.value(), nullptr);
        EXPECT_EQ(*result.value(), 3);
    }

    // A failing status is still normalized when a value is present
    {
        script_result<tracked_value> result(tracked_value(1), -1);

        EXPECT_TRUE(result);
        EXPECT_EQ(result.error(), 0);
        EXPECT_EQ(result.value().value(), 1);
    }
}

TEST(ScriptResult, Compare)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    {
        script_result<int> a(bad_script_result, -1);
        script_result<int> b(bad_script_result, -1);
        EXPECT_TRUE(a == b);
        EXPECT_EQ(a <=> b, std::partial_ordering::equivalent);
    }

    {
        script_result<int> a(bad_script_result, -1);
        script_result<int> b(bad_script_result, -2);
        EXPECT_FALSE(a == b);
        EXPECT_EQ(a <=> b, std::partial_ordering::unordered);
    }

    {
        script_result<int> a(1);
        script_result<int> b(bad_script_result, -1);
        EXPECT_FALSE(a == b);
        EXPECT_EQ(a <=> b, std::partial_ordering::unordered);
    }

    {
        script_result<int> a(1);
        script_result<int> b(2);
        EXPECT_TRUE(a != b);
        EXPECT_EQ(a <=> b, std::partial_ordering::less);
        EXPECT_EQ(b <=> a, std::partial_ordering::greater);
    }
}
