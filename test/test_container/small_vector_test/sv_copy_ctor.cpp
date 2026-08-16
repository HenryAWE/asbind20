// Tests for small_vector copy construction

#include <asbind_test/framework.hpp>
#include <asbind20/container/small_vector.hpp>
#include "sv_helper.hpp"

using sv_type = test_container::int_sv_type;
using test_container::make_int_sv;

TEST(SmallVector, CopyCtorEmpty)
{
    auto v = make_int_sv();
    ASSERT_THAT(v, ::testing::IsEmpty());

    sv_type copy(v);
    EXPECT_THAT(copy, ::testing::IsEmpty());
    EXPECT_EQ(copy.element_type_id(), v.element_type_id());
    EXPECT_NE(copy.data(), v.data());
}

TEST(SmallVector, CopyCtorStatic)
{
    auto v = make_int_sv({0, 1, 2});
    ASSERT_LE(v.capacity(), v.static_capacity());

    sv_type copy(v);
    ASSERT_EQ(copy.size(), 3);
    EXPECT_EQ(*static_cast<int*>(copy[0]), 0);
    EXPECT_EQ(*static_cast<int*>(copy[1]), 1);
    EXPECT_EQ(*static_cast<int*>(copy[2]), 2);
    EXPECT_NE(copy.data(), v.data());

    // The copy is independent from the original
    int val = 99;
    v.assign(0, &val);
    EXPECT_EQ(*static_cast<int*>(copy[0]), 0);
}

TEST(SmallVector, CopyCtorDynamic)
{
    auto v = make_int_sv({});
    for(int i = 0; i < 64; ++i)
        v.push_back(&i);
    ASSERT_GT(v.capacity(), v.static_capacity());
    std::size_t orig_cap = v.capacity();

    sv_type copy(v);
    ASSERT_EQ(copy.size(), 64);
    EXPECT_EQ(copy.capacity(), orig_cap);
    EXPECT_NE(copy.data(), v.data());
    for(std::size_t i = 0; i < copy.size(); ++i)
    {
        EXPECT_EQ(*static_cast<int*>(copy[i]), static_cast<int>(i))
            << "i = " << i;
    }

    // Modifying the source must not affect the copy
    v.erase(0, 2);
    ASSERT_EQ(v.size(), 62);
    ASSERT_EQ(copy.size(), 64);
    EXPECT_EQ(*static_cast<int*>(copy[0]), 0);
    EXPECT_EQ(*static_cast<int*>(copy[63]), 63);
}

TEST(SmallVector, CopyCtorValueObject)
{
    using namespace asbind20;
    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    auto* m = asbind20::create_module(engine, "test_sv_copy_ctor");
    m->AddScriptSection(
        "test_sv_copy_ctor_helper",
        "class foo\n"
        "{\n"
        "    int data;\n"
        "    foo() { data = 0; }\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* foo_ti = m->GetTypeInfoByDecl("foo");
    ASSERT_THAT(foo_ti, ::testing::NotNull());

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    sv_type v(foo_ti);
    v.emplace_back();
    v.emplace_back();
    ASSERT_EQ(v.size(), 2);

    auto set_data = [](sv_type& vec, std::size_t i, int value)
    {
        auto* obj = static_cast<object_pointer>(vec[i]);
        *static_cast<int*>(obj->GetAddressOfProperty(0)) = value;
    };
    auto get_data = [](sv_type& vec, std::size_t i) -> int
    {
        auto* obj = static_cast<object_pointer>(vec[i]);
        return *static_cast<int*>(obj->GetAddressOfProperty(0));
    };

    set_data(v, 0, 10);
    set_data(v, 1, 20);

    sv_type copy(v);
    ASSERT_EQ(copy.size(), 2);

    // Deep copy: the elements are independent objects
    EXPECT_NE(
        static_cast<object_pointer>(copy[0]),
        static_cast<object_pointer>(v[0])
    );
    EXPECT_EQ(get_data(copy, 0), 10);
    EXPECT_EQ(get_data(copy, 1), 20);

    // Modifying the copy must not affect the original
    set_data(copy, 0, 99);
    EXPECT_EQ(get_data(copy, 0), 99);
    EXPECT_EQ(get_data(v, 0), 10);

    // Both containers release their own objects on destruction
}

TEST(SmallVector, CopyCtorString)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_script_string(engine, true);
    asbind_test::setup_message_callback(engine);

    auto* string_ti = engine->GetTypeInfoByName("string");
    ASSERT_THAT(string_ti, ::testing::NotNull());

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    sv_type v(string_ti);
    {
        std::string s = "hello";
        v.push_back(&s);
    }
    {
        std::string s = "world";
        v.push_back(&s);
    }
    EXPECT_EQ(v.size(), 2);

    sv_type copy(v);
    EXPECT_EQ(copy.size(), 2);
    EXPECT_EQ(*static_cast<std::string*>(copy[0]), "hello");
    EXPECT_EQ(*static_cast<std::string*>(copy[1]), "world");

    // The copy holds independent string objects
    EXPECT_NE(
        static_cast<std::string*>(copy[0]),
        static_cast<std::string*>(v[0])
    );
}

TEST(SmallVector, CopyCtorHandle)
{
    using namespace asbind20;
    using test_container::sv_ref_foo;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_container::register_sv_ref_foo(engine);

    int sv_ref_foo_handle_tid = engine->GetTypeIdByDecl("sv_ref_foo@");
    ASSERT_PRED1(&is_objhandle, sv_ref_foo_handle_tid);

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    const int baseline = test_container::sv_ref_foo::live_count;
    {
        SCOPED_TRACE("baseline live_count = " + std::to_string(baseline));

        sv_type v(engine, sv_ref_foo_handle_tid);

        for(int val : {10, 13})
        {
            sv_ref_foo* obj = new sv_ref_foo;
            obj->data = val;
            v.push_back(&obj);
            obj->release();
        }
        ASSERT_EQ(v.size(), 2);
        EXPECT_EQ(sv_ref_foo::live_count, baseline + 2);

        // After copying, both containers share the same objects
        sv_type copy(v);
        EXPECT_EQ(copy.size(), 2);
        EXPECT_EQ(sv_ref_foo::live_count, baseline + 2);

        // The handles in the copy point to the same objects as the source
        auto* obj_a = *static_cast<sv_ref_foo**>(copy[0]);
        auto* obj_b = *static_cast<sv_ref_foo**>(copy[1]);
        ASSERT_THAT(obj_a, ::testing::NotNull());
        ASSERT_THAT(obj_b, ::testing::NotNull());
        EXPECT_EQ(obj_a->data, 10);
        EXPECT_EQ(obj_b->data, 13);

        // Each container holds one reference
        EXPECT_EQ(obj_a->use_count(), 2);
        EXPECT_EQ(obj_b->use_count(), 2);
    }

    EXPECT_EQ(sv_ref_foo::live_count, baseline)
        << "All references should be released on destruction";
}
