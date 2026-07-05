#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>
#include <gmock/gmock-matchers.h>
#include "listener_suites.hpp"

namespace test_listener
{
class record_aliases
{
public:
    template <typename BindGenerator>
    static void on_function(BindGenerator& g, int id)
    {
        GTEST_FAIL()
            << "Unreachable. name = " << g.get_name() << " id = " << id;
    }

    template <typename BindGenerator>
    static void on_global_property(BindGenerator& g, int id)
    {
        GTEST_FAIL()
            << "Unreachable. name = " << g.get_name() << " id = " << id;
    }

    template <typename BindGenerator>
    void on_typedef(BindGenerator& g, int id)
    {
        if(id < 0)
        {
            using asbind20::to_string;
            GTEST_FAIL()
                << "bad typedef: " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(id));
        }

        asbind20::engine_pointer engine = g.get_engine();
        ASSERT_NE(engine, nullptr);
        // The return value of RegisterTypedef is the ID of original decl
        if(!asbind20::is_primitive_type(id))
        {
            auto* ti = engine->GetTypeInfoById(id);
            ASSERT_NE(ti, nullptr) << "id = " << id;

            EXPECT_NE(ti->GetTypeId(), AS_NAMESPACE_QUALIFIER asTYPEID_VOID);
            recorded_typedef.emplace_back(ti->GetName());
        }
        else
        {
            const char* name = engine->GetTypeDeclaration(id);
            ASSERT_NE(nullptr, name) << "id = " << id;
            recorded_typedef.emplace_back(name);
        }
    }

    template <typename BindGenerator>
    void on_funcdef(BindGenerator& g, int id)
    {
        using asbind20::to_string;
        if(id < 0)
        {
            GTEST_FAIL()
                << "bad funcdef: " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(id));
        }

        asbind20::engine_pointer engine = g.get_engine();
        ASSERT_NE(engine, nullptr);
        auto* ti = engine->GetTypeInfoById(id);
        ASSERT_NE(ti, nullptr) << "id = " << id;

        recorded_funcdef.emplace_back(ti->GetName());
    }

    std::vector<std::string> recorded_typedef;
    std::vector<std::string> recorded_funcdef;
};
} // namespace test_listener

using ListenerTest = test_listener::general_listener_suite;

TEST_F(ListenerTest, RecordTypedefs)
{
    asbind20::global<true, test_listener::record_aliases> g(engine);
    auto& listener = g.get_listener();

    EXPECT_THAT(listener.recorded_typedef, ::testing::SizeIs(0));
    EXPECT_THAT(listener.recorded_funcdef, ::testing::SizeIs(0));

    g.typedef_("float", "flt32");
    EXPECT_THAT(listener.recorded_typedef, ::testing::SizeIs(1));
    EXPECT_THAT(listener.recorded_funcdef, ::testing::SizeIs(0));
    EXPECT_THAT(listener.recorded_typedef, ::testing::Contains("float"));

    g.using_("my_int", "int64");
    EXPECT_THAT(listener.recorded_typedef, ::testing::SizeIs(2));
    EXPECT_THAT(listener.recorded_funcdef, ::testing::SizeIs(0));
    EXPECT_THAT(listener.recorded_typedef, ::testing::Contains("int64"));
}

TEST_F(ListenerTest, RecordFuncdefs)
{
    asbind20::global<true, test_listener::record_aliases> g(engine);
    auto& listener = g.get_listener();

    EXPECT_THAT(listener.recorded_typedef, ::testing::SizeIs(0));
    EXPECT_THAT(listener.recorded_funcdef, ::testing::SizeIs(0));

    g.funcdef("int foo(float)");
    EXPECT_THAT(listener.recorded_typedef, ::testing::SizeIs(0));
    EXPECT_THAT(listener.recorded_funcdef, ::testing::SizeIs(1));
    EXPECT_THAT(listener.recorded_funcdef, ::testing::Contains("foo"));

    g.funcdef("float bar(int)");
    EXPECT_THAT(listener.recorded_typedef, ::testing::SizeIs(0));
    EXPECT_THAT(listener.recorded_funcdef, ::testing::SizeIs(2));
    EXPECT_THAT(listener.recorded_funcdef, ::testing::Contains("bar"));
}
