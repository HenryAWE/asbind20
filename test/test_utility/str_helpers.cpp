#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>

TEST(Utility, StringConcat)
{
    using asbind20::string_concat;

    EXPECT_EQ(string_concat(), "");

    {
        const char* name = "my_name";
        EXPECT_EQ(string_concat("void f(", name, ')'), "void f(my_name)");
    }

    {
        using namespace std::string_view_literals;
        const char* name = "my_name";
        EXPECT_EQ(string_concat("void f("sv, name, ')'), "void f(my_name)");
    }
}

TEST(WithCStr, WithCStr)
{
    using namespace asbind20;
    using namespace std::string_view_literals;

    {
        std::string result = util::with_cstr(
            [](const char* a, char ch, const char* b, const char* c, const char* d)
            {
                return string_concat(a, ch, b, c, d);
            },
            "hello",
            ' ',
            "hello world"sv.substr(0, 5),
            cstring_ref("!"),
            util::fixed_string("!!")
        );
        EXPECT_EQ(result, "hello hello!!!");
    }

    {
        std::string result = util::with_cstr(
            [](const char* a, char ch, const char* b, const char* c)
            {
                return std::string(a) + ch + std::string(b) + c;
            },
            "hello",
            ' ',
            "hello world"sv.substr(0, 5),
            std::string(" world")
        );
        EXPECT_EQ(result, "hello hello world");
    }
}

TEST(StringLike, Adaptor)
{
    using asbind20::string_like;
    using asbind20::util::string_like_to_string;

    static_assert(string_like<char*>);
    static_assert(string_like<char[]>);
    static_assert(string_like<const char*>);
    static_assert(string_like<const char[]>);
    static_assert(string_like<std::string_view>);

    EXPECT_EQ(string_like_to_string(""), "");
    EXPECT_EQ(string_like_to_string("test"), "test");

    {
        std::string_view sv = "view";
        EXPECT_EQ(string_like_to_string(sv), "view")
            << "sv = " << sv;
    }
}

#ifdef ASBIND20_HAS_LIB_FORMAT

TEST(SetException, FormatException)
{
    using namespace asbind20;

    SCOPED_TRACE(std::format("__cpp_lib_format={}L", __cpp_lib_format));

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    global<true>(engine)
        .function(
            "void throw(int v)",
            [](int v)
            {
                format_script_exception("Value:{}", v);
            }
        );

    auto* m = asbind20::create_module(engine, "test");
    m->AddScriptSection(
        "test",
        "void f() { throw(1013); }"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("f");
    ASSERT_NE(f, nullptr);

    request_context ctx(engine);
    auto result = script_invoke<void>(ctx, f);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);

    EXPECT_STREQ(ctx->GetExceptionString(), "Value:1013");
}

#endif
