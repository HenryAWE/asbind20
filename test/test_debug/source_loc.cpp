#include <asbind_test/framework.hpp>
#include <gmock/gmock-matchers.h>
#include <asbind20/debugging.hpp>

#ifdef ASBIND20_HAS_SCRIPT_FUNCTION_GET_DECLARED_AT

TEST(SourceLocation, FromFunction)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    auto* m = create_module(engine, "source_loc_from_function");
    m->AddScriptSection(
        "test.as",
        "int foo() { return 42; }\n"
        "int bar() { return foo(); }\n"
        "int foobar() { return bar(); }"
    );
    ASSERT_GE(m->Build(), 0);

    auto* foobar = m->GetFunctionByName("foobar");
    EXPECT_NE(foobar, nullptr);

    auto loc = debugging::script_source_location::from_function(foobar);
    EXPECT_STREQ(loc.function_name().safe_c_str(), "foobar");
    EXPECT_STREQ(loc.section_name().safe_c_str(), "test.as");
    EXPECT_EQ(loc.line(), 3);
    EXPECT_EQ(loc.column(), 1);

    const std::string desc = loc.description();
    EXPECT_THAT(
        desc,
        ::testing::HasSubstr("test.as")
    );
    EXPECT_THAT(
        desc,
        ::testing::HasSubstr("3:1")
    );
    EXPECT_THAT(
        desc,
        ::testing::EndsWith("foobar")
    );
}

#endif
