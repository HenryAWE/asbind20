#include <asbind20/ext/array.hpp>
#include <cassert>
#include <cstring>
#include <asbind20/invoke.hpp>

namespace asbind20::ext
{
namespace detail
{
    void* script_array_base::operator new(std::size_t bytes)
    {
        return AS_NAMESPACE_QUALIFIER asAllocMem(bytes);
    }

    void script_array_base::operator delete(void* p)
    {
        return AS_NAMESPACE_QUALIFIER asFreeMem(p);
    }

    bool script_array_base::elem_opEquals(
        int subtype_id,
        const void* lhs,
        const void* rhs,
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx,
        const array_cache* cache
    )
    {
        assert(!is_primitive_type(subtype_id));

        if(is_objhandle(subtype_id))
        {
            if(*(const void* const*)lhs == *(const void* const*)rhs)
                return true;
        }

        if(!cache) [[unlikely]]
            return false;

        assert(ctx != nullptr);
        if(cache->subtype_opEquals) [[likely]]
        {
            auto result = script_invoke<bool>(
                ctx,
                lhs,
                cache->subtype_opEquals,
                rhs
            );

            if(!result.has_value())
                return false;
            return *result;
        }
        // Fallback to OpCmp() == 0
        else if(cache->subtype_opCmp)
        {
            auto result = script_invoke<int>(
                ctx,
                lhs,
                cache->subtype_opCmp,
                rhs
            );

            if(!result.has_value())
                return false;
            return *result == 0;
        }

        return false;
    }

    void script_array_base::find_required_elem_methods(
        array_cache& out, int subtype_id, AS_NAMESPACE_QUALIFIER asITypeInfo* ti
    )
    {
        assert(!is_primitive_type(subtype_id));

        bool must_be_const = subtype_id & AS_NAMESPACE_QUALIFIER asTYPEID_HANDLETOCONST;

        AS_NAMESPACE_QUALIFIER asITypeInfo* subtype_ti =
            ti->GetEngine()->GetTypeInfoById(subtype_id);
        assert(subtype_ti != nullptr);

        for(AS_NAMESPACE_QUALIFIER asUINT i = 0; i < subtype_ti->GetMethodCount(); ++i)
        {
            AS_NAMESPACE_QUALIFIER asIScriptFunction* func = subtype_ti->GetMethodByIndex(i);

            if(func->GetParamCount() == 1 && (!must_be_const || func->IsReadOnly()))
            {
                using namespace std::string_view_literals;

                AS_NAMESPACE_QUALIFIER asDWORD flags = 0;
                int return_type_id = func->GetReturnTypeId(&flags);
                if(flags != asTM_NONE)
                    continue;

                bool is_opEquals = false;
                bool is_opCmp = false;

                if(return_type_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT32 &&
                   func->GetName() == "opCmp"sv)
                    is_opCmp = true;
                else if(return_type_id == AS_NAMESPACE_QUALIFIER asTYPEID_BOOL &&
                        func->GetName() == "opEquals"sv)
                    is_opEquals = true;

                if(!is_opEquals && !is_opCmp)
                    continue;

                int param_type_id;
                func->GetParam(0, &param_type_id, &flags);

                constexpr AS_NAMESPACE_QUALIFIER asQWORD handle_flags =
                    AS_NAMESPACE_QUALIFIER asTYPEID_OBJHANDLE |
                    AS_NAMESPACE_QUALIFIER asTYPEID_HANDLETOCONST;

                if((param_type_id & ~handle_flags) !=
                   (subtype_id & ~handle_flags))
                    continue;

                if((flags & AS_NAMESPACE_QUALIFIER asTM_INREF))
                {
                    if((param_type_id & AS_NAMESPACE_QUALIFIER asTYPEID_OBJHANDLE) ||
                       (must_be_const && !(flags & AS_NAMESPACE_QUALIFIER asTM_CONST)))
                        continue;
                }
                else if(param_type_id & AS_NAMESPACE_QUALIFIER asTYPEID_OBJHANDLE)
                {
                    if(must_be_const &&
                       !(param_type_id & AS_NAMESPACE_QUALIFIER asTYPEID_HANDLETOCONST))
                        continue;
                }
                else
                    continue;

                if(is_opCmp)
                {
                    if(out.subtype_opCmp || out.opCmp_status)
                    {
                        out.subtype_opCmp = nullptr;
                        out.opCmp_status = AS_NAMESPACE_QUALIFIER asMULTIPLE_FUNCTIONS;
                    }
                    else
                        out.subtype_opCmp = func;
                }
                else if(is_opEquals)
                {
                    if(out.subtype_opEquals || out.opEquals_status)
                    {
                        out.subtype_opEquals = nullptr;
                        out.opEquals_status = AS_NAMESPACE_QUALIFIER asMULTIPLE_FUNCTIONS;
                    }
                    else
                        out.subtype_opEquals = func;
                }
            }
        }

        if(!out.subtype_opEquals && out.opEquals_status == 0)
            out.opEquals_status = AS_NAMESPACE_QUALIFIER asNO_FUNCTION;
        if(!out.subtype_opCmp && out.opCmp_status == 0)
            out.opCmp_status = AS_NAMESPACE_QUALIFIER asNO_FUNCTION;
    }

