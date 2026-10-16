#include <asbind_test/framework.hpp>
#include <gmock/gmock.h>
#include <asbind20/debugging.hpp>
#include <asbind20/debugging/stacktrace.hpp>

#ifdef ASBIND20_HAS_FORMAT_LIBRARY

TEST(Trace, Format)
{
    EXPECT_EQ(
        asbind20::io::fmtlib::format(
            "{}", asbind20::debugging::stacktrace{}
        ),
        ""
    );
}

TEST(Trace, FormatterSpec)
{
    using namespace asbind20;
    namespace io = asbind20::io;

    auto engine = make_script_engine();

    struct data_type
    {
        debugging::stacktrace trace;
    };

    data_type data;

    global<true>(engine)
        .function(
            "void capture()",
            [](generic_pointer gen) -> void
            {
                auto& data = get_generic_auxiliary<data_type>(gen);
                data.trace = debugging::stacktrace::current();
            },
            auxiliary(data)
        );

    auto* m = create_module(engine, "formatter_spec");
    m->AddScriptSection(
        "test.as",
        "int foo() { capture(); return 0; }\n"
        "int bar() { return foo(); }"
    );
    ASSERT_GE(m->Build(), 0);

    auto* bar = m->GetFunctionByName("bar");
    ASSERT_THAT(bar, ::testing::NotNull());

    request_context ctx(engine);
    auto result = script_invoke<int>(ctx, bar);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);

    ASSERT_FALSE(data.trace.empty());
    debugging::stacktrace_entry entry = *data.trace.cbegin();
    const std::string desc = entry.description();
    ASSERT_FALSE(desc.empty());

    // `stacktrace_entry` accepts fill, alignment and width only
    EXPECT_EQ(io::fmtlib::format("{}", entry), desc);
    EXPECT_EQ(io::fmtlib::format("{:20}", entry), io::fmtlib::format("{:20}", desc));
    EXPECT_EQ(io::fmtlib::format("{:<20}", entry), io::fmtlib::format("{:<20}", desc));
    EXPECT_EQ(io::fmtlib::format("{:>20}", entry), io::fmtlib::format("{:>20}", desc));
    EXPECT_EQ(io::fmtlib::format("{:^20}", entry), io::fmtlib::format("{:^20}", desc));
    EXPECT_EQ(io::fmtlib::format("{:*>20}", entry), io::fmtlib::format("{:*>20}", desc));
    EXPECT_EQ(io::fmtlib::format("{:*^20}", entry), io::fmtlib::format("{:*^20}", desc));
    EXPECT_EQ(
        io::fmtlib::format("{:*^{}}", entry, 30),
        io::fmtlib::format("{:*^{}}", desc, 30)
    );

    // `basic_stacktrace` does not accept any format specifier
    EXPECT_EQ(io::fmtlib::format("{}", data.trace), data.trace.description());

#    if !defined(ASBIND20_NO_EXCEPTIONS)

    EXPECT_THROW(
        (void)io::fmtlib::vformat(
            "{:>10}", io::fmtlib::make_format_args(data.trace)
        ),
        io::fmtlib::format_error
    );
    EXPECT_THROW(
        (void)io::fmtlib::vformat(
            "{:*^10}", io::fmtlib::make_format_args(data.trace)
        ),
        io::fmtlib::format_error
    );

    // `stacktrace_entry` rejects the other options
    EXPECT_THROW(
        (void)io::fmtlib::vformat(
            "{:.3}", io::fmtlib::make_format_args(entry)
        ),
        io::fmtlib::format_error
    );
    EXPECT_THROW(
        (void)io::fmtlib::vformat(
            "{:s}", io::fmtlib::make_format_args(entry)
        ),
        io::fmtlib::format_error
    );
    EXPECT_THROW(
        (void)io::fmtlib::vformat(
            "{:#}", io::fmtlib::make_format_args(entry)
        ),
        io::fmtlib::format_error
    );

#    endif
}

#endif

TEST(Trace, Print)
{
    using namespace asbind20;

    auto engine = make_script_engine();

    struct data_type
    {
        std::ostringstream ss;
        std::vector<std::string> collected_entries;
    };

    data_type data;

    global<true>(engine)
        .function(
            "void trace()",
            [](generic_pointer gen) -> void
            {
                auto& data = get_generic_auxiliary<data_type>(gen);
                debugging::stacktrace tr = debugging::stacktrace::current();
                EXPECT_EQ(tr.size(), 3);
                EXPECT_THAT(tr, ::testing::Not(::testing::IsEmpty()));
                data.ss << tr;

                for(auto& entry : tr)
                {
                    data.collected_entries.push_back(
                        entry.get_function()->GetName()
                    );
                }
            },
            auxiliary(data)
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
    ASSERT_THAT(foobar, ::testing::NotNull());

    EXPECT_THAT(data.ss.str(), ::testing::IsEmpty());
    request_context ctx(engine);
    auto result = script_invoke<int>(ctx, foobar);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 42);

    EXPECT_THAT(
        data.collected_entries,
        ::testing::ElementsAre("foo", "bar", "foobar")
    );

    const std::string traced = data.ss.str();
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
