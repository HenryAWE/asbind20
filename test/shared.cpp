#include "shared.hpp"

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
}

void asbind_test_suite::TearDown()
{
    m_engine->ShutDownAndRelease();
    m_engine = nullptr;
}
} // namespace asbind_test
