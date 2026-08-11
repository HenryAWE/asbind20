// Tests for small_vector initialization-list construction and the
// typeinfo_subtype type information policy.

#include <asbind_test/framework.hpp>
#include <asbind_test/array.hpp>
#include <asbind20/container/small_vector.hpp>
#include <gmock/gmock.h>

namespace
{
using namespace asbind20;

enum class my_enum : compat::script_enum_value_type
{
    zero = 0,
    one = 1,
    two = 2,
    three = 3
};

void setup_my_enum(engine_pointer engine)
{
    enum_<my_enum, std::underlying_type_t<my_enum>>(
        engine, "my_enum"
    )
        .value("zero", my_enum::zero)
        .value("one", my_enum::one)
        .value("two", my_enum::two)
        .value("three", my_enum::three);
}
} // namespace

TEST(SmallVector, InitListEnum)
{
    using namespace asbind20;
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();
    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    setup_my_enum(engine);

    auto* enum_ti = engine->GetTypeInfoByName("my_enum");
    ASSERT_THAT(enum_ti, ::testing::NotNull());

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    using enum_value = compat::script_enum_value_type;

    // Manually simulate AngelScript's initialization list layout
    alignas(std::max_align_t) std::byte buf
        [sizeof(AS_NAMESPACE_QUALIFIER asUINT) + 3 * sizeof(enum_value)];
    *reinterpret_cast<AS_NAMESPACE_QUALIFIER asUINT*>(buf) = 3;
    auto* data = reinterpret_cast<enum_value*>(buf + sizeof(AS_NAMESPACE_QUALIFIER asUINT));
    data[0] = static_cast<enum_value>(my_enum::one);
    data[1] = static_cast<enum_value>(my_enum::two);
    data[2] = static_cast<enum_value>(my_enum::three);

    sv_type v(enum_ti, script_init_list_repeat(buf));
    ASSERT_EQ(v.size(), 3);
    EXPECT_EQ(
        *static_cast<enum_value*>(v[0]),
        static_cast<enum_value>(my_enum::one)
    );
    EXPECT_EQ(
        *static_cast<enum_value*>(v[1]),
        static_cast<enum_value>(my_enum::two)
    );
    EXPECT_EQ(
        *static_cast<enum_value*>(v[2]),
        static_cast<enum_value>(my_enum::three)
    );
}

TEST(SmallVector, InitListString)
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

    // Manually simulate AngelScript's initialization list layout:
    // Build an initialization list of two std::string objects.
    // Buffer layout: [asUINT size][string body 0][string body 1]
    alignas(std::max_align_t) std::byte buf
        [sizeof(AS_NAMESPACE_QUALIFIER asUINT) + 2 * sizeof(std::string)];
    *reinterpret_cast<AS_NAMESPACE_QUALIFIER asUINT*>(buf) = 2;
    auto* body = buf + sizeof(AS_NAMESPACE_QUALIFIER asUINT);
    new(body) std::string("hello");
    new(body + sizeof(std::string)) std::string("world");

    {
        sv_type v(string_ti, script_init_list_repeat(buf));
        ASSERT_EQ(v.size(), 2);
        EXPECT_EQ(*static_cast<std::string*>(v[0]), "hello");
        EXPECT_EQ(*static_cast<std::string*>(v[1]), "world");
    }

    // Destroy the strings kept in the buffer
    std::destroy_n(
        static_cast<std::string*>(static_cast<void*>(body)),
        2
    );
}

TEST(SmallVector, TypeInfoSubtype)
{
    using namespace asbind20;
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();
    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    asbind_test::setup_script_string(engine, true);
    asbind_test::register_script_array(engine, true, false);

    auto* m = asbind20::create_module(engine, "test_sv_subtype");
    m->AddScriptSection(
        "test_sv_subtype_helper",
        "array<int> global_arr;"
    );
    ASSERT_GE(m->Build(), 0);

    auto* arr_ti = m->GetTypeInfoByDecl("array<int>");
    ASSERT_THAT(arr_ti, ::testing::NotNull());
    const int elem_id = arr_ti->GetSubTypeId(0);
    ASSERT_EQ(elem_id, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);

    using sv_type = container::small_vector<
        container::typeinfo_subtype<0>,
        4 * sizeof(void*),
        std::allocator<void>>;

    sv_type v(arr_ti);
    EXPECT_EQ(v.get_type_info(), arr_ti);
    EXPECT_EQ(v.element_type_id(), elem_id);
    // Note: for a template instance AngelScript may return a null typeinfo
    // for GetSubType() even though GetSubTypeId() is valid.
    EXPECT_EQ(v.element_type_info(), arr_ti->GetSubType(0));

    int val = 42;
    v.push_back(&val);
    ASSERT_EQ(v.size(), 1);
    EXPECT_EQ(*static_cast<int*>(v[0]), 42);

    int val2 = 43;
    v.assign(v.begin(), &val2);
    EXPECT_EQ(*static_cast<int*>(v[0]), 43);
}
