#include <asbind20/ext/unittest.hpp>
#include <cstring>
#include <asbind20/ext/helper.hpp>

namespace asbind20::ext
{
asIScriptFunction* find_opEquals(asITypeInfo* ti)
{
    assert(ti);

    for(asUINT i = 0; i < ti->GetMethodCount(); ++i)
    {
        asIScriptFunction* f = ti->GetMethodByIndex(i);

        asDWORD return_t_flags;
        int return_t_id = f->GetReturnTypeId(&return_t_flags);
        std::string_view name = f->GetName();
        if(
            name == "opEquals" &&
            return_t_id == asTYPEID_BOOL &&
            return_t_flags == asTM_NONE &&
            f->GetParamCount() == 1
        )
        {
            int param_t_id;
            asDWORD param_t_flags;
            f->GetParam(0, &param_t_id, &param_t_flags);

            // According to the official add-on:
            // The parameter must be an input reference of the same type
            // If the reference is a inout reference, then it must also be read-only
            bool valid =
                (param_t_flags & asTM_INREF) ||
                param_t_id != ti->GetTypeId() ||
                ((param_t_flags & asTM_OUTREF) && !(param_t_flags & asTM_CONST));

            if(valid)
                return f;
        }
    }

    return nullptr;
}

static bool is_primitive_type_id(int tid)
{
    return tid >= asTYPEID_VOID && tid <= asTYPEID_DOUBLE;
}

bool expect_eq(void* lhs_ref, int lhs_t_id, void* rhs_ref, int rhs_t_id)
{
    asIScriptContext* current_ctx = asGetActiveContext();
    asIScriptEngine* engine = current_ctx->GetEngine();
    if(lhs_t_id != rhs_t_id)
    {
        std::string info =
            "[expect_eq] Different type " +
            std::to_string(lhs_t_id) +
            " and " +
            std::to_string(rhs_t_id);
        asGetActiveContext()->SetException(info.c_str());
        return false;
    }

    bool is_eq = false;

    if(is_primitive_type_id(lhs_t_id))
    {
        asUINT size = engine->GetSizeOfPrimitiveType(lhs_t_id);
        is_eq = memcmp(lhs_ref, rhs_ref, size) == 0;
    }
    else
    {
        asITypeInfo* ti = engine->GetTypeInfoById(lhs_t_id);
        if(!ti)
        {
            std::string info = "[expect_eq] Bad type id " + std::to_string(lhs_t_id);
            current_ctx->SetException(info.c_str());
            return false;
        }
        asIScriptFunction* op = find_opEquals(ti);
        if(!op)
        {
            current_ctx->SetException("[expect_eq] No suitable opEquals method");
            return false;
        }
        asIScriptContext* ctx = engine->CreateContext();
        auto result = script_invoke<bool>(
            ctx,
            static_cast<asIScriptObject*>(lhs_ref),
            op,
            static_cast<asIScriptObject*>(rhs_ref)
        );
        ctx->Release();
        ctx = nullptr;
        if(!result.has_value())
        {
            current_ctx->SetException("[expect_eq] Bad function call");
            return false;
        }

        is_eq = *result;
    }

    if(!is_eq)
    {
        const char* section;
        int line = current_ctx->GetLineNumber(0, nullptr, &section);
        std::string info = "[expect_eq] Unexpected result (";
        info += section;
        info += " : " + std::to_string(line) + ')';

        current_ctx->SetException(info.c_str());
        return false;
    }

    return true;
}

template <typename Fn>
bool expect_cmp_impl(
    Fn&& pred,
    const char* expected_name,
    void* lhs_ref,
    int lhs_t_id,
    void* rhs_ref,
    int rhs_t_id,
    const char* func_name
)
{
    auto result = script_compare(
        asGetActiveContext()->GetEngine(),
        lhs_ref,
        lhs_t_id,
        rhs_ref,
        rhs_t_id
    );

    if(result == std::partial_ordering::unordered)
    {
        std::string info = "[";
        info += func_name;
        info += "] Cannot compare two values";
        asGetActiveContext()->SetException(info.c_str());
        return false;
    }

    auto relation_name = [](std::weak_ordering r) -> const char*
    {
        if(r == std::weak_ordering::less)
            return "less";
        else if(r == std::weak_ordering::greater)
            return "greater";
        else
            return "equal";
    };

    std::weak_ordering r =
        result == std::partial_ordering::greater ? std::weak_ordering::greater :
        result == std::partial_ordering::less    ? std::weak_ordering::less :
                                                   std::weak_ordering::equivalent;
    if(pred(r))
    {
        return true;
    }
    else
    {
        std::string info = "[";
        info += func_name;
        info += "] Expected \"";
        info += expected_name;
        info += "\" Actual: \"";
        info += relation_name(r);
        info += '"';
        asGetActiveContext()->SetException(info.c_str());

        return false;
    }
}

bool expect_le(void* lhs_ref, int lhs_t_id, void* rhs_ref, int rhs_t_id)
{
    return expect_cmp_impl(
        [](std::weak_ordering r)
        { return r <= 0; },
        "less or equal",
        lhs_ref,
        lhs_t_id,
        rhs_ref,
        rhs_t_id,
        __func__
    );
}

int register_unittest(asIScriptEngine* engine)
{
    std::string previous_ns = engine->GetDefaultNamespace();
    engine->SetDefaultNamespace("testing");

    engine->RegisterGlobalFunction(
        "bool expect_eq(?&in, ?&in)",
        asFUNCTION(expect_eq),
        asCALL_CDECL
    );
    engine->RegisterGlobalFunction(
        "bool expect_le(?&in, ?&in)",
        asFUNCTION(expect_le),
        asCALL_CDECL
    );

    engine->SetDefaultNamespace(previous_ns.c_str());

    return asSUCCESS;
}
} // namespace asbind20::ext
