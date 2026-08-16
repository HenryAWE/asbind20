#include "sv_helper.hpp"

namespace test_container
{
int_sv_type make_int_sv(std::initializer_list<int> values)
{
    // We won't use any AngelScript APIs for primitive element types,
    // so we can pass nullptr for the engine and use the type ID directly

    int_sv_type sv(nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    sv.reserve(values.size());
    for(int val : values)
        sv.push_back(&val);

    EXPECT_EQ(sv.size(), values.size());
    return sv;
}

void register_sv_ref_foo(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    ref_class<sv_ref_foo, true> c(
        engine, "sv_ref_foo", AS_NAMESPACE_QUALIFIER asOBJ_GC
    );
    c
        .default_factory()
        .addref(fp<&sv_ref_foo::addref>)
        .release(fp<&sv_ref_foo::release>)
        .set_gc_flag(fp<&sv_ref_foo::set_gc_flag>)
        .get_gc_flag(fp<&sv_ref_foo::get_gc_flag>)
        .enum_refs(fp<&sv_ref_foo::enum_refs>)
        .release_refs(fp<&sv_ref_foo::release_refs>)
        .method("uint use_count() const", fp<&sv_ref_foo::use_count>)
        .property("int data", &sv_ref_foo::data);
}
} // namespace test_container
