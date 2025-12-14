#include <vector>
#include <ranges>
#include <asbind_test/framework.hpp>
#include <asbind20/ranges/ranges.hpp>
#include <gmock/gmock-matchers.h>

namespace test_utility
{
void setup_big_interface(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    asbind20::interface bi(engine, "bi");

    for(const char ch : {'A', 'B', 'C'})
    {
        bi.method("void " + std::string(3, ch) + "()");
    }
}
} // namespace test_utility

TEST(Ranges, AllMethods)
{
    namespace abv = asbind20::views;

    static_assert(std::bidirectional_iterator<abv::all_methods::iterator>);
    static_assert(std::ranges::input_range<abv::all_methods>);

    auto engine = asbind20::make_script_engine();
    test_utility::setup_big_interface(engine);

    auto* ti = engine->GetTypeInfoByName("bi");
    ASSERT_NE(ti, nullptr);

    auto v =
        abv::all_methods(ti) |
        std::views::transform(
            [](AS_NAMESPACE_QUALIFIER asIScriptFunction* f) -> std::string
            { return f->GetDeclaration(false); }
        );

    EXPECT_THAT(
        std::vector(v.begin(), v.end()),
        ::testing::ElementsAre("void AAA()", "void BBB()", "void CCC()")
    );
}
