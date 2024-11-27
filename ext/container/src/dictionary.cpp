#include <asbind20/ext/dictionary.hpp>

namespace asbind20::ext
{
int dictionary::get_refcount() const
{
    return m_refcount;
}

void dictionary::set_gc_flag()
{
    m_gc_flag = true;
}

bool dictionary::get_gc_flag() const
{
    return m_gc_flag;
}

void dictionary::enum_refs(asIScriptEngine* engine)
{
    std::lock_guard guard(*this);

    for(auto& [k, v] : m_container)
    {
        if(!(v.type_id & asTYPEID_MASK_OBJECT))
            continue;

        void* obj = v.type_id & asTYPEID_OBJHANDLE ?
                        *(void**)v.storage.local :
                        v.storage.ptr;
        if(!obj)
            continue;

        asITypeInfo* ti = v.type_info(engine);
        assert(ti != nullptr);
        if(ti->GetFlags() & asOBJ_REF)
        {
            engine->GCEnumCallback(obj);
        }
        else if((ti->GetFlags() & asOBJ_VALUE) && (ti->GetFlags() & asOBJ_GC))
        {
            engine->ForwardGCEnumReferences(obj, ti);
        }
    }
}

void dictionary::release_refs(asIScriptEngine* engine)
{
    clear();
}

static void dictionary_factory_default(asIScriptGeneric* gen)
{
    dictionary* ptr = new dictionary(gen->GetEngine());
    if(ptr)
    {
        asIScriptEngine* engine = gen->GetEngine();
        int tid = gen->GetReturnTypeId();
        assert(tid != 0);
        asITypeInfo* ti = engine->GetTypeInfoById(tid);
        int r = gen->GetEngine()->NotifyGarbageCollectorOfNewObject(ptr, ti);
        assert(r >= 0);
    }
    gen->SetReturnAddress(ptr);
}

template <bool UseGeneric>
static void register_script_dictionary_impl(asIScriptEngine* engine)
{
    using std::string;

    ref_class<dictionary, UseGeneric> c(engine, "dictionary", asOBJ_GC);
    c
        .factory_function("", &dictionary_factory_default)
        .template addref<&dictionary::addref>()
        .template release<&dictionary::release>()
        .template get_refcount<&dictionary::get_refcount>()
        .template set_gc_flag<&dictionary::set_gc_flag>()
        .template get_gc_flag<&dictionary::get_gc_flag>()
        .template enum_refs<&dictionary::enum_refs>()
        .template release_refs<&dictionary::release_refs>();

    const char* try_emplace_decl = "bool try_emplace(const string&in k, const ?&in value)";
    const char* get_decl = "bool get(const string&in k, ?&out value) const";

    if constexpr(UseGeneric)
    {
        c.method(
            try_emplace_decl,
            +[](asIScriptGeneric* gen) -> void
            {
                set_generic_return<bool>(
                    gen,
                    get_generic_object<dictionary>(gen).try_emplace(
                        get_generic_arg<const string&>(gen, 0),
                        (const void*)gen->GetArgAddress(1),
                        gen->GetArgTypeId(1)
                    )
                );
            }
        );
        c.method(
            get_decl,
            +[](asIScriptGeneric* gen) -> void
            {
                set_generic_return<bool>(
                    gen,
                    get_generic_object<dictionary>(gen).get(
                        get_generic_arg<const string&>(gen, 0),
                        gen->GetArgAddress(1),
                        gen->GetArgTypeId(1)
                    )
                );
            }
        );
    }
    else
    {
        c
            .method(try_emplace_decl, &dictionary::try_emplace)
            .method(get_decl, &dictionary::get);
    }

    c
        .template method<&dictionary::erase>("bool erase(const string&in k)")
        .template method<static_cast<bool (dictionary::*)(const string&) const>(&dictionary::contains)>("bool contains(const string&in k) const");
}

void register_script_dictionary(asIScriptEngine* engine, bool generic)
{
    if(generic)
        register_script_dictionary_impl<true>(engine);
    else
        register_script_dictionary_impl<false>(engine);
}
} // namespace asbind20::ext
