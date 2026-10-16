// Tests for small_vector with reference type elements (asOBJ_REF / handles)
// and the GC support via enum_refs().

#include <asbind_test/framework.hpp>
#include <asbind20/container/small_vector.hpp>
#include "sv_helper.hpp"

namespace
{
using sv_type = asbind20::container::small_vector<
    asbind20::container::typeinfo_identity,
    4 * sizeof(void*),
    std::allocator<void>>;
} // namespace

TEST(SmallVector, RefHandleAsElement)
{
    // TODO: Why are we triggering AS assertion in this case?
    ASBIND_TEST_SKIP_IF_SCRIPT_DEBUG();

    using namespace asbind20;
    using test_container::sv_ref_foo;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_container::register_sv_ref_foo(engine.get());

    auto* foo_ti = engine->GetTypeInfoByName("sv_ref_foo");
    ASSERT_THAT(foo_ti, ::testing::NotNull());
    int foo_handle_id = engine->GetTypeIdByDecl("sv_ref_foo@");
    ASSERT_TRUE(is_objhandle(foo_handle_id));

    const int baseline = sv_ref_foo::live_count;
    {
        SCOPED_TRACE("baseline live_count = " + std::to_string(baseline));

        sv_type v(engine, foo_handle_id);
        EXPECT_TRUE(is_objhandle(v.element_type_id()));

        // Create two objects and store them as handles
        sv_ref_foo* a = new sv_ref_foo;
        a->data = 1;
        sv_ref_foo* b = new sv_ref_foo;
        b->data = 2;

        // push_back stores a handle: the container takes an AddRef
        v.push_back(&a);
        v.push_back(&b);
        ASSERT_EQ(v.size(), 2);
        EXPECT_EQ(a->use_count(), 2); // host + container
        EXPECT_EQ(b->use_count(), 2);

        // Release the host references
        a->release();
        b->release();

        // Elements are still alive (owned by the container)
        EXPECT_EQ(sv_ref_foo::live_count, baseline + 2);
        auto* obj_a = *static_cast<sv_ref_foo**>(v[0]);
        auto* obj_b = *static_cast<sv_ref_foo**>(v[1]);
        ASSERT_THAT(obj_a, ::testing::NotNull());
        ASSERT_THAT(obj_b, ::testing::NotNull());
        EXPECT_EQ(obj_a->data, 1);
        EXPECT_EQ(obj_b->data, 2);

        // insert a handle in the middle
        sv_ref_foo* c = new sv_ref_foo;
        c->data = 3;
        v.insert(v.begin() + 1, &c);
        c->release();
        ASSERT_EQ(v.size(), 3);
        EXPECT_EQ(sv_ref_foo::live_count, baseline + 3);

        // erase the inserted element -> the container releases it
        v.erase(v.begin() + 1);
        ASSERT_EQ(v.size(), 2);
        EXPECT_EQ(sv_ref_foo::live_count, baseline + 2);

        v.enum_refs(); // Should be no-op

        v.clear();
        EXPECT_EQ(sv_ref_foo::live_count, baseline);
    }

    EXPECT_EQ(sv_ref_foo::live_count, baseline)
        << "Destructor should release the remaining elements";
}

TEST(SmallVector, RefHandleResize)
{
    using namespace asbind20;
    using test_container::sv_ref_foo;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_container::register_sv_ref_foo(engine.get());

    int foo_handle_id = engine->GetTypeIdByDecl("sv_ref_foo@");
    ASSERT_TRUE(is_objhandle(foo_handle_id));

    const int baseline = sv_ref_foo::live_count;
    {
        sv_type v(engine, foo_handle_id);

        // Growing with handle elements initializes them to null
        v.resize(4);
        ASSERT_EQ(v.size(), 4);
        for(std::size_t i = 0; i < v.size(); ++i)
        {
            SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
            EXPECT_EQ(*static_cast<void**>(v[i]), nullptr);
        }

        // Store real handles through assign
        for(std::size_t i = 0; i < 4; ++i)
        {
            sv_ref_foo* obj = new sv_ref_foo;
            obj->data = static_cast<int>(i);
            v.assign(i, &obj);
            obj->release();
        }
        EXPECT_EQ(sv_ref_foo::live_count, baseline + 4);
        for(std::size_t i = 0; i < 4; ++i)
        {
            SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
            auto* obj = *static_cast<sv_ref_foo**>(v[i]);
            ASSERT_THAT(obj, ::testing::NotNull());
            EXPECT_EQ(obj->data, static_cast<int>(i));
        }

        // Shrinking releases the removed handles
        v.resize(2);
        ASSERT_EQ(v.size(), 2);
        EXPECT_EQ(sv_ref_foo::live_count, baseline + 2);

        // pop_back also releases
        v.pop_back();
        ASSERT_EQ(v.size(), 1);
        EXPECT_EQ(sv_ref_foo::live_count, baseline + 1);
    }
    EXPECT_EQ(sv_ref_foo::live_count, baseline);
}

