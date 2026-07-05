#include <version>
#include <vector>
#include <asbind_test/framework.hpp>
#include <asbind20/ranges/ranges.hpp>
#include <gmock/gmock-matchers.h>

#ifdef ASBIND20_HAS_LIB_RANGES

namespace test_utility
{
static void setup_abc_interface(asbind20::engine_pointer engine)
{
    asbind20::interface bi(engine, "abc");

    for(const char ch : {'A', 'B', 'C'})
    {
        bi.method("void " + std::string(3, ch) + "()");
    }
}
} // namespace test_utility

TEST(Ranges, AllMethodsWithStdViews)
{
    namespace abr = asbind20::ranges;
    namespace abv = asbind20::views;

    static_assert(std::random_access_iterator<abr::all_methods_view::iterator>);
    static_assert(std::ranges::input_range<abr::all_methods_view>);

    auto engine = asbind20::make_script_engine();
    test_utility::setup_abc_interface(engine);

    auto* ti = engine->GetTypeInfoByName("abc");
    ASSERT_NE(ti, nullptr);

    // Check empty view
    {
        auto empty_v = abv::all_methods(nullptr);
        EXPECT_FALSE(empty_v);
        EXPECT_EQ(std::ranges::size(empty_v), 0);
        EXPECT_EQ(empty_v.begin(), empty_v.end());
    }

    auto v =
        abv::all_methods(ti) |
        std::views::transform(
            [](asbind20::function_pointer f) -> std::string
            { return f->GetDeclaration(false); }
        );
    EXPECT_EQ(std::ranges::size(v), ti->GetMethodCount());
    EXPECT_EQ(v.end() - v.begin(), ti->GetMethodCount());
    EXPECT_EQ(v.begin() + ti->GetMethodCount(), v.end());
    EXPECT_LT(v.begin(), v.end());
    EXPECT_FALSE(std::ranges::empty(v));
    EXPECT_TRUE(v);
    using v_t = decltype(v);
    static_assert(std::ranges::sized_range<v_t>);

    EXPECT_THAT(
        std::vector(v.begin(), v.end()),
        ::testing::ElementsAre("void AAA()", "void BBB()", "void CCC()")
    );
}

TEST(Ranges, AllBehavioursWithStdView)
{
    namespace abv = asbind20::views;

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    auto* m = engine->GetModule(
        "foo", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "foo",
        "class foo{};"
    );
    ASSERT_GE(m->Build(), 0);

    auto* ti = m->GetTypeInfoByName("foo");
    ASSERT_NE(ti, nullptr);

    // Check empty view
    {
        auto empty_v = abv::all_behaviours(nullptr);
        EXPECT_FALSE(empty_v);
        EXPECT_EQ(std::ranges::size(empty_v), 0);
        EXPECT_EQ(empty_v.begin(), empty_v.end());
    }

    auto v = abv::all_behaviours(ti);
    static_assert(std::ranges::sized_range<decltype(v)>);
    EXPECT_EQ(std::ranges::size(v), ti->GetBehaviourCount());
    EXPECT_EQ(v.end() - v.begin(), ti->GetBehaviourCount());
    EXPECT_EQ(v.begin() + ti->GetBehaviourCount(), v.end());
    EXPECT_LT(v.begin(), v.end());
    EXPECT_FALSE(std::ranges::empty(v));
    EXPECT_TRUE(v);

    auto beh_view = v | std::views::keys;
    EXPECT_THAT(
        std::vector(beh_view.begin(), beh_view.end()),
        ::testing::Contains(AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT)
    );

    auto fn_decl_view =
        v |
        std::views::values |
        std::views::transform(
            [](asbind20::function_pointer f) -> std::string
            { return f->GetDeclaration(false); }
        );
    EXPECT_THAT(
        std::vector(fn_decl_view.begin(), fn_decl_view.end()),
        ::testing::Contains("foo()") // The default constructor
    );
}

