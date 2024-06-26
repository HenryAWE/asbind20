#include "shared.hpp"
#include <asbind20/ext/stdstring.hpp>
#include <asbind20/ext/unittest.hpp>

namespace asbind_test
{
static void msg_callback(const asSMessageInfo* msg, void*)
{
    if(msg->type == asEMsgType::asMSGTYPE_ERROR)
        FAIL() << msg->message;
}

void asbind_test_suite::SetUp()
{
    m_engine = asCreateScriptEngine();
    m_engine->SetMessageCallback(asFUNCTION(msg_callback), this, asCALL_CDECL);

    using namespace asbind20;

    ext::register_std_string(m_engine, true);
    ext::register_unittest(m_engine);
}

void asbind_test_suite::TearDown()
{
    m_engine->ShutDownAndRelease();
    m_engine = nullptr;
}
} // namespace asbind_test
