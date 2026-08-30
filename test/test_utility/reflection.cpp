#include <asbind_test/framework.hpp>
#include <asbind20/meta/reflection.hpp>

#ifdef ASBIND20_HAS_LIB_REFLECTION

#    if defined(__GNUC__) && !defined(__clang__)
// False positive if functions are only used in reflection
#        pragma GCC diagnostic ignored "-Wunused-function"
#    endif

namespace
{
int func0()
{
    return 0;
}

int func1(std::int8_t arg0, float arg1)
{
    (void)arg0;
    (void)arg1;
    return 0;
}

unsigned int& func2(const std::int8_t& arg0)
{
    (void)arg0;
    std::terminate();
}
} // namespace

TEST(Reflection, FuncSig)
{
    EXPECT_EQ(
        asbind20::meta::refl_function_sig<^^func0>(),
        "int func0()"
    );
    EXPECT_EQ(
        asbind20::meta::refl_function_sig<^^func1>(),
        "int func1(int8 arg0,float arg1)"
    );
    EXPECT_EQ(
        asbind20::meta::refl_function_sig<^^func2>(),
        "uint& func2(const int8&in arg0)"
    );
}

namespace
{
int helper(int)
{
    return 1013;
}
} // namespace

TEST(Reflection, Proxy)
{
    using asbind20::reflect;

    {
        auto proxy = reflect<^^helper>();
        EXPECT_EQ(
            proxy.get_decl(),
            "int helper(int)"
        );
        EXPECT_EQ(
            proxy.get_func(),
            &helper
        );
        EXPECT_EQ(
            std::invoke(proxy.get_func(), 0),
            1013
        );
    }
}

#endif