TEST(Ranges, AllFactoriesWithStdView)
{
    namespace abv = asbind20::views;

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    auto* m = engine->GetModule(
        "foo", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "foo",
        "class foo{};"
    );
    ASSERT_GE(m->Build(), 0);

    auto* ti = m->GetTypeInfoByName("foo");
    ASSERT_NE(ti, nullptr);

    // Check empty view
    {
        auto empty_v = abv::all_factories(nullptr);
        EXPECT_FALSE(empty_v);
        EXPECT_EQ(std::ranges::size(empty_v), 0);
        EXPECT_EQ(empty_v.begin(), empty_v.end());
    }

    auto v = abv::all_factories(ti);
    static_assert(std::ranges::sized_range<decltype(v)>);
    EXPECT_EQ(std::ranges::size(v), ti->GetFactoryCount());
    EXPECT_EQ(v.end() - v.begin(), ti->GetFactoryCount());
    EXPECT_EQ(v.begin() + ti->GetFactoryCount(), v.end());
    EXPECT_LT(v.begin(), v.end());
    EXPECT_FALSE(std::ranges::empty(v));
    EXPECT_TRUE(v);

    EXPECT_TRUE(
        std::ranges::all_of(
            v,
            [](asbind20::function_pointer f)
            { return f != nullptr; }
        )
    );
}

TEST(Ranges, AllEnumValuesWithStdView)
{
    namespace abv = asbind20::views;

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    auto* m = engine->GetModule(
        "foo", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "foo",
        "enum E{ A = 0, B = 1, C = 0 };"
    );
    ASSERT_GE(m->Build(), 0);

    auto* ti = m->GetTypeInfoByName("E");
    ASSERT_NE(ti, nullptr);

    // Check empty view
    {
        auto empty_v = abv::all_enum_values(nullptr);
        EXPECT_FALSE(empty_v);
        EXPECT_EQ(std::ranges::size(empty_v), 0);
        EXPECT_EQ(empty_v.begin(), empty_v.end());
    }

    auto v = abv::all_enum_values(ti);
    EXPECT_EQ(v.get_type_info(), ti);
    static_assert(std::ranges::sized_range<decltype(v)>);
    EXPECT_EQ(std::ranges::size(v), ti->GetEnumValueCount());
    EXPECT_EQ(v.end() - v.begin(), ti->GetEnumValueCount());
    EXPECT_EQ(v.begin() + ti->GetEnumValueCount(), v.end());
    EXPECT_LT(v.begin(), v.end());
    EXPECT_TRUE(v);

    auto enum_names =
        v |
        std::views::keys;
    EXPECT_THAT(
        std::vector<std::string>(enum_names.begin(), enum_names.end()),
        ::testing::ElementsAre("A", "B", "C")
    );
    auto enum_values =
        v |
        std::views::values;
    EXPECT_THAT(
        std::vector(enum_values.begin(), enum_values.end()),
        ::testing::ElementsAre(0, 1, 0)
    );
}

#    ifdef ASBIND20_HAS_ENUM_UNDERLYING_TYPE

TEST(Ranges, AllEnumValuesCustomUnderlying)
{
    namespace abv = asbind20::views;

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    enum my_enum : std::int64_t
    {
        a = std::numeric_limits<std::int64_t>::min(),
        b = std::numeric_limits<std::int64_t>::max()
    };

    asbind20::enum_underlying<my_enum>(engine, "my_enum")
        .value(my_enum::a, "a")
        .value(my_enum::b, "b");

    auto* ti = engine->GetTypeInfoByName("my_enum");
    ASSERT_NE(ti, nullptr);

    // Check empty view
    {
        auto empty_v = abv::all_enum_values(nullptr);
        EXPECT_FALSE(empty_v);
        EXPECT_EQ(std::ranges::size(empty_v), 0);
        EXPECT_EQ(empty_v.begin(), empty_v.end());
    }

    auto v = abv::all_enum_values_of<std::int64_t>(ti);
    EXPECT_EQ(v.get_type_info(), ti);
    static_assert(std::ranges::sized_range<decltype(v)>);
    EXPECT_EQ(std::ranges::size(v), ti->GetEnumValueCount());
    EXPECT_EQ(v.end() - v.begin(), ti->GetEnumValueCount());
    EXPECT_EQ(v.begin() + ti->GetEnumValueCount(), v.end());
    EXPECT_LT(v.begin(), v.end());
    EXPECT_TRUE(v);

    auto enum_names =
        v |
        std::views::keys;
    EXPECT_THAT(
        std::vector<std::string>(enum_names.begin(), enum_names.end()),
        ::testing::ElementsAre("a", "b")
    );
    auto enum_values =
        v |
        std::views::values;
    static_assert(std::same_as<std::int64_t, std::ranges::range_value_t<decltype(enum_values)>>);
    EXPECT_THAT(
        std::vector(enum_values.begin(), enum_values.end()),
        ::testing::ElementsAre(
            std::numeric_limits<std::int64_t>::min(),
            std::numeric_limits<std::int64_t>::max()
        )
    );
}

