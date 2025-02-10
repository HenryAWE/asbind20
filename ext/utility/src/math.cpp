#include <asbind20/ext/math.hpp>
#include <cmath>
#include <limits>
#include <numbers>
#include <complex>
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
    global<UseGeneric> g(engine);

    g
        .function("int min(int a, int b)", fp<&script_math_min<int>>)
        .function("int max(int a, int b)", fp<&script_math_max<int>>)
        .function("int clamp(int val, int a, int b)", fp<&script_math_clamp<int>>)
        .function("int abs(int x)", fp<static_cast<int (*)(int)>(&std::abs)>);

#define ASBIND20_EXT_MATH_UNARY_FUNC(name, float_t) \
    function(#float_t " " #name "(" #float_t " x)", fp<static_cast<float_t (*)(float_t)>(&std::name)>)

#define ASBIND20_EXT_MATH_BINARY_FUNC(name, float_t) \
    function(#float_t " " #name "(" #float_t " x, " #float_t " y)", fp<static_cast<float_t (*)(float_t, float_t)>(&(std::name))>)

#define ASBIND20_EXT_MATH_REGISTER_FUNCS(register_class, float_t)                                                       \
    register_class                                                                                                      \
        .function("bool close_to(" #float_t " a, " #float_t " b)", fp<&script_close_to_simple<float_t>>)                \
        .function("bool close_to(" #float_t " a, " #float_t " b, " #float_t " epsilon)", fp<&close_to<float_t>>)        \
        .function(#float_t " min(" #float_t " a, " #float_t " b)", fp<&script_math_min<float_t>>)                       \
        .function(#float_t " max(" #float_t " a, " #float_t " b)", fp<&script_math_max<float_t>>)                       \
        .function(#float_t " clamp(" #float_t " val, " #float_t " a, " #float_t " b)", fp<&script_math_clamp<float_t>>) \
        .function(#float_t " lerp(" #float_t " a, " #float_t " b, " #float_t " t)", fp<&script_math_lerp<float_t>>)     \
        .ASBIND20_EXT_MATH_UNARY_FUNC(ceil, float_t)                                                                    \
        .ASBIND20_EXT_MATH_UNARY_FUNC(floor, float_t)                                                                   \
        .ASBIND20_EXT_MATH_UNARY_FUNC(round, float_t)                                                                   \
        .ASBIND20_EXT_MATH_UNARY_FUNC(trunc, float_t)                                                                   \
        .ASBIND20_EXT_MATH_UNARY_FUNC(abs, float_t)                                                                     \
        .ASBIND20_EXT_MATH_UNARY_FUNC(sqrt, float_t)                                                                    \
        .ASBIND20_EXT_MATH_UNARY_FUNC(cbrt, float_t)                                                                    \
        .ASBIND20_EXT_MATH_UNARY_FUNC(exp, float_t)                                                                     \
        .ASBIND20_EXT_MATH_UNARY_FUNC(exp2, float_t)                                                                    \
        .ASBIND20_EXT_MATH_UNARY_FUNC(log, float_t)                                                                     \
        .ASBIND20_EXT_MATH_UNARY_FUNC(log2, float_t)                                                                    \
        .ASBIND20_EXT_MATH_UNARY_FUNC(log10, float_t)                                                                   \
        .ASBIND20_EXT_MATH_UNARY_FUNC(cos, float_t)                                                                     \
        .ASBIND20_EXT_MATH_UNARY_FUNC(sin, float_t)                                                                     \
        .ASBIND20_EXT_MATH_UNARY_FUNC(tan, float_t)                                                                     \
        .ASBIND20_EXT_MATH_UNARY_FUNC(acos, float_t)                                                                    \
        .ASBIND20_EXT_MATH_UNARY_FUNC(asin, float_t)                                                                    \
        .ASBIND20_EXT_MATH_UNARY_FUNC(atan, float_t)                                                                    \
        .ASBIND20_EXT_MATH_UNARY_FUNC(cosh, float_t)                                                                    \
        .ASBIND20_EXT_MATH_UNARY_FUNC(sinh, float_t)                                                                    \
        .ASBIND20_EXT_MATH_UNARY_FUNC(tanh, float_t)                                                                    \
        .ASBIND20_EXT_MATH_UNARY_FUNC(acosh, float_t)                                                                   \
        .ASBIND20_EXT_MATH_UNARY_FUNC(asinh, float_t)                                                                   \
        .ASBIND20_EXT_MATH_UNARY_FUNC(atanh, float_t)                                                                   \
        .function("bool isfinite(" #float_t " x)", fp<&script_isfinite>)                                                \
        .function("bool isnan(" #float_t " x)", fp<&script_isnan>)                                                      \
        .function("bool isinf(" #float_t " x)", fp<&script_isinf>)                                                      \
        .ASBIND20_EXT_MATH_BINARY_FUNC(pow, float_t)                                                                    \
        .ASBIND20_EXT_MATH_BINARY_FUNC(atan2, float_t)                                                                  \
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

template <typename Float>
void complex_list_constructor(void* mem, Float* list_buf)
{
    new(mem) std::complex<Float>{list_buf[0], list_buf[1]};
}

template <typename Float>
static Float complex_squared_length(const std::complex<Float>& c)
{
    return c.real() * c.real() + c.imag() * c.imag();
}

template <typename Float>
static Float complex_length(const std::complex<Float>& c)
{
    return std::sqrt(complex_squared_length<Float>(c));
}

template <typename Float>
static Float complex_abs(const std::complex<Float>& c)
{
    return std::abs(c);
}

template <bool UseGeneric>
void register_math_complex_impl(asIScriptEngine* engine)
{
    using complex_t = std::complex<float>;

    value_class<std::complex<float>, UseGeneric> c(
        engine,
        "complex",
        asOBJ_POD | asOBJ_APP_CLASS_MORE_CONSTRUCTORS | asOBJ_APP_CLASS_ALLFLOATS
    );
    c
        .constructor_function(
            "",
            [](void* mem)
            { new(mem) std::complex<float>(); }
        )
        .template constructor<float>("float r")
        .template constructor<float, float>("float r, float i")
        .list_constructor_function("float,float", fp<&complex_list_constructor<float>>)
        .opEquals()
        .opAddAssign()
        .opSubAssign()
        .opMulAssign()
        .opDivAssign()
        .opAdd()
        .opSub()
        .opMul()
        .opDiv()
        .method("float get_squared_length() const property", fp<&complex_squared_length<float>>)
        .method("float get_length() const property", fp<&complex_length<float>>)
        .property("float real", 0)
        .property("float imag", sizeof(float));

    global<UseGeneric> g(engine);
    g
        .function("float abs(const complex&in)", fp<&complex_abs<float>>);
}

void register_math_complex(asIScriptEngine* engine, bool generic)
{
    if(generic)
        register_math_complex_impl<true>(engine);
    else
        register_math_complex_impl<false>(engine);
}
} // namespace asbind20::ext
