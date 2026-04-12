#include <asbind_test/assertion.hpp>
#include <asbind_test/framework.hpp>
#include <sstream>

namespace asbind_test
{
std::function<void(std::string_view)> global_assertion_callback{};

void default_assertion_callback(std::string_view msg)
{
    FAIL() << msg;
}

std::string gen_full_assert_msg(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, std::string_view msg
)
{
    std::stringstream ss;

    if(ctx)
    {
        int column;
        const char* section = nullptr;
        int line = ctx->GetLineNumber(0, &column, &section);

        ss << line << ':' << column;
        ss << " at " << (section ? section : "(null)");

        if(auto* f = ctx->GetFunction(0); f)
            ss << " (fn:" << f->GetName() << ')';
    }
    else
        ss << "(no ctx)";

    ss << ": " << msg;

    return std::move(ss).str();
}

static void call_assertion_callback(std::string_view full_msg)
{
    if(global_assertion_callback)
        global_assertion_callback(full_msg);
    else
        default_assertion_callback(full_msg);
}

static void assert_impl(bool cond)
{
    if(cond)
        return;

    auto* ctx = asbind20::current_context();
    std::string msg = gen_full_assert_msg(ctx, "assertion failed");
    ctx->SetException(msg.c_str(), false);
    call_assertion_callback(msg);
}

void setup_script_assertion(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
)
{
    using namespace asbind20;

    // Force generic for max compatibility
    global<true>(engine)
        .function("void assert(bool cond)", fp<&assert_impl>);
}
} // namespace asbind_test
