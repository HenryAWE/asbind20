#include <asbind20/utility.hpp>
#include <cassert>
#include <stdexcept>
#include <asbind20/invoke.hpp>

namespace asbind20
{
asUINT sizeof_script_type(asIScriptEngine* engine, int type_id)
{
    assert(engine != nullptr);

    if(is_primitive_type(type_id))
        return engine->GetSizeOfPrimitiveType(type_id);

    asITypeInfo* ti = engine->GetTypeInfoById(type_id);
    if(!ti)
        return 0;

    return ti->GetSize();
}

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

std::size_t copy_primitive_value(void* dst, const void* src, int type_id)
{
    assert(!(type_id & ~asTYPEID_MASK_SEQNBR));

    switch(type_id)
    {
    [[unlikely]] case asTYPEID_VOID:
        return 0;

    case asTYPEID_BOOL:
    case asTYPEID_INT8:
    case asTYPEID_UINT8:
        *(std::uint8_t*)dst = *(std::uint8_t*)src;
        return sizeof(std::uint8_t);

    case asTYPEID_INT16:
    case asTYPEID_UINT16:
        *(std::uint16_t*)dst = *(std::uint16_t*)src;
        return sizeof(std::uint16_t);

    default: // enums
    case asTYPEID_FLOAT:
    case asTYPEID_INT32:
    case asTYPEID_UINT32:
        *(std::uint32_t*)dst = *(std::uint32_t*)src;
        return sizeof(std::uint32_t);

    case asTYPEID_DOUBLE:
    case asTYPEID_INT64:
    case asTYPEID_UINT64:
        *(std::uint64_t*)dst = *(std::uint64_t*)src;
        return sizeof(std::uint64_t);
    }
}

asIScriptFunction* get_default_factory(asITypeInfo* ti)
{
    assert(ti != nullptr);
    assert(ti->GetFlags() & asOBJ_REF);

    for(asUINT i = 0; i < ti->GetFactoryCount(); ++i)
    {
        asIScriptFunction* func = ti->GetFactoryByIndex(i);
        if(func->GetParamCount() == 0)
            return func;
    }

    return nullptr;
}

asIScriptFunction* get_default_constructor(asITypeInfo* ti)
{
    assert((ti->GetFlags() & asOBJ_VALUE) && !(ti->GetFlags() & asOBJ_POD));
    for(asUINT i = 0; i < ti->GetBehaviourCount(); ++i)
    {
        asEBehaviours beh;
        asIScriptFunction* func = ti->GetBehaviourByIndex(i, &beh);
        if(beh == asBEHAVE_CONSTRUCT)
        {
            if(func->GetParamCount() == 0)
                return func;
        }
    }

    return nullptr;
}

int translate_three_way(std::weak_ordering ord) noexcept
{
    if(ord < 0)
        return -1;
    else if(ord > 0)
        return 1;
    else
        return 0;
}

std::strong_ordering translate_opCmp(int cmp) noexcept
{
    if(cmp < 0)
        return std::strong_ordering::less;
    else if(cmp > 0)
        return std::strong_ordering::greater;
    else
        return std::strong_ordering::equivalent;
}

void set_script_exception(asIScriptContext* ctx, const char* info)
{
    assert(ctx != nullptr);
    ctx->SetException(info);
}

void set_script_exception(asIScriptContext* ctx, const std::string& info)
{
    assert(ctx != nullptr);
    ctx->SetException(info.c_str());
}

void set_script_exception(const char* info)
{
    asIScriptContext* ctx = asGetActiveContext();
    if(ctx)
        set_script_exception(ctx, info);
}

void set_script_exception(const std::string& info)
{
    set_script_exception(info.c_str());
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