    void script_array_base::generate_cache(
        array_cache& out, int subtype_id, AS_NAMESPACE_QUALIFIER asITypeInfo* ti
    )
    {
        if(!is_primitive_type(subtype_id))
            find_required_elem_methods(out, subtype_id, ti);

        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = ti->GetEngine();
        {
            const char* subtype_decl = engine->GetTypeDeclaration(subtype_id, true);
            out.iterator_ti = engine->GetTypeInfoByDecl(
                string_concat("array_iterator<", subtype_decl, '>').c_str()
            );
        }
    }

    bool script_array_base::template_callback(
        AS_NAMESPACE_QUALIFIER asITypeInfo* ti, bool& no_gc
    )
    {
        int subtype_id = ti->GetSubTypeId();
        if(is_void_type(subtype_id))
            return false;

        if(is_primitive_type(subtype_id))
        {
            no_gc = true;
        }
        else if(!is_objhandle(subtype_id))
        {
            AS_NAMESPACE_QUALIFIER asITypeInfo* subtype_ti = ti->GetSubType();
            assert(subtype_ti != nullptr);
            auto flags = subtype_ti->GetFlags();

            if(
                (flags & AS_NAMESPACE_QUALIFIER asOBJ_VALUE) &&
                !(flags & AS_NAMESPACE_QUALIFIER asOBJ_POD)
            )
            {
                if(!get_default_constructor(subtype_ti))
                {
                    ti->GetEngine()->WriteMessage(
                        "array",
                        0,
                        0,
                        AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR,
                        "The subtype has no default constructor"
                    );
                    return false;
                }
            }
            else if((flags & AS_NAMESPACE_QUALIFIER asOBJ_REF))
            {
                if(!get_default_factory(ti->GetSubType()))
                {
                    ti->GetEngine()->WriteMessage(
                        "array",
                        0,
                        0,
                        AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR,
                        "The subtype has no default factory"
                    );
                    return false;
                }
            }

            if(!(flags & AS_NAMESPACE_QUALIFIER asOBJ_GC))
                no_gc = true;
        }
        else
        {
            assert(is_objhandle(subtype_id));

            // According to the official add-on, if a handle cannot refer to an object type
            // that can form a circular reference with the array,
            // it's not necessary to make the array garbage collected.
            AS_NAMESPACE_QUALIFIER asITypeInfo* subtype_ti = ti->GetSubType();
            auto flags = subtype_ti->GetFlags();
            if(!(flags & AS_NAMESPACE_QUALIFIER asOBJ_GC))
            {
                if(flags & AS_NAMESPACE_QUALIFIER asOBJ_SCRIPT_OBJECT)
                {
                    // According to the official add-on, the classes that derive from it can form a circular reference.
                    if(flags & AS_NAMESPACE_QUALIFIER asOBJ_NOINHERIT)
                    {
                        // A script class declared as final cannot be inherited from.
                        no_gc = true;
                    }
                }
                else
                {
                    // Class registered by host, we assume it knows what it is doing.
                    no_gc = true;
                }
            }
        }

        return true;
    }
} // namespace detail
} // namespace asbind20::ext
