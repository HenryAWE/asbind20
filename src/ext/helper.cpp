#include <asbind20/ext/helper.hpp>
#include <functional>

namespace asbind20::ext
{
bool is_primitive_type(int type_id) noexcept
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
    asIScriptContext* exec_ctx =
        ctx ? ctx : engine->RequestContext();

    int r = detail::exec_impl(
        engine,
        exec_ctx,
        code
    );

    if(!ctx)
        engine->ReturnContext(exec_ctx);
    return r;
}

template <typename T>
std::uint64_t script_hash_impl(T val)
{
    return std::hash<T>{}(val);
}

void register_script_hash(asIScriptEngine* engine)
{
    global(engine)
        .typedef_("uint64", "hash_result_t")
        .function("uint64 hash(int8)", &script_hash_impl<std::int8_t>)
        .function("uint64 hash(int16)", &script_hash_impl<std::int16_t>)
        .function("uint64 hash(int)", &script_hash_impl<std::int32_t>)
        .function("uint64 hash(int64)", &script_hash_impl<std::int64_t>)
        .function("uint64 hash(uint8)", &script_hash_impl<std::uint8_t>)
        .function("uint64 hash(uint16)", &script_hash_impl<std::uint16_t>)
        .function("uint64 hash(uint)", &script_hash_impl<std::uint32_t>)
        .function("uint64 hash(uint64)", &script_hash_impl<std::uint64_t>)
        .function("uint64 hash(float)", &script_hash_impl<float>)
        .function("uint64 hash(double)", &script_hash_impl<double>);
}
} // namespace asbind20::ext
