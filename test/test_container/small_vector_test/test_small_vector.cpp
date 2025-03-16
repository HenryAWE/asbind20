#include <asbind20/asbind.hpp>
#include <asbind20/container/small_vector.hpp>
#include <shared_test_lib.hpp>
#include <gtest/gtest.h>
#include <asbind20/ext/stdstring.hpp>

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
    v.shrink_to_fit();
    EXPECT_GE(v.capacity(), v.static_capacity());
    EXPECT_FALSE(v.empty());
    ASSERT_EQ(v.size(), 1);
    EXPECT_EQ(*(int*)v[0], 1013);
    v.pop_back();
    EXPECT_EQ(v.size(), 0);
    EXPECT_TRUE(v.empty());

    for(int i = 0; i < 64; ++i)
    {
        insert_helper(v.begin(), i);
        EXPECT_EQ(*(int*)v[0], i);
        if(i != 0)
        {
            EXPECT_EQ(*(int*)v[1], i - 1);
        }
    }
    ASSERT_EQ(v.size(), 64);
    for(int i = 0; i < 64; ++i)
    {
        ASSERT_NE(v[i], nullptr) << "i = " << i;
        EXPECT_EQ(*(int*)v[i], 64 - (i + 1)) << "i = " << i;
    }
    v.clear();
    EXPECT_TRUE(v.empty());

    for(int i = 0; i < 128; ++i)
        push_back_helper(i);
    EXPECT_GE(v.capacity(), 128);

    ASSERT_EQ(v.size(), 128);
    for(int i = 0; i < 128; ++i)
        EXPECT_EQ(*(int*)v[i], i);

    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), v.size());
    ASSERT_EQ(v.size(), 128);
    for(int i = 0; i < 128; ++i)
        EXPECT_EQ(*(int*)v[i], i);

    v.clear();
    EXPECT_TRUE(v.empty());
    EXPECT_GE(v.capacity(), 128);
    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), v.static_capacity());

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

    {
        counter = 1013;
        auto* special_foo = (AS_NAMESPACE_QUALIFIER asIScriptObject*)engine->CreateScriptObject(foo_ti);
        ASSERT_NE(special_foo, nullptr);
        EXPECT_EQ(*(int*)special_foo->GetAddressOfProperty(0), 1013);
        EXPECT_EQ(counter, 1013 + 1);

        counter = 0;
        sv_type v(foo_ti);
        for(int i = 0; i < 10; ++i)
            v.emplace_back();
        EXPECT_EQ(v.size(), 10);
        EXPECT_EQ(counter, 10);
        v.insert(v.begin(), special_foo);
        special_foo->Release();
        special_foo = nullptr;
        EXPECT_EQ(v.size(), 11);

        auto expect_member_data_at = [&v](std::size_t i, int expected)
        {
            SCOPED_TRACE(string_concat("v[i] is v[", std::to_string(i), ']'));

            auto* obj = (AS_NAMESPACE_QUALIFIER asIScriptObject*)v[i];
            ASSERT_NE(obj, nullptr);
            int* data = (int*)obj->GetAddressOfProperty(0);
            ASSERT_NE(data, nullptr);

            EXPECT_EQ(*data, expected);
        };

        expect_member_data_at(0, 1013);
        for(std::size_t i = 1; i < v.size(); ++i)
            expect_member_data_at(i, static_cast<int>(i - 1));

        counter = -1;
        special_foo = (AS_NAMESPACE_QUALIFIER asIScriptObject*)engine->CreateScriptObject(foo_ti);
        ASSERT_NE(special_foo, nullptr);
        EXPECT_EQ(counter, -1 + 1);

        v.insert(v.begin() + 1, special_foo);
        special_foo->Release();
        special_foo = nullptr;
        EXPECT_EQ(v.size(), 12);

        expect_member_data_at(0, 1013);
        expect_member_data_at(1, -1);
        for(std::size_t i = 2; i < v.size(); ++i)
            expect_member_data_at(i, static_cast<int>(i - 2));
    }

    counter = 0;
    {
        int foo_handle_id = m->GetTypeIdByDecl("foo@");
        ASSERT_TRUE(is_objhandle(foo_handle_id));

        sv_type v(engine, foo_handle_id);
        EXPECT_TRUE(is_objhandle(v.element_type_id()));

        for(int i = 0; i < 10; ++i)
            v.emplace_back();
        ASSERT_EQ(v.size(), 10);
        EXPECT_EQ(counter, 0); // all handles are initialized by null

        for(std::size_t i = 0; i < v.size(); ++i)
        {
            void* handle = *(void**)v[0];
            EXPECT_EQ(handle, nullptr);
        }

        auto range_visitor = []<typename T>(T* start, T* stop)
        {
            if constexpr(std::same_as<T, void*>)
            {
                for(void** it = start; it != stop; ++it)
                {
                    void* handle = *it;
                    EXPECT_EQ(handle, nullptr);
                }
            }
            else
                FAIL() << "unreachable";
        };

        v.visit(
            range_visitor,
            v.begin(),
            v.end()
        );
        v.visit(
            range_visitor,
            0,
            v.size()
        );
    }
}

TEST(small_vector, script_string)
{
    using namespace asbind20;
    auto engine = make_script_engine();

    ext::register_std_string(engine, true);
    asbind_test::setup_message_callback(engine, true);

    AS_NAMESPACE_QUALIFIER asITypeInfo* string_ti = engine->GetTypeInfoByName("string");
    ASSERT_NE(string_ti, nullptr);

    // Use std::allocator for debugging
    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    sv_type v(string_ti);
    v.emplace_back();

    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(((std::string*)v[0])->size(), 0);
    EXPECT_EQ(*(std::string*)v[0], "");

    {
        std::string str = "hello";
        v.push_back(&str);
    }
    EXPECT_EQ(v.size(), 2);
    EXPECT_EQ(((std::string*)v[1])->size(), 5);
    EXPECT_EQ(*(std::string*)v[1], "hello");

    v.reserve(128);
    EXPECT_EQ(v.size(), 2);
    EXPECT_EQ(((std::string*)v[0])->size(), 0);
    EXPECT_EQ(*(std::string*)v[0], "");
    EXPECT_EQ(((std::string*)v[1])->size(), 5);
    EXPECT_EQ(*(std::string*)v[1], "hello");
}
