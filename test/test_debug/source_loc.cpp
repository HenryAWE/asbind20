#include <asbind_test/framework.hpp>
#include <gmock/gmock-matchers.h>
#include <asbind20/debugging.hpp>

TEST(SourceLocation, Custom)
{
    using asbind20::debugging::script_source_location;

    {
        script_source_location loc;
        EXPECT_THAT(loc.section_name(), ::testing::IsEmpty());
        EXPECT_THAT(loc.function_name(), ::testing::IsEmpty());
        EXPECT_EQ(loc.line(), 0);
        EXPECT_EQ(loc.column(), 0);
    }

    {
        script_source_location loc("<system>");
        EXPECT_EQ(loc.section_name(), "<system>");
        EXPECT_THAT(loc.function_name(), ::testing::IsEmpty());
        EXPECT_EQ(loc.line(), 0);
        EXPECT_EQ(loc.column(), 0);
    }

    {
        script_source_location loc("<system>", "foo");
        EXPECT_EQ(loc.section_name(), "<system>");
        EXPECT_EQ(loc.function_name(), "foo");
        EXPECT_EQ(loc.line(), 0);
        EXPECT_EQ(loc.column(), 0);
    }

    {
        script_source_location loc("<system>", "foo", 3);
        EXPECT_EQ(loc.section_name(), "<system>");
        EXPECT_EQ(loc.function_name(), "foo");
        EXPECT_EQ(loc.line(), 3);
        EXPECT_EQ(loc.column(), 0);
    }

    {
        script_source_location loc("<system>", "foo", 3, 4);
        EXPECT_EQ(loc.section_name(), "<system>");
        EXPECT_EQ(loc.function_name(), "foo");
        EXPECT_EQ(loc.line(), 3);
        EXPECT_EQ(loc.column(), 4);
    }

    {
        script_source_location loc("<system>", 3);
        EXPECT_EQ(loc.section_name(), "<system>");
        EXPECT_THAT(loc.function_name(), ::testing::IsEmpty());
        EXPECT_EQ(loc.line(), 3);
        EXPECT_EQ(loc.column(), 0);
    }

    {
        script_source_location loc("<system>", 3, 4);
        EXPECT_EQ(loc.section_name(), "<system>");
        EXPECT_THAT(loc.function_name(), ::testing::IsEmpty());
        EXPECT_EQ(loc.line(), 3);
        EXPECT_EQ(loc.column(), 4);
    }
}

#ifdef ASBIND20_HAS_SCRIPT_FUNCTION_GET_DECLARED_AT

TEST(SourceLocation, FromFunction)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    auto* m = create_module(engine, "source_loc_from_function");
    m->AddScriptSection(
        "test.as",
        "int foo() { return 42; }\n"
        "int bar() { return foo(); }\n"
        "int foobar() { return bar(); }"
    );
    ASSERT_GE(m->Build(), 0);

    auto* foobar = m->GetFunctionByName("foobar");
    EXPECT_THAT(foobar, ::testing::NotNull());

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


TEST(SourceLocation, FromContext)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    using asbind20::debugging::script_source_location;
    static std::optional<script_source_location> loc;
    static std::optional<script_source_location> loc_lvl_1;
    loc.reset();
    loc_lvl_1.reset();

    global<true>(engine)
        .function(
            "void record_loc()",
            []() -> void
            {
                loc.emplace(script_source_location::from_current_context());
                loc_lvl_1.emplace(script_source_location::from_current_context(1));
            }
        );

    auto* m = create_module(engine, "source_loc_from_context");
    m->AddScriptSection(
        "test.as",
        "int foo() { return 42; }\n"
        "int bar() { record_loc(); return foo(); }\n"
        "int foobar() { return bar(); }"
    );
    ASSERT_GE(m->Build(), 0);

    auto* foobar = m->GetFunctionByName("foobar");
    EXPECT_THAT(foobar, ::testing::NotNull());
    request_context ctx(engine);
    auto result = script_invoke<int>(ctx, foobar);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    EXPECT_TRUE(loc.has_value());
    EXPECT_TRUE(loc_lvl_1.has_value());

    // Level 0 is the script function that called record_loc(),
    EXPECT_STREQ(loc->function_name().safe_c_str(), "bar");
    EXPECT_STREQ(loc->section_name().safe_c_str(), "test.as");
    EXPECT_EQ(loc->line(), 2);
    EXPECT_EQ(loc->column(), 13);

    // Level 1 is its caller, again located at the call site.
    EXPECT_STREQ(loc_lvl_1->function_name().safe_c_str(), "foobar");
    EXPECT_STREQ(loc_lvl_1->section_name().safe_c_str(), "test.as");
    EXPECT_EQ(loc_lvl_1->line(), 3);
    EXPECT_EQ(loc_lvl_1->column(), 16);
}
