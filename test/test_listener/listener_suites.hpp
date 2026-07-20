#pragma once

#include <asbind_test/framework.hpp>
#include <gmock/gmock.h>

namespace test_listener
{
class general_listener_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        engine = asbind20::make_script_engine();
        ASSERT_TRUE(engine);

        // Error will be reported by listeners
        asbind_test::setup_message_callback(engine, false);
    }

    void TearDown() override
    {
        engine.reset();
    }

    asbind20::script_engine engine;
};

// Recording registered global entities
struct record_global
{
    struct mock_global_listener
    {
        MOCK_METHOD(void, on_function, (std::string_view name), ());
        MOCK_METHOD(void, on_global_property, (std::string_view name), ());
        MOCK_METHOD(void, on_typedef, (std::string_view name), ());
        MOCK_METHOD(void, on_funcdef, (std::string_view name), ());
    };

    using mock_type = ::testing::StrictMock<mock_global_listener>;

    std::shared_ptr<mock_type> mock;

    record_global();

    template <typename BindGenerator>
    void on_function(BindGenerator& g, int id)
    {
        if(id < 0)
        {
            using asbind20::to_string;
            GTEST_FAIL()
                << "bad function: "
                << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(id));
        }

        asbind20::engine_pointer engine = g.get_engine();
        ASSERT_NE(engine, nullptr);
        auto* f = engine->GetFunctionById(id);
        ASSERT_NE(f, nullptr);

        mock->on_function(f->GetName());
    }

    template <typename BindGenerator>
    void on_global_property(BindGenerator& g, int id)
    {
        using asbind20::to_string;
        if(id < 0)
        {
            GTEST_FAIL()
                << "bad global property: "
                << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(id));
        }

        asbind20::engine_pointer engine = g.get_engine();
        ASSERT_NE(engine, nullptr);
        const char* name;
        int r = engine->GetGlobalPropertyByIndex(static_cast<AS_NAMESPACE_QUALIFIER asUINT>(id), &name);
        ASSERT_GE(r, 0)
            << "r = " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r));
        ASSERT_NE(name, nullptr);

        mock->on_global_property(name);
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
            mock->on_typedef(ti->GetName());
        }
        else
        {
            const char* name = engine->GetTypeDeclaration(id);
            ASSERT_NE(nullptr, name) << "id = " << id;
            mock->on_typedef(name);
        }
    }

    template <typename BindGenerator>
    void on_funcdef(BindGenerator& g, int id)
    {
        using asbind20::to_string;
        if(id < 0)
        {
            GTEST_FAIL()
                << "bad funcdef: "
                << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(id));
        }

        asbind20::engine_pointer engine = g.get_engine();
        ASSERT_NE(engine, nullptr);
        auto* ti = engine->GetTypeInfoById(id);
        ASSERT_NE(ti, nullptr) << "id = " << id;

        mock->on_funcdef(ti->GetName());
    }
};
} // namespace test_listener
