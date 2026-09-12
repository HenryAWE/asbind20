#include <asbind_test/framework.hpp>
#include <asbind20/util/script_result.hpp>

TEST(ScriptResult, SwapNonNegative)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    {
        script_result<std::string> a(
            std::piecewise_construct,
            std::forward_as_tuple(1, 'A'),
            std::forward_as_tuple(2)
        );
        script_result<std::string> b(
            std::piecewise_construct,
            std::forward_as_tuple(2, 'B'),
            std::forward_as_tuple(5)
        );

        static_assert(noexcept(a.swap(b)));

        a.swap(b);
        EXPECT_TRUE(a);
        EXPECT_EQ(a.value(), "BB");
        EXPECT_EQ(a.error(), 5);
        EXPECT_TRUE(b);
        EXPECT_EQ(b.value(), "A");
        EXPECT_EQ(b.error(), 2);

        swap(a, b);
        EXPECT_TRUE(a);
        EXPECT_EQ(a.value(), "A");
        EXPECT_EQ(a.error(), 2);
        EXPECT_TRUE(b);
        EXPECT_EQ(b.value(), "BB");
        EXPECT_EQ(b.error(), 5);
    }

    {
        script_result<int> a(1, 2);
        script_result<int> b(bad_script_result, -1);

        a.swap(b);
        EXPECT_FALSE(a);
        EXPECT_EQ(a.error(), -1);
        EXPECT_TRUE(b);
        EXPECT_EQ(b.value(), 1);
        EXPECT_EQ(b.error(), 2);

        swap(a, b);
        EXPECT_TRUE(a);
        EXPECT_EQ(a.value(), 1);
        EXPECT_EQ(a.error(), 2);
        EXPECT_FALSE(b);
        EXPECT_EQ(b.error(), -1);
    }

    {
        script_result<int> a(bad_script_result, -1);
        script_result<int> b(bad_script_result, -3);

        a.swap(b);
        EXPECT_FALSE(a);
        EXPECT_EQ(a.error(), -3);
        EXPECT_FALSE(b);
        EXPECT_EQ(b.error(), -1);
    }

    {
        script_result<int> a(1);
        a.swap(a);
        EXPECT_TRUE(a);
        EXPECT_EQ(a.value(), 1);
    }
}

TEST(ScriptResult, SwapReturnCode)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;
    using asbind20::script_result_policy;
    using result_t = script_result<int, script_result_policy::return_code>;

    result_t a(1, AS_NAMESPACE_QUALIFIER asSUCCESS);
    result_t b(
        bad_script_result, AS_NAMESPACE_QUALIFIER asERROR
    );

    static_assert(noexcept(a.swap(b)));

    a.swap(b);
    EXPECT_FALSE(a);
    EXPECT_EQ(a.error(), AS_NAMESPACE_QUALIFIER asERROR);
    EXPECT_TRUE(b);
    EXPECT_EQ(b.value(), 1);
    EXPECT_EQ(b.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);

    swap(a, b);
    EXPECT_TRUE(a);
    EXPECT_EQ(a.value(), 1);
    EXPECT_EQ(a.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    EXPECT_FALSE(b);
    EXPECT_EQ(b.error(), AS_NAMESPACE_QUALIFIER asERROR);
}

TEST(ScriptResult, RefSwapNonNegative)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    {
        int a_val = 1;
        int b_val = 2;
        script_result<int&> a(a_val, 2);
        script_result<int&> b(b_val, 5);

        static_assert(noexcept(a.swap(b)));

        a.swap(b);
        EXPECT_TRUE(a);
        EXPECT_EQ(a.error(), 5);
        EXPECT_EQ(*a, 2);
        EXPECT_EQ(std::addressof(*a), &b_val);
        EXPECT_TRUE(b);
        EXPECT_EQ(b.error(), 2);
        EXPECT_EQ(*b, 1);
        EXPECT_EQ(std::addressof(*b), &a_val);

        swap(a, b);
        EXPECT_TRUE(a);
        EXPECT_EQ(a.error(), 2);
        EXPECT_EQ(*a, 1);
        EXPECT_EQ(std::addressof(*a), &a_val);
        EXPECT_TRUE(b);
        EXPECT_EQ(b.error(), 5);
        EXPECT_EQ(*b, 2);
        EXPECT_EQ(std::addressof(*b), &b_val);
    }

    {
        int a_val = 1;
        script_result<int&> a(a_val, 2);
        script_result<int&> b(bad_script_result, -1);

        a.swap(b);
        EXPECT_FALSE(a);
        EXPECT_EQ(a.error(), -1);
        EXPECT_TRUE(b);
        EXPECT_EQ(b.error(), 2);
        EXPECT_EQ(*b, 1);
        EXPECT_EQ(std::addressof(*b), &a_val);
    }
}

TEST(ScriptResult, RefSwapReturnCode)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;
    using asbind20::script_result_policy;
    using result_t = script_result<int&, script_result_policy::return_code>;

    int a_val = 1;
    result_t a(a_val, AS_NAMESPACE_QUALIFIER asSUCCESS);
    result_t b(
        bad_script_result, AS_NAMESPACE_QUALIFIER asERROR
    );

    static_assert(noexcept(a.swap(b)));

    a.swap(b);
    EXPECT_FALSE(a);
    EXPECT_EQ(a.error(), AS_NAMESPACE_QUALIFIER asERROR);
    EXPECT_TRUE(b);
    EXPECT_EQ(b.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    EXPECT_EQ(*b, 1);
    EXPECT_EQ(std::addressof(*b), &a_val);

    swap(a, b);
    EXPECT_TRUE(a);
    EXPECT_EQ(a.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    EXPECT_EQ(*a, 1);
    EXPECT_EQ(std::addressof(*a), &a_val);
    EXPECT_FALSE(b);
    EXPECT_EQ(b.error(), AS_NAMESPACE_QUALIFIER asERROR);
}

TEST(ScriptResult, VoidSwapNonNegative)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;

    script_result<void> a(2);
    script_result<void> b(bad_script_result, -1);

    static_assert(noexcept(a.swap(b)));

    a.swap(b);
    EXPECT_FALSE(a);
    EXPECT_EQ(a.error(), -1);
    EXPECT_TRUE(b);
    EXPECT_EQ(b.error(), 2);

    swap(a, b);
    EXPECT_TRUE(a);
    EXPECT_EQ(a.error(), 2);
    EXPECT_FALSE(b);
    EXPECT_EQ(b.error(), -1);
}

TEST(ScriptResult, VoidSwapReturnCode)
{
    using asbind20::bad_script_result;
    using asbind20::script_result;
    using asbind20::script_result_policy;
    using result_t = script_result<void, script_result_policy::return_code>;

    result_t a(AS_NAMESPACE_QUALIFIER asSUCCESS);
    result_t b(
        bad_script_result, AS_NAMESPACE_QUALIFIER asERROR
    );

    static_assert(noexcept(a.swap(b)));

    a.swap(b);
    EXPECT_FALSE(a);
    EXPECT_EQ(a.error(), AS_NAMESPACE_QUALIFIER asERROR);
    EXPECT_TRUE(b);
    EXPECT_EQ(b.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);

    swap(a, b);
    EXPECT_TRUE(a);
    EXPECT_EQ(a.error(), AS_NAMESPACE_QUALIFIER asSUCCESS);
    EXPECT_FALSE(b);
    EXPECT_EQ(b.error(), AS_NAMESPACE_QUALIFIER asERROR);
}
