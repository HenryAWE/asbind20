#include <asbind20/ext/math.hpp>
#include <cmath>
#include <limits>
#include <numbers>
#include <type_traits>

namespace asbind20::ext
{
template <typename T>
requires std::is_fundamental_v<T>
static T script_math_min(T a, T b)
{
    return std::min(a, b);
}

template <typename T>
requires std::is_fundamental_v<T>
static T script_math_max(T a, T b)
{
    return std::max(a, b);
}

template <typename T>
static constexpr T constant_INFINITY = std::numeric_limits<T>::infinity();

template <typename T>
static bool script_close_to_simple(float a, float b)
{
    return close_to<T>(a, b, std::numeric_limits<T>::epsilon());
}

static bool script_isfinite(float x)
{
    return std::isfinite(x);
}

static bool script_isnan(float x)
{
    return std::isnan(x);
}

static bool script_isinf(float x)
{
    return std::isinf(x);
}

void register_math_function(asIScriptEngine* engine, bool disable_double)
{
    global(engine)
        .function("int min(int a, int b)", &script_math_min<int>)
        .function("int max(int a, int b)", &script_math_max<int>)
        .function("int abs(int x)", static_cast<int (*)(int)>(&std::abs));

#define ASBIND20_EXT_MATH_UNARY_FUNC(name) \
    function("float " #name "(float x)", static_cast<float (*)(float)>(&std::name))

#define ASBIND20_EXT_MATH_BINARY_FUNC(name) \
    function("float " #name "(float x, float y)", static_cast<float (*)(float, float)>(&std::name))


    global(engine)
        .property("const float PI_f", std::numbers::pi_v<float>)
        .property("const float E_f", std::numbers::e_v<float>)
        .property("const float INFINITY_f", constant_INFINITY<float>)
        .function("bool close_to(float a, float b)", &script_close_to_simple<float>)
        .function("bool close_to(float a, float b, float epsilon)", &close_to<float>)
        .function("float min(float a, float b)", &script_math_min<float>)
        .function("float max(float a, float b)", &script_math_min<float>)
        .ASBIND20_EXT_MATH_UNARY_FUNC(ceil)
        .ASBIND20_EXT_MATH_UNARY_FUNC(floor)
        .ASBIND20_EXT_MATH_UNARY_FUNC(round)
        .ASBIND20_EXT_MATH_UNARY_FUNC(trunc)
        .ASBIND20_EXT_MATH_UNARY_FUNC(abs)
        .ASBIND20_EXT_MATH_UNARY_FUNC(sqrt)
        .ASBIND20_EXT_MATH_UNARY_FUNC(cbrt)
        .ASBIND20_EXT_MATH_UNARY_FUNC(exp)
        .ASBIND20_EXT_MATH_UNARY_FUNC(exp2)
        .ASBIND20_EXT_MATH_UNARY_FUNC(log)
        .ASBIND20_EXT_MATH_UNARY_FUNC(log2)
        .ASBIND20_EXT_MATH_UNARY_FUNC(log10)
        .ASBIND20_EXT_MATH_UNARY_FUNC(cos)
        .ASBIND20_EXT_MATH_UNARY_FUNC(sin)
        .ASBIND20_EXT_MATH_UNARY_FUNC(tan)
        .ASBIND20_EXT_MATH_UNARY_FUNC(acos)
        .ASBIND20_EXT_MATH_UNARY_FUNC(asin)
        .ASBIND20_EXT_MATH_UNARY_FUNC(atan)
        .ASBIND20_EXT_MATH_UNARY_FUNC(cosh)
        .ASBIND20_EXT_MATH_UNARY_FUNC(sinh)
        .ASBIND20_EXT_MATH_UNARY_FUNC(tanh)
        .ASBIND20_EXT_MATH_UNARY_FUNC(acosh)
        .ASBIND20_EXT_MATH_UNARY_FUNC(asinh)
        .ASBIND20_EXT_MATH_UNARY_FUNC(atanh)
        .function("bool isfinite(float x)", &script_isfinite)
        .function("bool isnan(float x)", &script_isnan)
        .function("bool isinf(float x)", &script_isinf)
        .ASBIND20_EXT_MATH_BINARY_FUNC(pow)
        .ASBIND20_EXT_MATH_BINARY_FUNC(atan2)
        .ASBIND20_EXT_MATH_BINARY_FUNC(hypot);

    // TODO: double support
    if(disable_double)
        return;

#undef ASBIND20_EXT_MATH_UNARY_FUNC
#undef ASBIND20_EXT_MATH_BINARY_FUNC
}
} // namespace asbind20::ext
