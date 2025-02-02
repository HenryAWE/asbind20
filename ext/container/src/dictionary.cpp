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
        .addref(fp<&dictionary::addref>)
        .release(fp<&dictionary::release>)
        .get_refcount(fp<&dictionary::get_refcount>)
        .set_gc_flag(fp<&dictionary::set_gc_flag>)
        .get_gc_flag(fp<&dictionary::get_gc_flag>)
        .enum_refs(fp<&dictionary::enum_refs>)
        .release_refs(fp<&dictionary::release_refs>)
        .method("bool try_emplace(const string&in k, const ?&in value)", fp<&dictionary::try_emplace>, var_type<1>)
        .method("bool get(const string&in k, ?&out value) const", fp<&dictionary::get>, var_type<1>)
        .method("bool erase(const string&in k)", fp<&dictionary::erase>)
        .method("bool contains(const string&in k) const", fp<static_cast<bool (dictionary::*)(const string&) const>(&dictionary::contains)>);
}

void register_script_dictionary(asIScriptEngine* engine, bool generic)
{
    if(generic)
        register_script_dictionary_impl<true>(engine);
    else
        register_script_dictionary_impl<false>(engine);
}
} // namespace asbind20::ext
