#include "asbind20/bind.hpp"
#include "asbind20/invoke.hpp"
#include "asbind20/utility.hpp"
#include <angelscript.h>
#include <cstdint>
#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/exec.hpp>

#ifdef __GNUC__

// Ignoring "ISO C++ does not support '__int128' for 'type name'"
#    pragma GCC diagnostic ignored "-Wpedantic"

namespace test_bind
{
using int128_t = __int128;
using uint128_t = unsigned __int128;

template <bool UseGeneric>
void register_int128(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    constexpr AS_NAMESPACE_QUALIFIER asQWORD flags =
        AS_NAMESPACE_QUALIFIER asOBJ_POD |
        AS_NAMESPACE_QUALIFIER asOBJ_APP_PRIMITIVE;

    auto i128 = value_class<int128_t, UseGeneric>(
        engine, "int128", flags
    );
    i128
        .template constructor<std::int64_t>("int64")
        .behaviours_by_traits(flags)
        .opEquals()
        .opAdd()
        .opAddAssign()
        .opSub()
        .opSubAssign()
        .opMul()
        .opMulAssign()
        .opDiv()
        .opDivAssign()
        .opNeg()
        .template opImplConv<std::int64_t>();

    auto u128 = value_class<uint128_t, UseGeneric>(
        engine, "uint128", flags
    );
    u128
        .template constructor<std::uint64_t>("uint64")
        .behaviours_by_traits(flags)
        .opEquals()
        .opAdd()
        .opAddAssign()
        .opSub()
        .opSubAssign()
        .opMul()
        .opMulAssign()
        .opDiv()
        .opDivAssign()
        .opNeg()
        .template opImplConv<std::uint64_t>();

    i128.template opImplConv<uint128_t>(u128.get_name());
    u128.template opImplConv<int128_t>(i128.get_name());
}

static void check_int128(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "check_int128", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );

    m->AddScriptSection(
        "check_int128",
        "int128 get_i128() { return -int128(42);}\n"
        "uint128 get_u128() { return uint128(1013);}"
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
}
} // namespace test_bind

TEST(test_bind, builtin_int128_type_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_int128<false>(engine);
    test_bind::check_int128(engine);
}

TEST(test_bind, builtin_int128_type_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_int128<true>(engine);
    test_bind::check_int128(engine);
}

#endif