#    endif

TEST(Ranges, Tokenize)
{
    namespace abv = asbind20::views;
    using namespace std::string_view_literals;

    {
        auto guard_null =
            "class"sv |
            abv::tokenize(nullptr);
        EXPECT_FALSE(guard_null);
        EXPECT_FALSE(guard_null.begin());
        EXPECT_EQ(guard_null.begin(), guard_null.end());
    }

    auto engine = asbind20::make_script_engine();

    auto v =
        "class foo{};"sv |
        abv::tokenize(engine);
    EXPECT_EQ(v.get_engine(), engine.get());

    auto no_ws_view =
        v |
        std::views::filter(
            // Skip whitespaces
            [](const auto& token) -> bool
            { return token.first != AS_NAMESPACE_QUALIFIER asTC_WHITESPACE; }
        );

    std::vector<AS_NAMESPACE_QUALIFIER asETokenClass> tcs;
    std::vector<std::string> tokens;
    for(auto&& [tc, token] : no_ws_view)
    {
        tcs.push_back(tc);
        tokens.emplace_back(token);
    }

    EXPECT_EQ(tokens.size(), tcs.size());
    EXPECT_THAT(
        tcs,
        testing::ElementsAre(
            AS_NAMESPACE_QUALIFIER asTC_KEYWORD,
            AS_NAMESPACE_QUALIFIER asTC_IDENTIFIER,
            AS_NAMESPACE_QUALIFIER asTC_KEYWORD,
            AS_NAMESPACE_QUALIFIER asTC_KEYWORD,
            AS_NAMESPACE_QUALIFIER asTC_KEYWORD
        )
    );
    EXPECT_THAT(
        tokens,
        testing::ElementsAre("class", "foo", "{", "}", ";")
    );
}

namespace test_utility
{
static std::string proc_args(asbind20::generic_pointer gen)
{
    namespace abv = asbind20::views;
    auto v = abv::all_generic_args(gen);

    EXPECT_EQ(std::ranges::size(v), gen->GetArgCount());

    bool first = true;
    std::string result;
    for(auto&& [type_id, addr] : v)
    {
        if(!first)
            result += ", ";
        else
            first = false;


        if(asbind20::is_void_type(type_id))
        {
            result += "(void)";
            continue;
        }

        if(!asbind20::is_primitive_type(type_id))
        {
            result += '(';
            auto* ti = gen->GetEngine()->GetTypeInfoById(type_id);
            if(ti)
                result += ti->GetName();
            else
                result += "unknown";
            result += ')';
            continue;
        }

        if(asbind20::is_objhandle(type_id))
        {
            result += "(handle)";
            continue;
        }

        asbind20::visit_primitive_type(
            [&]<typename T>(const T* p)
            {
                if(!p)
                {
                    result += "null";
                    return;
                }

                std::stringstream ss;
                ss << *p;

                result += std::move(ss).str();
            },
            type_id,
            // asIScriptGeneric::GetAddressOfArg returns a pointer to the address of the argument,
            // so we need to dereference it to get the actual address of the argument.
            *static_cast<void**>(addr)
        );
    }

    return result;
}
} // namespace test_utility

TEST(Ranges, GenericArguments)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    asbind_test::setup_script_string(engine);

    generic_function wrapper = [](asbind20::generic_pointer gen) -> void
    {
        set_generic_return<std::string>(
            gen, test_utility::proc_args(gen)
        );
    };

    global<true>(engine)
        .function(
            "string proc_args(const?&in,const?&in)",
            wrapper
        )
        .function(
            "string proc_args(const?&in,const?&in,const?&in)",
            wrapper
        );

    auto* m = engine->GetModule(
        "test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "test",
        "string f2() { return proc_args(10, 13); }\n"
        "string f3() { return proc_args(1, 3.14, 'test'); }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* f2 = m->GetFunctionByDecl("string f2()");
        ASSERT_NE(f2, nullptr);

        request_context ctx(engine);
        auto result = asbind20::script_invoke<std::string>(ctx, f2);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), "10, 13");
    }

    {
        auto* f3 = m->GetFunctionByDecl("string f3()");
        ASSERT_NE(f3, nullptr);

        request_context ctx(engine);
        auto result = asbind20::script_invoke<std::string>(ctx, f3);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), "1, 3.14, (string)");
    }
}

#endif