// Regression test: assigning an element to itself must not release the last
// reference and then AddRef the destroyed object. `v[i]` returns the address
// of the stored handle, so `v.assign(i, v[i])` makes the source alias the
// destination slot.
TEST(SmallVector, RefHandleAssignSelfAlias)
{
    using namespace asbind20;
    using test_container::sv_ref_foo;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_container::register_sv_ref_foo(engine.get());

    int foo_handle_id = engine->GetTypeIdByDecl("sv_ref_foo@");
    ASSERT_TRUE(is_objhandle(foo_handle_id));

    const int baseline = sv_ref_foo::live_count;
    {
        sv_type v(engine, foo_handle_id);

        sv_ref_foo* obj = new sv_ref_foo;
        obj->data = 42;
        v.push_back(&obj);
        obj->release(); // the container is now the only owner

        EXPECT_EQ(sv_ref_foo::live_count, baseline + 1);

        // v[0] is the address of the stored handle, i.e. the destination slot.
        v.assign(0, v[0]);

        EXPECT_EQ(sv_ref_foo::live_count, baseline + 1)
            << "the element must not be destroyed by a self-assignment";
        EXPECT_EQ(obj->use_count(), 1u);

        auto* got = *static_cast<sv_ref_foo**>(v[0]);
        ASSERT_THAT(got, ::testing::NotNull());
        EXPECT_EQ(got, obj);
        EXPECT_EQ(got->data, 42);

        // A real (non-aliasing) assignment must still swap the stored handle.
        sv_ref_foo* other = new sv_ref_foo;
        other->data = 13;
        void* src = other;
        v.assign(0, &src);

        EXPECT_EQ(sv_ref_foo::live_count, baseline + 1)
            << "the previously stored element should have been released";
        EXPECT_EQ(other->use_count(), 2u); // host + container
        EXPECT_EQ(*static_cast<sv_ref_foo**>(v[0]), other);

        // Drop the host reference; the vector still owns one and will release
        // it in its destructor.
        other->release();
    }
    EXPECT_EQ(sv_ref_foo::live_count, baseline)
        << "Destructor should release the remaining elements";
}

// Regression test: the new reference must be taken *before* the old one is
// released. The source handle may live inside the element being released, and
// that element may own the last reference to the source handle.
TEST(SmallVector, RefHandleAssignNestedSourceAlias)
{
    using namespace asbind20;
    using test_container::sv_ref_foo;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_container::register_sv_ref_foo(engine.get());

    int foo_handle_id = engine->GetTypeIdByDecl("sv_ref_foo@");
    ASSERT_TRUE(is_objhandle(foo_handle_id));

    const int baseline = sv_ref_foo::live_count;
    {
        sv_type v(engine, foo_handle_id);

        sv_ref_foo* parent = new sv_ref_foo;
        sv_ref_foo* child = new sv_ref_foo;
        parent->set_child(child); // child use count == 2
        child->release();         // use count == 1, owned by `parent`
        parent->data = 1;
        child->data = 2;

        v.push_back(&parent); // container takes a reference to `parent`
        parent->release();    // now the container is the only owner

        // `ref` points inside the stored element. Releasing it destroys
        // `parent` and thereby releases the only reference to `child`.
        v.assign(0, &parent->child);

        EXPECT_EQ(sv_ref_foo::live_count, baseline + 1)
            << "child must survive the release of its parent";
        EXPECT_EQ(child->use_count(), 1u);
        EXPECT_EQ(*static_cast<sv_ref_foo**>(v[0]), child);
        EXPECT_EQ(child->data, 2);
    }
    EXPECT_EQ(sv_ref_foo::live_count, baseline)
        << "Destructor should release the remaining elements";
}

