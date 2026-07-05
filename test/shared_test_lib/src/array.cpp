#include <asbind_test/array.hpp>
#include <cstring>
#include <asbind20/invoke.hpp>
#include <asbind_test/framework.hpp>

namespace asbind_test
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
        EXPECT_FALSE(asbind20::is_primitive_type(subtype_id));

        if(asbind20::is_objhandle(subtype_id))
        {
            if(*(const void* const*)lhs == *(const void* const*)rhs)
                return true;
        }

        if(!cache) [[unlikely]]
            return false;

        EXPECT_NE(ctx, nullptr);
        if(cache->comp.has_opEquals()) [[likely]]
        {
            auto result = cache->comp.get_opEquals()(
                ctx,
                lhs,
                rhs
            );

            if(!result.has_value())
                return false;
            return *result;
        }
        // Fallback to OpCmp() == 0
        else if(cache->comp.has_opCmp())
        {
            auto result = cache->comp.get_opCmp()(
                ctx,
                lhs,
                rhs
            );

            if(!result.has_value())
                return false;
            return *result == 0;
        }

        return false;
    }

    void script_array_base::find_required_elem_methods(
        array_cache& out, int subtype_id, asbind20::typeinfo_pointer ti
    )
    {
        EXPECT_FALSE(asbind20::is_primitive_type(subtype_id));

        auto* subtype_ti = ti->GetSubType(0);
        out.comp = asbind20::container::get_comparator(subtype_ti).get();
    }

    void script_array_base::generate_cache(
        array_cache& out, int subtype_id, asbind20::typeinfo_pointer ti
    )
    {
        if(!asbind20::is_primitive_type(subtype_id))
            find_required_elem_methods(out, subtype_id, ti);

        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = ti->GetEngine();
        {
            const char* subtype_decl = engine->GetTypeDeclaration(subtype_id, true);
            out.iterator_ti = engine->GetTypeInfoByDecl(
                asbind20::string_concat("array_iterator<", subtype_decl, '>').c_str()
            );
        }
    }

    bool script_array_base::template_callback(
        asbind20::typeinfo_pointer ti, bool& no_gc
    )
    {
        int subtype_id = ti->GetSubTypeId();
        if(asbind20::is_void_type(subtype_id))
            return false;

        if(asbind20::is_primitive_type(subtype_id))
        {
            no_gc = true;
        }
        else if(!asbind20::is_objhandle(subtype_id))
        {
            asbind20::typeinfo_pointer subtype_ti = ti->GetSubType();
            EXPECT_NE(subtype_ti, nullptr);
            auto flags = subtype_ti->GetFlags();

            if(
                (flags & AS_NAMESPACE_QUALIFIER asOBJ_VALUE) &&
                !(flags & AS_NAMESPACE_QUALIFIER asOBJ_POD)
            )
            {
                if(!asbind20::get_default_constructor(subtype_ti))
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
                if(!asbind20::get_default_factory(ti->GetSubType()))
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
            EXPECT_TRUE(asbind20::is_objhandle(subtype_id));

            // According to the official add-on, if a handle cannot refer to an object type
            // that can form a circular reference with the array,
            // it's not necessary to make the array garbage collected.
            asbind20::typeinfo_pointer subtype_ti = ti->GetSubType();
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
} // namespace asbind_test
