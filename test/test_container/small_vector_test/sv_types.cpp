// Tests for small_vector with different element types: bool, float, double,
// enum, and the default script_allocator.

#include <asbind_test/framework.hpp>
#include <asbind20/container/small_vector.hpp>
#include <gmock/gmock.h>

namespace
{
using namespace asbind20;

enum class my_enum : compat::script_enum_value_type
{
    zero = 0,
    one = 1,
    two = 2
};

void setup_my_enum(engine_pointer engine)
{
    enum_<my_enum, std::underlying_type_t<my_enum>>(
        engine, "my_enum"
    )
        .value("zero", my_enum::zero)
        .value("one", my_enum::one)
        .value("two", my_enum::two);
}
} // namespace

TEST(SmallVector, BoolAsElement)
{
    using namespace asbind20;

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    sv_type v(nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_BOOL);
    EXPECT_TRUE(is_bool_type(v.element_type_id()));

    bool a = true, b = false;
    v.push_back(&a);
    v.push_back(&b);
    ASSERT_EQ(v.size(), 2);
    EXPECT_TRUE(*static_cast<bool*>(v[0]));
    EXPECT_FALSE(*static_cast<bool*>(v[1]));

    v.emplace_back();
    ASSERT_EQ(v.size(), 3);
    EXPECT_FALSE(*static_cast<bool*>(v[2])); // value-initialized to false
}

TEST(SmallVector, FloatAsElement)
{
    using namespace asbind20;

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    sv_type v(nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT);
    float val = 3.14f;
    v.push_back(&val);
    v.push_back(&val);
    ASSERT_EQ(v.size(), 2);
    EXPECT_FLOAT_EQ(*static_cast<float*>(v[0]), 3.14f);
    EXPECT_FLOAT_EQ(*static_cast<float*>(v[1]), 3.14f);
}

TEST(SmallVector, DoubleAsElement)
{
    using namespace asbind20;

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    sv_type v(nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_DOUBLE);
    double val = 2.718281828;
    v.push_back(&val);
    ASSERT_EQ(v.size(), 1);
    EXPECT_DOUBLE_EQ(*static_cast<double*>(v[0]), 2.718281828);
}

TEST(SmallVector, EnumAsElement)
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

    sv_type v(engine, enum_ti->GetTypeId());
    EXPECT_TRUE(is_enum_type(v.element_type_id()));

    using enum_value = compat::script_enum_value_type;
    my_enum val = my_enum::one;
    v.push_back(&val);
    ASSERT_EQ(v.size(), 1);
    EXPECT_EQ(
        *static_cast<enum_value*>(v[0]),
        static_cast<enum_value>(my_enum::one)
    );

    // resize() zero-initializes new elements
    v.resize(3);
    ASSERT_EQ(v.size(), 3);
    EXPECT_EQ(
        *static_cast<enum_value*>(v[0]),
        static_cast<enum_value>(my_enum::one)
    );
    EXPECT_EQ(
        *static_cast<enum_value*>(v[1]),
        static_cast<enum_value>(my_enum::zero)
    );
    EXPECT_EQ(
        *static_cast<enum_value*>(v[2]),
        static_cast<enum_value>(my_enum::zero)
    );
}

TEST(SmallVector, DefaultScriptAllocator)
{
    using namespace asbind20;

    // Default Allocator = script_allocator<void>
    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*)>;

    sv_type v(nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    for(int i = 0; i < 64; ++i)
        v.push_back(&i);

    ASSERT_EQ(v.size(), 64);
    for(int i = 0; i < 64; ++i)
        EXPECT_EQ(*static_cast<int*>(v[i]), i);

    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), v.size());
    ASSERT_EQ(v.size(), 64);
    for(int i = 0; i < 64; ++i)
        EXPECT_EQ(*static_cast<int*>(v[i]), i);
}

TEST(SmallVector, Int64AsElement)
{
    using namespace asbind20;

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    sv_type v(nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_INT64);
    std::int64_t val = 123456789012345LL;
    v.push_back(&val);
    v.push_back(&val);
    ASSERT_EQ(v.size(), 2);
    EXPECT_EQ(*static_cast<std::int64_t*>(v[0]), 123456789012345LL);
    EXPECT_EQ(*static_cast<std::int64_t*>(v[1]), 123456789012345LL);
}

TEST(SmallVector, UIntAsElement)
{
    using namespace asbind20;

    using sv_type = container::small_vector<
        container::typeinfo_identity,
        4 * sizeof(void*),
        std::allocator<void>>;

    sv_type v(nullptr, AS_NAMESPACE_QUALIFIER asTYPEID_UINT32);
    std::uint32_t val = 0xFFFFFFFFU;
    v.push_back(&val);
    ASSERT_EQ(v.size(), 1);
    EXPECT_EQ(*static_cast<std::uint32_t*>(v[0]), 0xFFFFFFFFU);
}
