#include "shared.hpp"
#include <asbind20/ext/stdstring.hpp>
#include <asbind20/ext/unittest.hpp>
#include <asbind20/ext/assert.hpp>
#include <asbind20/ext/math.hpp>

namespace asbind_test
{
static void msg_callback(const asSMessageInfo* msg, void*)
{
    if(msg->type == asEMsgType::asMSGTYPE_ERROR)
        FAIL() << msg->message;
}

static void assert_callback(std::string_view sv)
{
    asIScriptContext* ctx = asGetActiveContext();

    const char* section;
    int line = ctx->GetLineNumber(0, nullptr, &section);

    GTEST_FAIL_AT(section, line)
        << "Script assert() failed: " << sv;
}

void asbind_test_suite::SetUp()
{
    m_engine = asCreateScriptEngine();
    m_engine->SetMessageCallback(asFUNCTION(msg_callback), this, asCALL_CDECL);

    using namespace asbind20;

    ext::register_std_string(m_engine, true);
    ext::register_math_function(m_engine);
    ext::register_unittest(m_engine);
    ext::register_script_assert(
        m_engine,
        &assert_callback,
        false, // The exception will be set by GTEST_FAIL_AT() in the callback
        &ext::string_factory::get()
    );
}

void asbind_test_suite::TearDown()
{
    m_engine->ShutDownAndRelease();
    m_engine = nullptr;
}
} // namespace asbind_test
