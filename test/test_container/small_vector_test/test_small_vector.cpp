#include <asbind20/asbind.hpp>
#include <asbind20/container/small_vector.hpp>
#include <shared_test_lib.hpp>
#include <gtest/gtest.h>

TEST(small_vector, int)
{
    using namespace asbind20;

    // Use std::allocator for debugging
    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    sv_type v(
        nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_INT32
    );

    auto push_back_helper = [&v](int val)
    {
        v.push_back(&val);
    };
    auto insert_helper = [&v](auto where, int val)
    {
        v.insert(std::move(where), &val);
    };

    push_back_helper(1013);
    EXPECT_GE(v.capacity(), v.static_capacity());
    EXPECT_FALSE(v.empty());
    ASSERT_EQ(v.size(), 1);
    EXPECT_EQ(*(int*)v[0], 1013);
    v.pop_back();
    EXPECT_EQ(v.size(), 0);
    EXPECT_TRUE(v.empty());

    for(int i = 0; i < 128; ++i)
        push_back_helper(i);
    EXPECT_GE(v.capacity(), 128);

    ASSERT_EQ(v.size(), 128);
    for(int i = 0; i < 128; ++i)
        EXPECT_EQ(*(int*)v[i], i);

    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_GE(v.capacity(), 128);

    insert_helper(0, 13);
    insert_helper(v.begin(), 10);
    EXPECT_EQ(v.size(), 2);
    EXPECT_EQ(*(int*)v[0], 10);
    EXPECT_EQ(*(int*)v[1], 13);
}

TEST(small_vector, script_object)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    int counter = 0;
    global(engine)
        .function(
            "int counter()",
            [](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
            {
                set_generic_return<int>(
                    gen,
                    get_generic_auxiliary<int>(gen)++
                );
            },
            auxiliary(counter)
        );
    EXPECT_EQ(counter, 0);

    auto* m = engine->GetModule("test_small_vector", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);
    m->AddScriptSection(
        "test_small_vector_helper",
        "class foo\n"
        "{\n"
        "    int data;\n"
        "    foo() { data = counter(); }\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    AS_NAMESPACE_QUALIFIER asITypeInfo* foo_ti = m->GetTypeInfoByDecl("foo");
    ASSERT_NE(foo_ti, nullptr);

    // Use std::allocator for debugging
    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    {
        sv_type v(foo_ti);
        for(int i = 0; i < 10; ++i)
            v.emplace_back();
        ASSERT_EQ(v.size(), 10);
        EXPECT_EQ(counter, 10);

        const auto& cv = std::as_const(v);
        for(auto it = cv.begin(); it != cv.end(); ++it)
        {
            static_assert(
                std::convertible_to<
                    std::iterator_traits<decltype(it)>::iterator_category,
                    std::random_access_iterator_tag>
            );

            auto* obj = (AS_NAMESPACE_QUALIFIER asIScriptObject*)*it;
            ASSERT_NE(obj, nullptr);
            int* data = (int*)obj->GetAddressOfProperty(0);

            ASSERT_NE(data, nullptr);
            EXPECT_EQ(*data, it - cv.begin());
        }

        v.pop_back();
        EXPECT_EQ(v.size(), 9);

        auto expect_member_data_at = [&v](std::size_t i, int expected)
        {
            SCOPED_TRACE(string_concat("v[i] is v[", std::to_string(i), ']'));

            auto* obj = (AS_NAMESPACE_QUALIFIER asIScriptObject*)v[i];
            ASSERT_NE(obj, nullptr);
            int* data = (int*)obj->GetAddressOfProperty(0);
            ASSERT_NE(data, nullptr);

            EXPECT_EQ(*data, expected);
        };

        // Erase [1, 4)
        v.erase(1, 3);
        EXPECT_EQ(v.size(), 6);
        EXPECT_EQ(std::distance(v.begin(), v.end()), 6);
        expect_member_data_at(0, 0);
        expect_member_data_at(1, 4);
        expect_member_data_at(2, 5);

        v.erase(0);
        EXPECT_EQ(v.size(), 5);
        expect_member_data_at(0, 4);
        expect_member_data_at(1, 5);

        // stop > start: should have no effect
        v.erase(v.begin() + 2, v.begin() + 1);
        EXPECT_EQ(v.size(), 5);

        v.erase(v.begin() + 2, v.end());
        EXPECT_EQ(v.size(), 2);
        expect_member_data_at(0, 4);
        expect_member_data_at(1, 5);

        v.erase(v.begin());
        EXPECT_EQ(v.size(), 1);
        expect_member_data_at(0, 5);
    }

    counter = 0;
    {
        int foo_handle_id = m->GetTypeIdByDecl("foo@");
        ASSERT_TRUE(is_objhandle(foo_handle_id));

        sv_type v(engine, foo_handle_id);
        for(int i = 0; i < 10; ++i)
            v.emplace_back();
        ASSERT_EQ(v.size(), 10);
        EXPECT_EQ(counter, 0); // all handles are initialized by null

        for(std::size_t i = 0; i < v.size(); ++i)
        {
            void* handle = *(void**)v[0];
            EXPECT_EQ(handle, nullptr);
        }
    }
}
