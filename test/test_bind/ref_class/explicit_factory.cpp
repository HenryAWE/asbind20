#include <asbind_test/framework.hpp>
#include <asbind20/ranges/typeinfo_views.hpp>

namespace
{
struct explict_factory
{
    explict_factory() = default;
    explict_factory(const explict_factory&) = default;

    explict_factory& operator=(const explict_factory&) = default;

    explicit explict_factory(int v)
        : data(v) {}

    int data;
};

void check_explicit_factory(asbind20::engine_pointer engine, int tid)
{
    auto* ti = engine->GetTypeInfoById(tid);
    ASSERT_THAT(ti, ::testing::NotNull())
        << "type ID = " << tid;

    std::size_t matched = 0;
    for(auto f : asbind20::ranges::views::all_factories(ti))
    {
        // Only check for unary constructor
        if(f->GetParamCount() != 1)
            continue;
        ++matched;

        EXPECT_TRUE(f->IsExplicit())
            << "id = " << f->GetId();
    }

    EXPECT_GT(matched, 0);
}
} // namespace

TEST(ExplicitFactory, Native)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    ref_class<explict_factory> c(
        engine, "explict_factory"
    );
    c
        .factory<int>("int v", use_explicit);

    check_explicit_factory(engine, c.get_type_id());
}

TEST(ExplicitFactory, Generic)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    ref_class<explict_factory, true> c(
        engine, "explict_factory"
    );
    c
        .factory<int>("int v", use_explicit);

    check_explicit_factory(engine, c.get_type_id());
}
