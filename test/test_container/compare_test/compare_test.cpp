#include <asbind_test/framework.hpp>
#include <asbind20/container/compare.hpp>

TEST(Compare, EmptyClass)
{
    using namespace asbind20;

    auto engine = make_script_engine();
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

    auto result = container::get_comparator(ti);
    {
        auto st = result.get_status();
        EXPECT_FALSE(st.good());
        EXPECT_FALSE(st);
        EXPECT_EQ(st.opCmp, AS_NAMESPACE_QUALIFIER asNO_FUNCTION);
        EXPECT_EQ(st.opEquals, AS_NAMESPACE_QUALIFIER asNO_FUNCTION);
    }
    EXPECT_FALSE(result);
}
