#include <asbind20/ext/exec.hpp>
#include <fstream>
#include <sstream>

namespace asbind20::ext
{
int load_string(
    AS_NAMESPACE_QUALIFIER asIScriptModule* m,
    const char* section_name,
    std::string_view code,
    int line_offset
)
{
    assert(m != nullptr);

    return m->AddScriptSection(
        section_name,
        code.data(),
        code.size(),
        line_offset
    );
}

int load_file(
    AS_NAMESPACE_QUALIFIER asIScriptModule* m,
    const std::filesystem::path& filename,
    std::ios_base::openmode mode
)
{
    assert(m != nullptr);

    std::string code;
    {
        std::ifstream ifs(filename, std::ios_base::in | mode);
        if(!ifs.good())
            return AS_NAMESPACE_QUALIFIER asERROR;
        std::stringstream ss;
        ss << ifs.rdbuf();
        code = std::move(ss).str();
    }

    return load_string(
        m,
        reinterpret_cast<const char*>(filename.u8string().c_str()),
        code
    );
}

namespace detail
{
    int exec_impl(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
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

        auto* m = engine->GetModule(
            module_name, AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        AS_NAMESPACE_QUALIFIER asIScriptFunction* f = nullptr;
        r = m->CompileFunction(
            module_name,
            func_code.c_str(),
            -1,
            0,
            &f
        );
        if(r < 0)
            return r;

        assert(f);
        try
        {
            auto result = script_invoke<void>(ctx, f);
            f->Release();
            f = nullptr;

            if(!result)
                return result.error();
            else
                return AS_NAMESPACE_QUALIFIER asSUCCESS;
        }
        catch(...)
        {
            if(f)
                f->Release();
            throw;
        }
    }
} // namespace detail

int exec(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    std::string_view code,
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx
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
} // namespace asbind20::ext
