#include <asbind_test/framework.hpp>
#include <gmock/gmock.h>

namespace test_bind
{
class msg_callback_helper
{
public:
    struct mock_msg
    {
        MOCK_METHOD(void, on_message, (AS_NAMESPACE_QUALIFIER asSMessageInfo*), ());
    };

    using mock_type = ::testing::StrictMock<mock_msg>;
    std::shared_ptr<mock_type> mock = std::make_shared<mock_type>();

    // Stdcall (global) callback: passes data as void* to the helper
    static void ASBIND20_STDCALL stdcall_cb(
        AS_NAMESPACE_QUALIFIER asSMessageInfo* info, void* data
    )
    {
        auto* self = static_cast<msg_callback_helper*>(data);
        self->mock->on_message(info);
    }

    // Member callback bound via auxiliary()
    void mem_cb(AS_NAMESPACE_QUALIFIER asSMessageInfo* info)
    {
        mock->on_message(info);
    }
};

static void write_msg_helper(asbind20::engine_pointer engine, const char* msg)
{
    ASSERT_THAT(engine, ::testing::NotNull());
    engine->WriteMessage(
        "(system)",
        0,
        0,
        AS_NAMESPACE_QUALIFIER asMSGTYPE_INFORMATION,
        msg
    );
}
} // namespace test_bind

TEST(MessageCallback, Global)
{
    using ::testing::_;

    test_bind::msg_callback_helper helper;
    auto engine = asbind20::make_script_engine();

    asbind20::set_message_callback(
        engine, &test_bind::msg_callback_helper::stdcall_cb, &helper
    );
    EXPECT_CALL(*helper.mock, on_message(_)).Times(1);
    test_bind::write_msg_helper(engine, "msg");
}

TEST(MessageCallback, Member)
{
    using ::testing::_;

    test_bind::msg_callback_helper helper;
    auto engine = asbind20::make_script_engine();

    asbind20::set_message_callback(
        engine, &test_bind::msg_callback_helper::mem_cb, asbind20::auxiliary(helper)
    );
    EXPECT_CALL(*helper.mock, on_message(_)).Times(1);
    test_bind::write_msg_helper(engine, "msg");
}

#ifndef ASBIND20_NO_EXCEPTIONS

namespace test_bind
{
struct mock_ex
{
    MOCK_METHOD(void, on_exception, (asbind20::context_pointer), ());
};

class ex_translator_helper
{
public:
    using mock_type = ::testing::StrictMock<mock_ex>;
    std::shared_ptr<mock_type> mock = std::make_shared<mock_type>();

    class my_ex : public std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "what";
        }
    };

    void translate(asbind20::context_pointer ctx)
    {
        try
        {
            throw;
        }
        catch(const std::exception& e)
        {
            EXPECT_STREQ(e.what(), "what");
            ctx->SetException(e.what());
            mock->on_exception(ctx);
        }
        catch(...)
        {
            ctx->SetException("unreachable");
            FAIL() << "unreachable";
        }
    }
};

static void setup_funcs(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    global<true>(engine)
        .function(
            "void throw_my_ex()",
            []() -> void
            {
                throw ex_translator_helper::my_ex{};
            }
        );
}

static void trigger_ex_in_script(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "test_ex");
    ASSERT_THAT(m, ::testing::NotNull());
    m->AddScriptSection(
        "test_ex",
        "void f()\n"
        "{\n"
        "    throw_my_ex();"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("f");
    ASSERT_THAT(f, ::testing::NotNull());
    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<void>(ctx, f);
    ASBIND_TEST_EXPECT_INVOKE_NO_RESULT(result);
    EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);
}
} // namespace test_bind

TEST(ExceptionCallback, Member)
{
    using ::testing::_;

    auto engine = asbind20::make_script_engine();

    test_bind::ex_translator_helper helper;
    asbind20::set_exception_translator(
        engine, &test_bind::ex_translator_helper::translate, asbind20::auxiliary(helper)
    );

    EXPECT_CALL(*helper.mock, on_exception(_)).Times(1);
    test_bind::setup_funcs(engine);
    test_bind::trigger_ex_in_script(engine);
}

#endif
