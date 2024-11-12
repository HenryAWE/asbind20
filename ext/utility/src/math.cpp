#include <asbind20/ext/math.hpp>
#include <cmath>
#include <limits>
#include <numbers>
#include <type_traits>

namespace asbind20::ext
{
template <typename T>
static constexpr T constant_INFINITY = std::numeric_limits<T>::infinity();

template <typename T>
static constexpr T constant_NAN = std::numeric_limits<T>::quiet_NaN();

void register_math_constants(asIScriptEngine* engine, const char* ns)
{
    namespace_ ns_guard(engine, ns);

    global g(engine);

#define ASBIND20_EXT_MATH_REGISTER_CONSTANTS(register_class, float_t, suffix)                    \
    register_class                                                                               \
        .property("const float PI" suffix, std::numbers::pi_v<float_t>)                          \
        .property("const float E" suffix, std::numbers::e_v<float_t>)                            \
        .property("const float PHI" suffix, std::numbers::phi_v<float_t>) /* The golden ratio */ \
        .property("const float NAN" suffix, constant_NAN<float_t>)                               \
        .property("const float INFINITY" suffix, constant_INFINITY<float_t>)

    ASBIND20_EXT_MATH_REGISTER_CONSTANTS(g, float, "_f");
    ASBIND20_EXT_MATH_REGISTER_CONSTANTS(g, double, "_d");
}

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
requires std::is_fundamental_v<T>
static T script_math_clamp(T val, T a, T b)
{
    return std::clamp(val, a, b);
}

template <typename T>
requires std::is_fundamental_v<T>
static T script_math_lerp(T a, T b, T t)
{
    return std::lerp(a, b, t);
}

template <typename T>
static bool script_close_to_simple(float a, float b)
{
    return close_to<T>(a, b, std::numeric_limits<T>::epsilon());
}

// Using following wrappers to avoid ICE on MSVC
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

template <bool UseGeneric>
void register_math_function_impl(asIScriptEngine* engine)
{
    global_t<UseGeneric> g(engine);

    g
        .template function<&script_math_min<int>>("int min(int a, int b)")
        .template function<&script_math_max<int>>("int max(int a, int b)")
        .template function<&script_math_clamp<int>>("int clamp(int val, int a, int b)")
        .template function<static_cast<int (*)(int)>(&std::abs)>("int abs(int x)");

#define ASBIND20_EXT_MATH_UNARY_FUNC(name, float_t) \
    template function<static_cast<float_t (*)(float_t)>(&std::name)>(#float_t " " #name "(" #float_t " x)")

#define ASBIND20_EXT_MATH_BINARY_FUNC(name, float_t) \
    template function<static_cast<float_t (*)(float_t, float_t)>(&(std::name))>(#float_t " " #name "(" #float_t " x, " #float_t " y)")

#define ASBIND20_EXT_MATH_REGISTER_FUNCS(register_class, float_t)                                                            \
    register_class                                                                                                           \
        .template function<&script_close_to_simple<float_t>>("bool close_to(" #float_t " a, " #float_t " b)")                \
        .template function<&close_to<float_t>>("bool close_to(" #float_t " a, " #float_t " b, " #float_t " epsilon)")        \
        .template function<&script_math_min<float_t>>(#float_t " min(" #float_t " a, " #float_t " b)")                       \
        .template function<&script_math_max<float_t>>(#float_t " max(" #float_t " a, " #float_t " b)")                       \
        .template function<&script_math_clamp<float_t>>(#float_t " clamp(" #float_t " val, " #float_t " a, " #float_t " b)") \
        .template function<&script_math_lerp<float_t>>(#float_t " lerp(" #float_t " a, " #float_t " b, " #float_t " t)")     \
        .ASBIND20_EXT_MATH_UNARY_FUNC(ceil, float_t)                                                                         \
        .ASBIND20_EXT_MATH_UNARY_FUNC(floor, float_t)                                                                        \
        .ASBIND20_EXT_MATH_UNARY_FUNC(round, float_t)                                                                        \
        .ASBIND20_EXT_MATH_UNARY_FUNC(trunc, float_t)                                                                        \
        .ASBIND20_EXT_MATH_UNARY_FUNC(abs, float_t)                                                                          \
        .ASBIND20_EXT_MATH_UNARY_FUNC(sqrt, float_t)                                                                         \
        .ASBIND20_EXT_MATH_UNARY_FUNC(cbrt, float_t)                                                                         \
        .ASBIND20_EXT_MATH_UNARY_FUNC(exp, float_t)                                                                          \
        .ASBIND20_EXT_MATH_UNARY_FUNC(exp2, float_t)                                                                         \
        .ASBIND20_EXT_MATH_UNARY_FUNC(log, float_t)                                                                          \
        .ASBIND20_EXT_MATH_UNARY_FUNC(log2, float_t)                                                                         \
        .ASBIND20_EXT_MATH_UNARY_FUNC(log10, float_t)                                                                        \
        .ASBIND20_EXT_MATH_UNARY_FUNC(cos, float_t)                                                                          \
        .ASBIND20_EXT_MATH_UNARY_FUNC(sin, float_t)                                                                          \
        .ASBIND20_EXT_MATH_UNARY_FUNC(tan, float_t)                                                                          \
        .ASBIND20_EXT_MATH_UNARY_FUNC(acos, float_t)                                                                         \
        .ASBIND20_EXT_MATH_UNARY_FUNC(asin, float_t)                                                                         \
        .ASBIND20_EXT_MATH_UNARY_FUNC(atan, float_t)                                                                         \
        .ASBIND20_EXT_MATH_UNARY_FUNC(cosh, float_t)                                                                         \
        .ASBIND20_EXT_MATH_UNARY_FUNC(sinh, float_t)                                                                         \
        .ASBIND20_EXT_MATH_UNARY_FUNC(tanh, float_t)                                                                         \
        .ASBIND20_EXT_MATH_UNARY_FUNC(acosh, float_t)                                                                        \
        .ASBIND20_EXT_MATH_UNARY_FUNC(asinh, float_t)                                                                        \
        .ASBIND20_EXT_MATH_UNARY_FUNC(atanh, float_t)                                                                        \
        .template function<&script_isfinite>("bool isfinite(" #float_t " x)")                                                \
        .template function<&script_isnan>("bool isnan(" #float_t " x)")                                                      \
        .template function<&script_isinf>("bool isinf(" #float_t " x)")                                                      \
        .ASBIND20_EXT_MATH_BINARY_FUNC(pow, float_t)                                                                         \
        .ASBIND20_EXT_MATH_BINARY_FUNC(atan2, float_t)                                                                       \
        .ASBIND20_EXT_MATH_BINARY_FUNC(hypot, float_t)

    ASBIND20_EXT_MATH_REGISTER_FUNCS(g, float);
    ASBIND20_EXT_MATH_REGISTER_FUNCS(g, double);

#undef ASBIND20_EXT_MATH_UNARY_FUNC
#undef ASBIND20_EXT_MATH_BINARY_FUNC

#undef ASBIND20_EXT_MATH_REGISTER_FUNCS
}

void register_math_function(asIScriptEngine* engine, bool generic)
{
    if(generic)
        register_math_function_impl<true>(engine);
    else
        register_math_function_impl<false>(engine);
}
} // namespace asbind20::ext
