#include <asbind_test/framework.hpp>
#include <asbind20/container/small_vector.hpp>

TEST(SmallVector, EmptyIntSVMoveCtor)
{
    using namespace asbind20;

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*)>;

    sv_type v(
        nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_INT32
    );
    EXPECT_TRUE(v.empty());
    sv_type new_sv = std::move(v);
    EXPECT_TRUE(new_sv.empty());
    EXPECT_NE(v.data(), new_sv.data());
}

TEST(SmallVector, SmallIntSVMoveCtor)
{
    using namespace asbind20;

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*)>;

    sv_type v(
        nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_INT32
    );

    {
        int val = 1013;
        v.push_back(&val);
    }
    EXPECT_EQ(v.size(), 1);

    sv_type new_sv = std::move(v);
    EXPECT_EQ(new_sv.element_type_info(), v.element_type_info());
    EXPECT_EQ(new_sv.element_type_id(), v.element_type_id());

    // NOTE:
    // This is a designed behavior.
    // The content of old container of primitive type remains unchanged
    // if it's not dynamically allocated.
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(*(int*)v[0], 1013);

    EXPECT_EQ(new_sv.size(), 1);
    EXPECT_EQ(*(int*)new_sv[0], 1013);
}

TEST(SmallVector, DynIntSVMoveCtor)
{
    using namespace asbind20;

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*)>;

    sv_type v(
        nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_INT32
    );

    v.reserve(128);
    for(int i = 0; i < 128; ++i)
    {
        v.push_back(&i);
    }
    EXPECT_EQ(v.size(), 128);
    ASSERT_GE(v.capacity(), v.static_capacity());

    sv_type new_sv = std::move(v);
    EXPECT_EQ(new_sv.element_type_info(), v.element_type_info());
    EXPECT_EQ(new_sv.element_type_id(), v.element_type_id());

    EXPECT_EQ(v.size(), 0);
    EXPECT_EQ(v.capacity(), v.static_capacity());

    EXPECT_EQ(new_sv.size(), 128);
    for(int i = 0; i < 128; ++i)
    {
        EXPECT_EQ(*(int*)new_sv[i], i);
    }
}

TEST(SmallVector, EmptyStringSVMoveCtor)
{
    using namespace asbind20;
    auto engine = make_script_engine();

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*)>;

    asbind_test::setup_script_string(engine, true);
    asbind_test::setup_message_callback(engine, true);

    asbind20::typeinfo_pointer string_ti = engine->GetTypeInfoByName("string");
    ASSERT_NE(string_ti, nullptr);

    sv_type v(string_ti);
    EXPECT_TRUE(v.empty());
    sv_type new_sv = std::move(v);
    EXPECT_TRUE(new_sv.empty());
    EXPECT_NE(v.data(), new_sv.data());
}

TEST(SmallVector, SmallStringSVMoveCtor)
{
    using namespace asbind20;
    auto engine = make_script_engine();

    asbind_test::setup_script_string(engine, true);
    asbind_test::setup_message_callback(engine, true);

    asbind20::typeinfo_pointer string_ti = engine->GetTypeInfoByName("string");
    ASSERT_NE(string_ti, nullptr);

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*)>;

    sv_type v(string_ti);

    for(unsigned int i = 0; i < 2; ++i)
    {
        std::string s = std::to_string(i);
        v.push_back(&s);
    }
    EXPECT_EQ(v.size(), 2);
    EXPECT_LE(v.capacity(), v.static_capacity());

    sv_type new_sv = std::move(v);
    EXPECT_EQ(v.size(), 0);
    EXPECT_EQ(new_sv.size(), 2);
    for(unsigned int i = 0; i < 2; ++i)
    {
        const std::string& s = *(std::string*)new_sv[i];
        EXPECT_EQ(s, std::to_string(i));
    }
}

TEST(SmallVector, DynStringSVMoveCtor)
{
    using namespace asbind20;
    auto engine = make_script_engine();

    asbind_test::setup_script_string(engine, true);
    asbind_test::setup_message_callback(engine, true);

    asbind20::typeinfo_pointer string_ti = engine->GetTypeInfoByName("string");
    ASSERT_NE(string_ti, nullptr);

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*)>;

    sv_type v(string_ti);

    for(unsigned int i = 0; i < 16; ++i)
    {
        std::string s = std::to_string(i);
        v.push_back(&s);
    }
    EXPECT_EQ(v.size(), 16);
    EXPECT_GT(v.capacity(), v.static_capacity());

    sv_type new_sv = std::move(v);
    EXPECT_EQ(v.size(), 0);
    EXPECT_EQ(new_sv.size(), 16);
    for(unsigned int i = 0; i < 16; ++i)
    {
        const std::string& s = *(std::string*)new_sv[i];
        EXPECT_EQ(s, std::to_string(i));
    }
}
