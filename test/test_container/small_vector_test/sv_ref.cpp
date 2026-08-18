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
    test_container::register_sv_ref_foo(engine);

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
    test_container::register_sv_ref_foo(engine);

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
