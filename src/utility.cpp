#include <asbind20/utility.hpp>
#include <cassert>

namespace asbind20
{
asIScriptFunction* get_default_factory(asITypeInfo* ti)
{
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

void set_script_exception(asIScriptContext* ctx, const char* info)
{
    ctx->SetException(info);
}

void set_script_exception(asIScriptContext* ctx, const std::string& info)
{
    ctx->SetException(info.c_str());
}

void set_script_exception(const char* info)
{
    asIScriptContext* ctx = asGetActiveContext();
    assert(ctx != nullptr);
    set_script_exception(ctx, info);
}

void set_script_exception(const std::string& info)
{
    set_script_exception(info.c_str());
}
} // namespace asbind20
