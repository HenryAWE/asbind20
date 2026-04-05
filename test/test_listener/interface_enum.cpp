#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>
#include <gmock/gmock-matchers.h>
#include "listener_suites.hpp"

namespace test_listener
{
class record_type
{
public:
    template <typename BindGenerator>
    void on_enum_value(BindGenerator& g, int r)
    {
        SCOPED_TRACE(std::string("Type name: ") + g.get_name());

        EXPECT_NE(g.get_engine(), nullptr);

        using asbind20::to_string;
        EXPECT_GE(r, 0)
            << "r = " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r));

        // The return value of RegisterEnumValue only reports whether it was successful
        ++enum_value_count;
    }

    template <typename BindGenerator>
    void on_enum(BindGenerator& g, int r)
    {
        SCOPED_TRACE(std::string("Type name: ") + g.get_name());

        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = g.get_engine();
        ASSERT_NE(engine, nullptr);

        if(r < 0)
        {
            using asbind20::to_string;
            GTEST_FAIL()
                << "bad enum: " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r));
        }

        AS_NAMESPACE_QUALIFIER asITypeInfo* ti = engine->GetTypeInfoById(r);
        ASSERT_NE(ti, nullptr)
            << "id = " << r;

        rec_type("enum", ti->GetName());
    }

    template <typename BindGenerator>
    void on_interface(BindGenerator& g, int r)
    {
        SCOPED_TRACE(std::string("Type name: ") + g.get_name());

        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = g.get_engine();
        ASSERT_NE(engine, nullptr);

        if(r < 0)
        {
            using asbind20::to_string;
            GTEST_FAIL()
                << "bad interface: " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r));
        }

        AS_NAMESPACE_QUALIFIER asITypeInfo* ti = engine->GetTypeInfoById(r);
        ASSERT_NE(ti, nullptr)
            << "id = " << r;

        rec_type("interface", ti->GetName());
    }

    std::vector<std::string> recorded_type;
    std::size_t enum_value_count = 0;

private:
    void rec_type(std::string_view type_class, const char* name)
    {
        ASSERT_NE(name, nullptr);
        recorded_type.emplace_back(asbind20::string_concat(type_class, ' ', name));
    }
};

enum my_enum
{
    a,
    b
};
} // namespace test_listener

using ListenerTest = test_listener::general_listener_suite;

TEST_F(ListenerTest, RecordFInterfaceAndEnum)
{
    using namespace asbind20;
    using namespace test_listener;

    {
        asbind20::basic_interface<record_type> i(engine, "interface_0");
        auto& listener = i.get_listener();
        EXPECT_THAT(listener.recorded_type, ::testing::Contains("interface interface_0"));
    }

    {
        asbind20::enum_<my_enum, compat::script_enum_value_type, record_type> e(engine, "my_enum");
        auto& listener = e.get_listener();
        EXPECT_THAT(listener.recorded_type, ::testing::Contains("enum my_enum"));

        EXPECT_EQ(listener.enum_value_count, 0);
        e.value(my_enum::a, "a");
        EXPECT_EQ(listener.enum_value_count, 1);
        e.value(my_enum::b, "b");
        EXPECT_EQ(listener.enum_value_count, 2);
    }
}
