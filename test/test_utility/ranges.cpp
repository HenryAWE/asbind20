#include <version>
#include <vector>
#include <asbind_test/framework.hpp>
#include <asbind20/ranges/ranges.hpp>
#include <gmock/gmock-matchers.h>

#ifdef __cpp_lib_ranges
#    include <ranges>

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
    namespace abv = asbind20::views;

    static_assert(std::random_access_iterator<abv::all_methods::iterator>);
    static_assert(std::ranges::input_range<abv::all_methods>);

    auto engine = asbind20::make_script_engine();
    test_utility::setup_abc_interface(engine);

    auto* ti = engine->GetTypeInfoByName("abc");
    ASSERT_NE(ti, nullptr);

    auto v =
        abv::all_methods(ti) |
        std::views::transform(
            [](AS_NAMESPACE_QUALIFIER asIScriptFunction* f) -> std::string
            { return f->GetDeclaration(false); }
        );
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

    auto v = abv::all_behaviours(ti);
    EXPECT_EQ(std::ranges::size(v), ti->GetBehaviourCount());

    std::vector<AS_NAMESPACE_QUALIFIER asEBehaviours> bs;
    for(auto beh : v | std::views::values)
    {
        bs.push_back(beh);
    }

    EXPECT_THAT(
        bs,
        ::testing::Contains(AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT)
    );
}

#endif
