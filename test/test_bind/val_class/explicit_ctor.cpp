#include <asbind_test/framework.hpp>
#include <asbind20/ranges/typeinfo_views.hpp>

namespace
{
struct explict_ctor
{
    explict_ctor() = default;
    explict_ctor(const explict_ctor&) = default;

    explict_ctor& operator=(const explict_ctor&) = default;

    explicit explict_ctor(int v)
        : data(v) {}

    int data;
};

void check_explicit_ctor(asbind20::engine_pointer engine, int tid)
{
    auto* ti = engine->GetTypeInfoById(tid);
    ASSERT_THAT(ti, ::testing::NotNull())
        << "type ID = " << tid;

    std::size_t matched = 0;
    for(auto [beh, f] : asbind20::ranges::views::all_behaviours(ti))
    {
        if(beh != AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT)
            return;

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

TEST(ExplicitCtor, Native)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    value_class<explict_ctor> c(
        engine,
        "explicit_ctor",
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS
    );
    c
        .behaviours_by_traits()
        .constructor<int>("int v", use_explicit);

    check_explicit_ctor(engine, c.get_type_id());
}

TEST(ExplicitCtor, Generic)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    value_class<explict_ctor, true> c(
        engine,
        "explicit_ctor",
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS
    );
    c
        .behaviours_by_traits()
        .constructor<int>("int v", use_explicit);

    check_explicit_ctor(engine, c.get_type_id());
}
