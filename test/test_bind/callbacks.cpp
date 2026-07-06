#include <asbind_test/framework.hpp>

namespace test_bind
{
class log_counter
{
public:
    void count()
    {
        ++m_counter;
    }

    std::size_t get() const noexcept
    {
        return m_counter;
    }

    void mem_cb(AS_NAMESPACE_QUALIFIER asSMessageInfo* info)
    {
        ASSERT_NE(info, nullptr);
        EXPECT_STREQ(info->message, "msg");
        count();
    }

private:
    std::size_t m_counter = 0;
};

static void ASBIND20_STDCALL stdcall_msg_cb(AS_NAMESPACE_QUALIFIER asSMessageInfo* info, void* data)
{
    ASSERT_NE(data, nullptr);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->message, "msg");

    auto* counter = static_cast<log_counter*>(data);
    counter->count();
}

static void write_msg_helper(asbind20::engine_pointer engine, const char* msg)
{
    ASSERT_NE(engine, nullptr);
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
    test_bind::log_counter counter;
    auto engine = asbind20::make_script_engine();

    asbind20::set_message_callback(
        engine, &test_bind::stdcall_msg_cb, &counter
    );
    EXPECT_EQ(counter.get(), 0);
    test_bind::write_msg_helper(engine, "msg");
    EXPECT_EQ(counter.get(), 1);
}

TEST(MessageCallback, Member)
{
    test_bind::log_counter counter;
    auto engine = asbind20::make_script_engine();

    asbind20::set_message_callback(
        engine, &test_bind::log_counter::mem_cb, asbind20::auxiliary(counter)
    );
    EXPECT_EQ(counter.get(), 0);
    test_bind::write_msg_helper(engine, "msg");
    EXPECT_EQ(counter.get(), 1);
}

#ifndef ASBIND20_NO_EXCEPTIONS

namespace test_bind
{
class ex_counter
{
public:
    class my_ex : public std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "what";
        }
    };

    void count()
    {
        ++m_counter;
    }

    std::size_t get() const noexcept
    {
        return m_counter;
    }

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
            count();
        }
        catch(...)
        {
            ctx->SetException("...");
            FAIL() << "unreachable";
        }
    }

private:
    std::size_t m_counter = 0;
};

static void setup_funcs(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    global<true>(engine)
        .function(
            "void throw_my_ex()",
            []() -> void
            {
                throw ex_counter::my_ex{};
            }
        );
}

static void trigger_ex_in_script(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "test_ex");
    ASSERT_NE(m, nullptr);
    m->AddScriptSection(
        "test_ex",
        "void f()\n"
        "{\n"
        "    throw_my_ex();"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("f");
    ASSERT_NE(f, nullptr);
    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<void>(ctx, f);
    ASBIND_TEST_EXPECT_INVOKE_NO_RESULT(result);
    EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);
}
} // namespace test_bind

TEST(ExceptionCallback, Member)
{
    auto engine = asbind20::make_script_engine();

    test_bind::ex_counter counter;
    asbind20::set_exception_translator(
        engine, &test_bind::ex_counter::translate, asbind20::auxiliary(counter)
    );

    EXPECT_EQ(counter.get(), 0);
    test_bind::setup_funcs(engine);
    test_bind::trigger_ex_in_script(engine);
    EXPECT_EQ(counter.get(), 1);
}

#endif
