#include <cstdint>
#include <gtest/gtest.h>
#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>

#ifdef __GNUC__
namespace test_bind
{
// Ignoring "ISO C++ does not support '__int128' for 'type name'"
#    pragma GCC diagnostic ignored "-Wpedantic"

using int128_t = __int128;
using uint128_t = unsigned __int128;

#    define ASBIND20_TEST_HAS_INT128           1
#    define ASBIND20_TEST_HAS_PRIMITIVE_INT128 1
} // namespace test_bind

#elif defined _MSC_VER

#    include <__msvc_int128.hpp>

namespace test_bind
{
using int128_t = std::_Signed128;
using uint128_t = std::_Unsigned128;

#    define ASBIND20_TEST_HAS_INT128 1
} // namespace test_bind

#endif

#ifdef ASBIND20_TEST_HAS_INT128

namespace test_bind
{
template <bool UseGeneric>
static void register_int128(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    AS_NAMESPACE_QUALIFIER asQWORD flags = AS_NAMESPACE_QUALIFIER asOBJ_POD;
#    ifdef ASBIND20_TEST_HAS_PRIMITIVE_INT128
    flags |= AS_NAMESPACE_QUALIFIER asOBJ_APP_PRIMITIVE;
#    else
    flags |= AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS;
#    endif

    auto i128 = value_class<int128_t, UseGeneric>(
        engine, "int128", flags
    );
    i128
        .template constructor<std::int64_t>("int64")
        .behaviours_by_traits(flags)
        .opEquals()
        .opCmp()
        .opAdd()
        .opAddAssign()
        .opSub()
        .opSubAssign()
        .opMul()
        .opMulAssign()
        .opDiv()
        .opDivAssign()
        .opMod()
        .opModAssign()
        .opNeg()
        .template opImplConv<std::int64_t>();

    auto u128 = value_class<uint128_t, UseGeneric>(
        engine, "uint128", flags
    );
    u128
        .template constructor<std::uint64_t>("uint64")
        .behaviours_by_traits(flags)
        .opEquals()
        .opCmp()
        .opAdd()
        .opAddAssign()
        .opSub()
        .opSubAssign()
        .opMul()
        .opMulAssign()
        .opDiv()
        .opDivAssign()
        .opMod()
        .opModAssign()
        .opNeg()
        .template opImplConv<std::uint64_t>();

    i128.opImplConv(u128);
    u128.opImplConv(i128);
}

static void check_int128(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "check_int128", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );

    m->AddScriptSection(
        "check_int128",
        "int128 get_i128() { return -int128(42); }\n"
        "uint128 get_u128() { return uint128(1013); }\n"
        "uint128 add_u128(uint128 lhs, uint128 rhs)\n"
        "{ return lhs + rhs; }\n"
        "int128 mod_i128(int128 lhs, int128 rhs)\n"
        "{ return lhs % rhs; }\n"
        "uint128 mod_u128(uint128 lhs, uint128 rhs)\n"
        "{ return lhs % rhs; }\n"
        "int128 mod_assign_i128(int128 lhs, int128 rhs)\n"
        "{ lhs %= rhs; return lhs; }\n"
        "uint128 mod_assign_u128(uint128 lhs, uint128 rhs)\n"
        "{ lhs %= rhs; return lhs; }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* f = m->GetFunctionByName("get_i128");
        ASSERT_TRUE(f);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<uint128_t>(ctx, f);

        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), -42);
    }

    {
        auto* f = m->GetFunctionByName("get_u128");
        ASSERT_TRUE(f);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int128_t>(ctx, f);

        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 1013);
    }

    // TODO: Enable this case after generated operators for primitive type are fixed
#    if 0
    {
        auto* f = m->GetFunctionByName("mod_i128");
        ASSERT_TRUE(f);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int128_t>(
            ctx, f, int128_t(42), int128_t(10)
        );

        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 2); // 42 % 10 = 2
    }

    {
        auto* f = m->GetFunctionByName("mod_u128");
        ASSERT_TRUE(f);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<uint128_t>(
            ctx, f, uint128_t(1013), uint128_t(100)
        );

        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 13); // 1013 % 100 = 13
    }

    {
        auto* f = m->GetFunctionByName("mod_assign_i128");
        ASSERT_TRUE(f);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int128_t>(
            ctx, f, int128_t(42), int128_t(10)
        );

        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 2); // 42 %= 10 → 2
    }

    {
        auto* f = m->GetFunctionByName("mod_assign_u128");
        ASSERT_TRUE(f);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<uint128_t>(
            ctx, f, uint128_t(1013), uint128_t(100)
        );

        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 13); // 1013 %= 100 → 13
    }

    {
        auto * f= m->GetFunctionByName("add_u128");
        ASSERT_NE(f, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<uint128_t>(
            ctx, f, uint128_t(40), uint128_t(2)
        );

        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 42);
    }
#    endif
}
} // namespace test_bind

TEST(TestBind, BuiltinInt128TypeNative)
{
    using namespace asbind20;

    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_int128<false>(engine);
    test_bind::check_int128(engine);
}

TEST(TestBind, BuiltinInt128TypeGeneric)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_int128<true>(engine);
    test_bind::check_int128(engine);
}

#endif
