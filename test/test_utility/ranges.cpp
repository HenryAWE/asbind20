#include <version>
#include <vector>
#include <asbind_test/framework.hpp>
#include <asbind20/ranges/ranges.hpp>
#include <gmock/gmock-matchers.h>

#ifdef ASBIND20_HAS_LIB_RANGES

namespace test_utility
{
void setup_abc_interface(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
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
            [](AS_NAMESPACE_QUALIFIER asIScriptFunction* f) -> std::string
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
            [](AS_NAMESPACE_QUALIFIER asIScriptFunction* f) -> std::string
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
            [](AS_NAMESPACE_QUALIFIER asIScriptFunction* f)
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

TEST(Ranges, Tokenize)
{
    namespace abv = asbind20::views;
    using namespace std::string_view_literals;

    auto engine = asbind20::make_script_engine();

    auto v =
        "class foo{};"sv |
        abv::tokenize(engine) |
        std::views::filter(
            // Skip whitespaces
            [](const auto& token) -> bool
            { return token.first != AS_NAMESPACE_QUALIFIER asTC_WHITESPACE; }
        );

    std::vector<AS_NAMESPACE_QUALIFIER asETokenClass> tcs;
    std::vector<std::string> tokens;
    for(auto&& [tc, token] : v)
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

#endif
