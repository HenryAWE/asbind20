#include <asbind20/ext/helper.hpp>

namespace asbind20::ext
{
bool is_primitive_type(int type_id)
{
    return type_id >= asTYPEID_VOID && type_id <= asTYPEID_DOUBLE;
}

asUINT sizeof_script_type(asIScriptEngine* engine, int type_id)
{
    if(is_primitive_type(type_id))
        return engine->GetSizeOfPrimitiveType(type_id);

    asITypeInfo* ti = engine->GetTypeInfoById(type_id);
    if(!ti)
        return 0;

    return ti->GetSize();
}

primitive_t extract_primitive_value(asIScriptEngine* engine, void* ref, int type_id)
{
    switch(type_id)
    {
    default:
        throw std::invalid_argument("invalid type");

    case asTYPEID_VOID:
        return primitive_t();
    case asTYPEID_BOOL:
        return *static_cast<bool*>(ref);
    case asTYPEID_INT8:
        return *static_cast<std::int8_t*>(ref);
    case asTYPEID_INT16:
        return *static_cast<std::int16_t*>(ref);
    case asTYPEID_INT32:
        return *static_cast<std::int32_t*>(ref);
    case asTYPEID_INT64:
        return *static_cast<std::int64_t*>(ref);
    case asTYPEID_UINT8:
        return *static_cast<std::uint8_t*>(ref);
    case asTYPEID_UINT16:
        return *static_cast<std::uint16_t*>(ref);
    case asTYPEID_UINT32:
        return *static_cast<std::uint32_t*>(ref);
    case asTYPEID_UINT64:
        return *static_cast<std::uint64_t*>(ref);
    case asTYPEID_FLOAT:
        return *static_cast<float*>(ref);
    case asTYPEID_DOUBLE:
        return *static_cast<double*>(ref);
    }
}

std::partial_ordering script_compare(
    asIScriptEngine* engine,
    void* lhs_ref,
    int lhs_type_id,
    void* rhs_ref,
    int rhs_type_id
)
{
    if(is_primitive_type(lhs_type_id) && is_primitive_type(rhs_type_id))
    {
        return std::visit(
            []<typename T, typename U>(T lhs, U rhs) -> std::partial_ordering
            {
                if constexpr(std::is_same_v<T, std::monostate> || std::is_same_v<U, std::monostate>)
                {
                    if constexpr(std::is_same_v<T, U>)
                        return std::partial_ordering::equivalent;
                    else
                        return std::partial_ordering::unordered;
                }
                else
                {
                    using common_t = std::common_type_t<T, U>;

                    return static_cast<common_t>(lhs) <=> static_cast<common_t>(rhs);
                }
            },
            extract_primitive_value(engine, lhs_ref, lhs_type_id),
            extract_primitive_value(engine, rhs_ref, rhs_type_id)
        );
    }

    asIScriptFunction* op_cmp = nullptr;

    auto pred = [](asIScriptFunction* f, int param_type_id) -> bool
    {
        using namespace std::literals;

        asDWORD type_flags;
        int return_type_id = f->GetReturnTypeId(&type_flags);
        if(
            f->GetName() == "opCmp"sv &&
            return_type_id == asTYPEID_INT32 &&
            type_flags == asTM_NONE &&
            f->GetParamCount() == 1
        )
        {
            int func_param_type_id;
            f->GetParam(0, &func_param_type_id, &type_flags);

            // According to the official add-on:
            // The parameter must be an input reference of the same type
            // If the reference is a inout reference, then it must also be read-only
            return (type_flags & asTM_INREF) ||
                   func_param_type_id == param_type_id ||
                   ((type_flags & asTM_OUTREF) && (type_flags & asTM_CONST));
        }

        return false;
    };

    bool reversed = false;
    if(!is_primitive_type(lhs_type_id))
    {
        asITypeInfo* ti = engine->GetTypeInfoById(lhs_type_id);
        for(asUINT i = 0; i < ti->GetMethodCount(); ++i)
        {
            asIScriptFunction* f = ti->GetMethodByIndex(i);
            if(pred(f, rhs_type_id))
            {
                op_cmp = f;
                break;
            }
        }
    }
    if(!op_cmp)
    {
        asITypeInfo* ti = engine->GetTypeInfoById(rhs_type_id);
        for(asUINT i = 0; i < ti->GetMethodCount(); ++i)
        {
            asIScriptFunction* f = ti->GetMethodByIndex(i);
            if(pred(f, lhs_type_id))
            {
                op_cmp = f;
                reversed = true;
                break;
            }
        }
    }

    if(!op_cmp)
        return std::partial_ordering::unordered;

    asIScriptContext* ctx = engine->CreateContext();
    if(!reversed)
    {
        auto result = script_invoke<asINT32>(
            ctx,
            static_cast<asIScriptObject*>(lhs_ref),
            op_cmp,
            static_cast<asIScriptObject*>(rhs_ref)
        );
        if(!result.has_value())
            return std::partial_ordering::unordered;
        return *result <=> 0;
    }
    else
    {
        auto result = script_invoke<asINT32>(
            ctx,
            static_cast<asIScriptObject*>(lhs_ref),
            op_cmp,
            static_cast<asIScriptObject*>(rhs_ref)
        );
        if(!result.has_value())
            return std::partial_ordering::unordered;
        return 0 <=> *result;
    }

    return std::partial_ordering::unordered;
}
} // namespace asbind20::ext
