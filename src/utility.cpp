#include <asbind20/utility.hpp>
#include <cassert>
#include <stdexcept>
#include <asbind20/invoke.hpp>

namespace asbind20
{
std::string extract_string(asIStringFactory* factory, void* str)
{
    assert(factory);

    asUINT sz = 0;
    if(factory->GetRawStringData(str, nullptr, &sz) < 0)
    {
        throw std::runtime_error("failed to get raw string data");
    }

    std::string result;
    result.resize(sz);

    if(factory->GetRawStringData(str, result.data(), nullptr) < 0)
    {
        throw std::runtime_error("failed to get raw string data");
    }

    return result;
}

namespace detail
{
    int exec_impl(
        asIScriptEngine* engine,
        asIScriptContext* ctx,
        std::string_view code,
        const char* ret_decl = "void",
        const char* module_name = "asbind20_exec"
    )
    {
        int r = 0;

        std::string func_code;
        func_code = string_concat(
            ret_decl,
            ' ',
            module_name,
            "(){\n",
            code,
            "\n;}"
        );

        asIScriptModule* m = engine->GetModule(module_name, asGM_ALWAYS_CREATE);
        asIScriptFunction* f = nullptr;
        r = m->CompileFunction(module_name, func_code.c_str(), -1, 0, &f);
        if(r < 0)
            return r;

        assert(f);
        auto result = script_invoke<void>(ctx, f);
        f->Release();

        if(!result)
            return result.error();
        else
            return asSUCCESS;
    }
} // namespace detail

int exec(
    asIScriptEngine* engine,
    std::string_view code,
    asIScriptContext* ctx
)
{
    int r = 0;
    if(ctx)
    {
        r = detail::exec_impl(
            engine,
            ctx,
            code
        );
    }
    else
    {
        request_context exec_ctx(engine);
        r = detail::exec_impl(
            engine,
            exec_ctx,
            code
        );
    }

    return r;
}
} // namespace asbind20
