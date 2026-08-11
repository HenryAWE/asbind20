// Tests for small_vector copy construction.
// Copying is deep: the copy owns its own storage independent from the source.

#include <atomic>
#include <asbind_test/framework.hpp>
#include <asbind20/container/small_vector.hpp>
#include <gmock/gmock.h>
#include "sv_helper.hpp"

namespace
{
using namespace asbind20;

using sv_type = test_container::int_sv_type;
using test_container::make_int_sv;
} // namespace

TEST(SmallVector, CopyCtorEmpty)
{
    auto v = make_int_sv({});
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
        SCOPED_TRACE(string_concat("i = ", std::to_string(i)));
        EXPECT_EQ(*static_cast<int*>(copy[i]), static_cast<int>(i));
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
    ASSERT_EQ(v.size(), 2);

    sv_type copy(v);
    ASSERT_EQ(copy.size(), 2);
    EXPECT_EQ(*static_cast<std::string*>(copy[0]), "hello");
    EXPECT_EQ(*static_cast<std::string*>(copy[1]), "world");

    // The copy holds independent string objects
    EXPECT_NE(
        static_cast<std::string*>(copy[0]),
        static_cast<std::string*>(v[0])
    );
}

namespace
{
class copy_ref_foo
{
public:
    using counter_type = AS_NAMESPACE_QUALIFIER asUINT;

    copy_ref_foo()
        : data(0)
    {
        ++live_count;
    }

    ~copy_ref_foo()
    {
        --live_count;
    }

    void addref()
    {
        m_use_count += 1;
    }

    void release()
    {
        ASSERT_NE(m_use_count, 0);
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

    inline static std::atomic<int> live_count;

private:
    counter_type m_use_count = 1;
};

void register_copy_ref_foo(engine_pointer engine)
{
    ref_class<copy_ref_foo> c(
        engine, "copy_ref_foo", AS_NAMESPACE_QUALIFIER asOBJ_GC
    );
    c.default_factory()
        .addref(fp<&copy_ref_foo::addref>)
        .release(fp<&copy_ref_foo::release>)
        .set_gc_flag(fp<&copy_ref_foo::set_gc_flag>)
        .get_gc_flag(fp<&copy_ref_foo::get_gc_flag>)
        .enum_refs(fp<&copy_ref_foo::enum_refs>)
        .release_refs(fp<&copy_ref_foo::release_refs>)
        .method("uint use_count() const", fp<&copy_ref_foo::use_count>)
        .property("int data", &copy_ref_foo::data);
}
} // namespace

TEST(SmallVector, CopyCtorHandle)
{
    using namespace asbind20;
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();
    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    register_copy_ref_foo(engine);

    int foo_handle_id = engine->GetTypeIdByDecl("copy_ref_foo@");
    ASSERT_TRUE(is_objhandle(foo_handle_id));

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    const int baseline = copy_ref_foo::live_count;
    {
        sv_type v(engine, foo_handle_id);

        for(int i = 0; i < 2; ++i)
        {
            copy_ref_foo* obj = new copy_ref_foo;
            obj->data = i;
            v.push_back(&obj);
            obj->release();
        }
        ASSERT_EQ(v.size(), 2);
        EXPECT_EQ(copy_ref_foo::live_count, baseline + 2);

        // After copying, both containers share the same objects
        sv_type copy(v);
        ASSERT_EQ(copy.size(), 2);
        EXPECT_EQ(copy_ref_foo::live_count, baseline + 2);

        // The handles in the copy point to the same objects as the source
        auto* obj_a = *static_cast<copy_ref_foo**>(copy[0]);
        auto* obj_b = *static_cast<copy_ref_foo**>(copy[1]);
        ASSERT_THAT(obj_a, ::testing::NotNull());
        ASSERT_THAT(obj_b, ::testing::NotNull());
        EXPECT_EQ(obj_a->data, 0);
        EXPECT_EQ(obj_b->data, 1);

        // Each container holds one reference
        EXPECT_EQ(obj_a->use_count(), 2); // v + copy
        EXPECT_EQ(obj_b->use_count(), 2);
    }
    // All references released on destruction
    EXPECT_EQ(copy_ref_foo::live_count, baseline);
}
