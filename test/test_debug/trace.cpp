#include <asbind_test/framework.hpp>
#include <gmock/gmock-matchers.h>
#include <asbind20/debugging.hpp>
#include <asbind20/debugging/stacktrace.hpp>

TEST(Trace, Print)
{
    using namespace asbind20;

    auto engine = make_script_engine();

    std::stringstream ss;

    global<true>(engine)
        .function(
            "void trace()",
            [](generic_pointer gen) -> void
            {
                auto& ss = get_generic_auxiliary<std::stringstream>(gen);
                auto tr = debugging::stacktrace::current();
                ss << tr;
            },
            auxiliary(ss)
        );

    auto* m = asbind20::create_module(engine, "print_trace");
    m->AddScriptSection(
        "test.as",
        "int foo() { trace(); return 42; }\n"
        "int bar() { return foo(); }\n"
        "int foobar() { return bar(); }"
    );
    ASSERT_GE(m->Build(), 0);

    auto* foobar = m->GetFunctionByName("foobar");
    ASSERT_NE(foobar, nullptr);

    EXPECT_TRUE(ss.str().empty());
    request_context ctx(engine);
    auto result = script_invoke<int>(ctx, foobar);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 42);

    const std::string traced = ss.str();
    EXPECT_THAT(
        traced,
        ::testing::HasSubstr("#0 ")
    );
    EXPECT_THAT(
        traced,
        ::testing::HasSubstr("#1 ")
    );
    EXPECT_THAT(
        traced,
        ::testing::HasSubstr("#2 ")
    );
    EXPECT_THAT(
        traced,
        ::testing::HasSubstr("foo")
    );
    EXPECT_THAT(
        traced,
        ::testing::HasSubstr("bar")
    );
    EXPECT_THAT(
        traced,
        ::testing::HasSubstr("foobar")
    );
    EXPECT_THAT(
        traced,
        ::testing::HasSubstr(":test.as")
    );

#ifdef ASBIND20_HAS_SCRIPT_FUNCTION_GET_DECLARED_AT
    EXPECT_THAT(
        traced,
        ::testing::HasSubstr(":1")
    );
#endif
}
