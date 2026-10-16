// Regression tests for the Release-then-AddRef use-after-free on
// self-assignment in `asbind20::container::single`.

#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>

namespace test_gc
{
class single_self_assign_foo
{
public:
    using counter_type = AS_NAMESPACE_QUALIFIER asUINT;

    single_self_assign_foo()
    {
        ++live_count;
    }

    ~single_self_assign_foo()
    {
        --live_count;
        if(child)
            child->release();
    }

    void addref()
    {
        m_use_count += 1;
    }

    void release()
    {
        if(m_use_count == 0) [[unlikely]]
            return;
        m_use_count -= 1;
        if(m_use_count == 0)
            delete this;
    }

    void set_gc_flag() {}

    bool get_gc_flag() const
    {
        return true;
    }

    void enum_refs(asbind20::engine_pointer) {}

    void release_refs(asbind20::engine_pointer) {}

    [[nodiscard]]
    counter_type use_count() const
    {
        return m_use_count;
    }

    int data = 0;

    // Owning handle to another object of the same type. It is used to build
    // the case where the source slot lives inside the object being released.
    single_self_assign_foo* child = nullptr;

    inline static int live_count = 0;

private:
    counter_type m_use_count = 1;
};

static void register_single_self_assign_foo(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    ref_class<single_self_assign_foo, true> c(
        engine, "single_self_assign_foo", AS_NAMESPACE_QUALIFIER asOBJ_GC
    );
    c
        .default_factory()
        .addref(fp<&single_self_assign_foo::addref>)
        .release(fp<&single_self_assign_foo::release>)
        .set_gc_flag(fp<&single_self_assign_foo::set_gc_flag>)
        .get_gc_flag(fp<&single_self_assign_foo::get_gc_flag>)
        .enum_refs(fp<&single_self_assign_foo::enum_refs>)
        .release_refs(fp<&single_self_assign_foo::release_refs>)
        .method("uint use_count() const", fp<&single_self_assign_foo::use_count>)
        .property("int data", &single_self_assign_foo::data);
}
} // namespace test_gc

TEST(ContainerSingle, SelfAssignFromAliasedHandle)
{
    using namespace asbind20;
    using test_gc::single_self_assign_foo;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_gc::register_single_self_assign_foo(engine.get());

    const int type_id = engine->GetTypeIdByDecl("single_self_assign_foo@");
    ASSERT_TRUE(is_objhandle(type_id));

    const int baseline = single_self_assign_foo::live_count;
    {
        container::single::data_type data;

        auto* obj = new single_self_assign_foo;
        obj->data = 42;
        data.handle = obj; // the container is the only owner (use count == 1)

        // Self-assignment through the container's own stored slot.
        container::single::copy_assign_from(
            data, engine.get(), type_id, container::single::data_address(data, type_id)
        );

        EXPECT_EQ(single_self_assign_foo::live_count, baseline + 1)
            << "the stored object must not be destroyed by a self-assignment";
        EXPECT_EQ(obj->use_count(), 1u);

        // `copy_assign_to` with the destination aliasing the stored handle.
        container::single::copy_assign_to(
            data, engine.get(), type_id, container::single::data_address(data, type_id)
        );

        EXPECT_EQ(single_self_assign_foo::live_count, baseline + 1)
            << "the stored object must not be destroyed by a self-assignment";
        EXPECT_EQ(obj->use_count(), 1u);

        void* stored = container::single::data_address(data, type_id);
        EXPECT_EQ(*static_cast<single_self_assign_foo**>(stored), obj);
        EXPECT_EQ(obj->data, 42);

        container::single::destroy(data, engine.get(), type_id);
    }
    EXPECT_EQ(single_self_assign_foo::live_count, baseline);
}

TEST(ContainerSingle, AssignDifferentHandle)
{
    using namespace asbind20;
    using test_gc::single_self_assign_foo;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_gc::register_single_self_assign_foo(engine.get());

    const int type_id = engine->GetTypeIdByDecl("single_self_assign_foo@");
    ASSERT_TRUE(is_objhandle(type_id));

    const int baseline = single_self_assign_foo::live_count;
    {
        container::single::data_type data;

        auto* old_obj = new single_self_assign_foo;
        data.handle = old_obj; // use count == 1

        // A real (non-aliasing) assignment must still release the old object
        // and take a reference to the new one.
        auto* new_obj = new single_self_assign_foo; // use count == 1
        void* src = new_obj;

        container::single::copy_assign_from(
            data, engine.get(), type_id, &src
        );

        EXPECT_EQ(single_self_assign_foo::live_count, baseline + 1)
            << "the old object should have been released and destroyed";
        EXPECT_EQ(new_obj->use_count(), 2u); // host + container

        EXPECT_EQ(*static_cast<single_self_assign_foo**>(container::single::data_address(data, type_id)), new_obj);

        container::single::destroy(data, engine.get(), type_id);
        EXPECT_EQ(new_obj->use_count(), 1u); // only the host reference left
        new_obj->release();
    }
    EXPECT_EQ(single_self_assign_foo::live_count, baseline);
}

TEST(ContainerSingle, NestedSourceInsideReleasedObject)
{
    using namespace asbind20;
    using test_gc::single_self_assign_foo;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_gc::register_single_self_assign_foo(engine.get());

    const int type_id = engine->GetTypeIdByDecl("single_self_assign_foo@");
    ASSERT_TRUE(is_objhandle(type_id));

    const int baseline = single_self_assign_foo::live_count;
    {
        container::single::data_type data;

        // The new reference must be taken *before* the old one is released
        auto* parent = new single_self_assign_foo; // use count == 1
        auto* child = new single_self_assign_foo; // use count == 1
        parent->child = child; // the host reference is transferred to `parent`
        data.handle = parent; // the container owns `parent`

        // `ref` points inside the stored object. Releasing `parent` destroys it
        // and thereby releases the only reference to `child`.
        container::single::copy_assign_from(
            data, engine.get(), type_id, &parent->child
        );

        EXPECT_EQ(single_self_assign_foo::live_count, baseline + 1)
            << "child must survive the release of its parent";
        EXPECT_EQ(child->use_count(), 1u);
        EXPECT_EQ(*static_cast<single_self_assign_foo**>(container::single::data_address(data, type_id)), child);

        container::single::destroy(data, engine.get(), type_id);
    }
    EXPECT_EQ(single_self_assign_foo::live_count, baseline);
}