namespace
{
// Fill a handle container up to a heap buffer's capacity so that the next
// push_back() has to reallocate (and free) the buffer.
void fill_ref_sv_to_capacity(sv_type& v, const int baseline)
{
    using test_container::sv_ref_foo;

    const std::size_t static_cap = v.static_capacity();
    for(std::size_t i = 0; i < static_cap + 1; ++i)
    {
        sv_ref_foo* o = new sv_ref_foo;
        o->data = static_cast<int>(i);
        v.push_back(&o);
        o->release();
    }
    ASSERT_GT(v.capacity(), static_cap) << "expected a heap buffer";

    while(v.size() < v.capacity())
    {
        sv_ref_foo* o = new sv_ref_foo;
        o->data = static_cast<int>(v.size());
        v.push_back(&o);
        o->release();
    }
    ASSERT_EQ(v.size(), v.capacity());
    EXPECT_EQ(sv_ref_foo::live_count, baseline + static_cast<int>(v.size()));
}
} // namespace

// Regression test: `v.push_back(v[0])` for a handle container. `v[0]` is the
// address of the stored handle slot, which lives in the buffer that reserve()
// reallocates and frees, so it must be resolved before the growth.
TEST(SmallVector, RefHandlePushBackFromItselfReallocating)
{
    using namespace asbind20;
    using test_container::sv_ref_foo;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_container::register_sv_ref_foo(engine.get());

    int foo_handle_id = engine->GetTypeIdByDecl("sv_ref_foo@");
    ASSERT_TRUE(is_objhandle(foo_handle_id));

    const int baseline = sv_ref_foo::live_count;
    {
        sv_type v(engine, foo_handle_id);
        fill_ref_sv_to_capacity(v, baseline);

        auto* first = *static_cast<sv_ref_foo**>(v[0]);
        ASSERT_THAT(first, ::testing::NotNull());
        EXPECT_EQ(first->use_count(), 1u); // container is the sole owner

        const int live_before = sv_ref_foo::live_count;
        const std::size_t old_cap = v.capacity();
        v.push_back(v[0]);

        ASSERT_EQ(v.size(), old_cap + 1);
        EXPECT_GT(v.capacity(), old_cap);
        EXPECT_EQ(first->use_count(), 2u); // the container now owns it twice
        EXPECT_EQ(*static_cast<sv_ref_foo**>(v[v.size() - 1]), first);
        EXPECT_EQ(sv_ref_foo::live_count, live_before)
            << "pushing an existing handle must not create or destroy objects";
    }
    EXPECT_EQ(sv_ref_foo::live_count, baseline)
        << "Destructor should release the remaining elements";
}

TEST(SmallVector, RefHandlePushBackNFromItselfReallocating)
{
    using namespace asbind20;
    using test_container::sv_ref_foo;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_container::register_sv_ref_foo(engine.get());

    int foo_handle_id = engine->GetTypeIdByDecl("sv_ref_foo@");
    ASSERT_TRUE(is_objhandle(foo_handle_id));

    const int baseline = sv_ref_foo::live_count;
    {
        sv_type v(engine, foo_handle_id);
        fill_ref_sv_to_capacity(v, baseline);

        auto* first = *static_cast<sv_ref_foo**>(v[0]);
        ASSERT_THAT(first, ::testing::NotNull());
        EXPECT_EQ(first->use_count(), 1u);

        const int live_before = sv_ref_foo::live_count;
        const std::size_t old_size = v.size();
        const std::size_t old_cap = v.capacity();
        v.push_back_n(3, v[0]);

        ASSERT_EQ(v.size(), old_size + 3);
        EXPECT_GT(v.capacity(), old_cap);
        EXPECT_EQ(first->use_count(), 4u); // original slot + 3 new elements
        for(std::size_t i = old_size; i < v.size(); ++i)
        {
            SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
            EXPECT_EQ(*static_cast<sv_ref_foo**>(v[i]), first);
        }
        EXPECT_EQ(sv_ref_foo::live_count, live_before)
            << "pushing an existing handle must not create or destroy objects";
    }
    EXPECT_EQ(sv_ref_foo::live_count, baseline)
        << "Destructor should release the remaining elements";
}
