#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>
#include <gmock/gmock.h>
#include "listener_suites.hpp"

namespace test_listener
{

// gmock mock for enum value listener verification
struct mock_type_listener
{
    MOCK_METHOD(void, on_enum_value, (), ());
};

// Listener adapter — bridges binding generator callbacks to gmock.
// Uses NiceMock because on_enum and on_interface fire during construction.
// Constructor callbacks are recorded into recorded_type for post-check.
struct record_type
{
    std::shared_ptr<testing::NiceMock<mock_type_listener>> mock =
        std::make_shared<testing::NiceMock<mock_type_listener>>();
    std::vector<std::string> recorded_type;

    template <typename BindGenerator>
    void on_enum_value(BindGenerator& g, int r)
    {
        SCOPED_TRACE(std::string("Type name: ") + g.get_name());

        EXPECT_NE(g.get_engine(), nullptr);

        using asbind20::to_string;
        EXPECT_GE(r, 0)
            << "r = " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r));

        mock->on_enum_value();
    }

    template <typename BindGenerator>
    void on_enum(BindGenerator& g, int r)
    {
        SCOPED_TRACE(std::string("Type name: ") + g.get_name());

        asbind20::engine_pointer engine = g.get_engine();
        ASSERT_NE(engine, nullptr);

        if(r < 0)
        {
            using asbind20::to_string;
            GTEST_FAIL()
                << "bad enum: " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r));
        }

        asbind20::typeinfo_pointer ti = engine->GetTypeInfoById(r);
        ASSERT_NE(ti, nullptr)
            << "id = " << r;

        rec_type("enum", ti->GetName());
    }

    template <typename BindGenerator>
    void on_interface(BindGenerator& g, int r)
    {
        SCOPED_TRACE(std::string("Type name: ") + g.get_name());

        asbind20::engine_pointer engine = g.get_engine();
        ASSERT_NE(engine, nullptr);

        if(r < 0)
        {
            using asbind20::to_string;
            GTEST_FAIL()
                << "bad interface: " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r));
        }

        asbind20::typeinfo_pointer ti = engine->GetTypeInfoById(r);
        ASSERT_NE(ti, nullptr)
            << "id = " << r;

        rec_type("interface", ti->GetName());
    }

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

        EXPECT_CALL(*listener.mock, on_enum_value()).Times(2);

        e.value(my_enum::a, "a");
        e.value(my_enum::b, "b");
    }
}
