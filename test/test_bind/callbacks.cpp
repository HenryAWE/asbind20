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
        count();
    }

private:
    std::size_t m_counter = 0;
};

static void ASBIND20_STDCALL stdcall_msg_cb(AS_NAMESPACE_QUALIFIER asSMessageInfo* info, void* data)
{
    ASSERT_NE(data, nullptr);
    ASSERT_NE(info, nullptr);

    auto* counter = static_cast<log_counter*>(data);
    counter->count();
}

static void write_msg_helper(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, const char* msg)
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
