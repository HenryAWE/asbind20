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
