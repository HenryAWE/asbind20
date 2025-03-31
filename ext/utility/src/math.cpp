#include <asbind20/ext/math.hpp>
#include <cmath>
#include <limits>
#include <numbers>
#include <complex>
#include <numeric>

namespace asbind20::ext
{
template <typename T>
static constexpr T constant_inf_v = std::numeric_limits<T>::infinity();

template <typename T>
static constexpr T constant_nan_v = std::numeric_limits<T>::quiet_NaN();

void register_math_constants(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, std::string_view ns
)
{
    namespace_ ns_guard(engine, ns);

    global g(engine);

#define ASBIND20_EXT_MATH_REGISTER_CONSTANTS(float_t, suffix)                                           \
    g                                                                                                   \
        .property("const " #float_t " PI" suffix, std::numbers::pi_v<float_t>)                          \
        .property("const " #float_t " E" suffix, std::numbers::e_v<float_t>)                            \
        .property("const " #float_t " PHI" suffix, std::numbers::phi_v<float_t>) /* The golden ratio */ \
        .property("const " #float_t " NAN" suffix, constant_nan_v<float_t>)                             \
        .property("const " #float_t " INFINITY" suffix, constant_inf_v<float_t>)

    ASBIND20_EXT_MATH_REGISTER_CONSTANTS(float, "_f");
    ASBIND20_EXT_MATH_REGISTER_CONSTANTS(double, "_d");

#undef ASBIND20_EXT_MATH_REGISTER_CONSTANTS
}

// Use wrapper function to avoid directly getting function addresses from standard library,
// which may cause problems.

// clang-format off

#define ASBIND20_EXT_MATH_DEF_UNARY_F(name) \
template <typename T>                       \
static T math_##name##_1(T arg)             \
{                                           \
    return std::name(arg);                  \
}

#define ASBIND20_EXT_MATH_DEF_BINARY_F(name)     \
template <typename T>                            \
static T math_##name##_2(T arg1, T arg2)         \
{                                                \
    return std::name(arg1, arg2);                \
}

#define ASBIND20_EXT_MATH_DEF_TRINARY_F(name)            \
template <typename T>                                    \
static T math_##name##_3(T arg1, T arg2, T arg3)         \
{                                                        \
    return std::name(arg1, arg2, arg3);                  \
}

#define ASBIND20_EXT_MATH_DEF_BOOL_UNARY_F(name) \
template <typename T>                            \
static bool math_##name##_1(T arg)               \
{                                                \
    return std::name(arg);                       \
}

// clang-format on

// Basic operations
ASBIND20_EXT_MATH_DEF_UNARY_F(abs);
ASBIND20_EXT_MATH_DEF_BINARY_F(min);
ASBIND20_EXT_MATH_DEF_BINARY_F(max);
// Nearest integer for floating points
ASBIND20_EXT_MATH_DEF_UNARY_F(ceil)
ASBIND20_EXT_MATH_DEF_UNARY_F(floor)
ASBIND20_EXT_MATH_DEF_UNARY_F(trunc)
ASBIND20_EXT_MATH_DEF_UNARY_F(round)
ASBIND20_EXT_MATH_DEF_UNARY_F(nearbyint)
// Factor operations
ASBIND20_EXT_MATH_DEF_BINARY_F(gcd);
ASBIND20_EXT_MATH_DEF_BINARY_F(lcm);
// Interpolation
ASBIND20_EXT_MATH_DEF_BINARY_F(midpoint);
ASBIND20_EXT_MATH_DEF_TRINARY_F(lerp);
// Exponential functions
ASBIND20_EXT_MATH_DEF_UNARY_F(exp);
ASBIND20_EXT_MATH_DEF_UNARY_F(exp2);
ASBIND20_EXT_MATH_DEF_UNARY_F(expm1);
ASBIND20_EXT_MATH_DEF_UNARY_F(log);
ASBIND20_EXT_MATH_DEF_UNARY_F(log10);
ASBIND20_EXT_MATH_DEF_UNARY_F(log2);
ASBIND20_EXT_MATH_DEF_UNARY_F(log1p);
// Power functions
ASBIND20_EXT_MATH_DEF_BINARY_F(pow);
ASBIND20_EXT_MATH_DEF_UNARY_F(sqrt);
ASBIND20_EXT_MATH_DEF_UNARY_F(cbrt);
ASBIND20_EXT_MATH_DEF_BINARY_F(hypot);
ASBIND20_EXT_MATH_DEF_TRINARY_F(hypot);
// Trigonometric functions
ASBIND20_EXT_MATH_DEF_UNARY_F(sin);
ASBIND20_EXT_MATH_DEF_UNARY_F(cos);
ASBIND20_EXT_MATH_DEF_UNARY_F(tan);
ASBIND20_EXT_MATH_DEF_UNARY_F(asin);
ASBIND20_EXT_MATH_DEF_UNARY_F(acos);
ASBIND20_EXT_MATH_DEF_UNARY_F(atan);
ASBIND20_EXT_MATH_DEF_BINARY_F(atan2);
// Hyperbolic functions
ASBIND20_EXT_MATH_DEF_UNARY_F(sinh);
ASBIND20_EXT_MATH_DEF_UNARY_F(cosh);
ASBIND20_EXT_MATH_DEF_UNARY_F(tanh);
ASBIND20_EXT_MATH_DEF_UNARY_F(asinh);
ASBIND20_EXT_MATH_DEF_UNARY_F(acosh);
ASBIND20_EXT_MATH_DEF_UNARY_F(atanh);
// Floating-point classification
ASBIND20_EXT_MATH_DEF_BOOL_UNARY_F(isfinite);
ASBIND20_EXT_MATH_DEF_BOOL_UNARY_F(isinf);
ASBIND20_EXT_MATH_DEF_BOOL_UNARY_F(isnan);
ASBIND20_EXT_MATH_DEF_BOOL_UNARY_F(signbit);

#undef ASBIND20_EXT_MATH_DEF_UNARY_F
#undef ASBIND20_EXT_MATH_DEF_BINARY_F
#undef ASBIND20_EXT_MATH_DEF_TRINARY_F
#undef ASBIND20_EXT_MATH_DEF_BOOL_UNARY_F

template <std::floating_point T>
static bool close_to_simple(T a, T b) noexcept
{
    // Use the default epsilon value
    return math_close_to(a, b);
}

template <bool UseGeneric>
void register_math_func_integral(asIScriptEngine* engine)
{
    static_assert(sizeof(int) == sizeof(std::int32_t), "AngelScript assumes int is 32-bit");
    // Align type names between AS and C++
    using uint = std::uint32_t;
    using int64 = std::int64_t;
    using uint64 = std::uint64_t;

    // clang-format off

#define ASBIND20_EXT_MATH_REG_UNARY_F(type, name) \
    function(#type " " #name "(" #type " num)", fp<overload_cast<type>(&math_##name##_1<type>)>)

#define ASBIND20_EXT_MATH_REG_BINARY_F(type, name, arg1, arg2) \
    function(#type " " #name "(" #type " " #arg1 "," #type " " #arg2 ")", fp<overload_cast<type, type>(&math_##name##_2<type>)>)

#define ASBIND20_EXT_MATH_REG_UNARY_F_SIGNED(name) \
     ASBIND20_EXT_MATH_REG_UNARY_F(int, name)      \
    .ASBIND20_EXT_MATH_REG_UNARY_F(int64, name)

#define ASBIND20_EXT_MATH_REG_BINARY_F_SIGNED(name, arg1, arg2) \
     ASBIND20_EXT_MATH_REG_BINARY_F(int, name, arg1, arg2)      \
    .ASBIND20_EXT_MATH_REG_BINARY_F(int64, name, arg1, arg2)

#define ASBIND20_EXT_MATH_REG_UNARY_F_UNSIGNED(name) \
     ASBIND20_EXT_MATH_REG_UNARY_F(uint, name)       \
    .ASBIND20_EXT_MATH_REG_UNARY_F(uint64, name)

#define ASBIND20_EXT_MATH_REG_BINARY_F_UNSIGNED(name, arg1, arg2) \
     ASBIND20_EXT_MATH_REG_BINARY_F(uint, name, arg1, arg2)       \
    .ASBIND20_EXT_MATH_REG_BINARY_F(uint64, name, arg1, arg2)

#define ASBIND20_EXT_MATH_REG_UNARY_F_ALL(name) \
     ASBIND20_EXT_MATH_REG_UNARY_F_SIGNED(name) \
    .ASBIND20_EXT_MATH_REG_UNARY_F_UNSIGNED(name)

#define ASBIND20_EXT_MATH_REG_BINARY_F_ALL(name, arg1, arg2) \
     ASBIND20_EXT_MATH_REG_BINARY_F_SIGNED(name, arg1, arg2) \
    .ASBIND20_EXT_MATH_REG_BINARY_F_UNSIGNED(name, arg1, arg2)

    // clang-format on

    global<UseGeneric>(engine)
        // Basic operations
        .ASBIND20_EXT_MATH_REG_UNARY_F_SIGNED(abs)
        .ASBIND20_EXT_MATH_REG_BINARY_F_ALL(min, a, b)
        .ASBIND20_EXT_MATH_REG_BINARY_F_ALL(max, a, b)
        // Factor operations
        .ASBIND20_EXT_MATH_REG_BINARY_F_ALL(gcd, x, y)
        .ASBIND20_EXT_MATH_REG_BINARY_F_ALL(lcm, x, y)
        // Interpolation
        .ASBIND20_EXT_MATH_REG_BINARY_F_ALL(midpoint, a, b);

#undef ASBIND20_EXT_MATH_REG_UNARY_F_SIGNED
#undef ASBIND20_EXT_MATH_REG_BINARY_F_SIGNED
#undef ASBIND20_EXT_MATH_REG_UNARY_F_UNSIGNED
#undef ASBIND20_EXT_MATH_REG_BINARY_F_UNSIGNED
#undef ASBIND20_EXT_MATH_REG_UNARY_F_ALL
#undef ASBIND20_EXT_MATH_REG_BINARY_F_ALL
#undef ASBIND20_EXT_MATH_REG_UNARY_F
#undef ASBIND20_EXT_MATH_REG_BINARY_F
}

template <bool UseGeneric>
void register_math_func_float(asIScriptEngine* engine)
{
    // clang-format off

#define ASBIND20_EXT_MATH_REG_UNARY_F(name)                              \
     function("float " #name "(float num)", fp<&math_##name##_1<float>>) \
    .function("double " #name "(double num)", fp<&math_##name##_1<double>>)

#define ASBIND20_EXT_MATH_REG_BINARY_F(name, arg1, arg2)                                       \
     function("float " #name "(float " #arg1 ",float " #arg2 ")", fp<&math_##name##_2<float>>) \
    .function("double " #name "(double " #arg1 ",double " #arg2 ")", fp<&math_##name##_2<double>>)

#define ASBIND20_EXT_MATH_REG_TRINARY_F(name, arg1, arg2, arg3)                                                \
     function("float " #name "(float " #arg1 ",float " #arg2 ",float " #arg3 ")", fp<&math_##name##_3<float>>) \
    .function("double " #name "(double " #arg1 ",double " #arg2 ",double " #arg3 ")", fp<&math_##name##_3<double>>)

#define ASBIND20_EXT_MATH_REG_BOOL_UNARY_F(name)                        \
     function("bool " #name "(float num)", fp<&math_##name##_1<float>>) \
    .function("bool " #name "(double num)", fp<&math_##name##_1<double>>)

    // clang-format on

    asbind20::global<UseGeneric>(engine)
        // Basic operations
        .ASBIND20_EXT_MATH_REG_UNARY_F(abs)
        .ASBIND20_EXT_MATH_REG_BINARY_F(min, a, b)
        .ASBIND20_EXT_MATH_REG_BINARY_F(max, a, b)
        .function("bool close_to(float a, float b)", fp<&close_to_simple<float>>)
        .function("bool close_to(double a, double b)", fp<&close_to_simple<double>>)
        .function("bool close_to(float a, float b, float epsilon)", fp<&math_close_to<float>>)
        .function("bool close_to(double a, double b, double epsilon)", fp<&math_close_to<double>>)
        // Nearest integer for floating points
        .ASBIND20_EXT_MATH_REG_UNARY_F(ceil)
        .ASBIND20_EXT_MATH_REG_UNARY_F(floor)
        .ASBIND20_EXT_MATH_REG_UNARY_F(trunc)
        .ASBIND20_EXT_MATH_REG_UNARY_F(round)
        .ASBIND20_EXT_MATH_REG_UNARY_F(nearbyint)
        // Interpolation
        .ASBIND20_EXT_MATH_REG_BINARY_F(midpoint, a, b)
        .ASBIND20_EXT_MATH_REG_TRINARY_F(lerp, a, b, t)
        // Exponential functions
        .ASBIND20_EXT_MATH_REG_UNARY_F(exp)
        .ASBIND20_EXT_MATH_REG_UNARY_F(exp2)
        .ASBIND20_EXT_MATH_REG_UNARY_F(expm1)
        .ASBIND20_EXT_MATH_REG_UNARY_F(log)
        .ASBIND20_EXT_MATH_REG_UNARY_F(log10)
        .ASBIND20_EXT_MATH_REG_UNARY_F(log2)
        .ASBIND20_EXT_MATH_REG_UNARY_F(log1p)
        // Power functions
        .ASBIND20_EXT_MATH_REG_BINARY_F(pow, x, y)
        .ASBIND20_EXT_MATH_REG_UNARY_F(sqrt)
        .ASBIND20_EXT_MATH_REG_UNARY_F(cbrt)
        .ASBIND20_EXT_MATH_REG_BINARY_F(hypot, x, y)
        .ASBIND20_EXT_MATH_REG_TRINARY_F(hypot, x, y, z)
        // Trigonometric functions
        .ASBIND20_EXT_MATH_REG_UNARY_F(sin)
        .ASBIND20_EXT_MATH_REG_UNARY_F(cos)
        .ASBIND20_EXT_MATH_REG_UNARY_F(tan)
        .ASBIND20_EXT_MATH_REG_UNARY_F(asin)
        .ASBIND20_EXT_MATH_REG_UNARY_F(acos)
        .ASBIND20_EXT_MATH_REG_UNARY_F(atan)
        .ASBIND20_EXT_MATH_REG_BINARY_F(atan2, y, x) // The order of parameters of atan2 is defined as (y, x)
        // Hyperbolic functions
        .ASBIND20_EXT_MATH_REG_UNARY_F(sinh)
        .ASBIND20_EXT_MATH_REG_UNARY_F(cosh)
        .ASBIND20_EXT_MATH_REG_UNARY_F(tanh)
        .ASBIND20_EXT_MATH_REG_UNARY_F(asinh)
        .ASBIND20_EXT_MATH_REG_UNARY_F(acosh)
        .ASBIND20_EXT_MATH_REG_UNARY_F(atanh)
        // Floating-point classification
        .ASBIND20_EXT_MATH_REG_BOOL_UNARY_F(isfinite)
        .ASBIND20_EXT_MATH_REG_BOOL_UNARY_F(isinf)
        .ASBIND20_EXT_MATH_REG_BOOL_UNARY_F(isnan)
        .ASBIND20_EXT_MATH_REG_BOOL_UNARY_F(signbit);

#undef ASBIND20_EXT_MATH_REG_UNARY_F
#undef ASBIND20_EXT_MATH_REG_BINARY_F
#undef ASBIND20_EXT_MATH_REG_TRINARY_F
}

void register_math_function(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, bool generic
)
{
    if(generic)
    {
        register_math_func_integral<true>(engine);
        register_math_func_float<true>(engine);
    }
    else
    {
        register_math_func_integral<false>(engine);
        register_math_func_float<false>(engine);
    }
}

struct complex_placeholder
{
    complex_placeholder(AS_NAMESPACE_QUALIFIER asITypeInfo*)
    {
        assert(false && "unreachable");
    }

    ~complex_placeholder()
    {
        assert(false && "unreachable");
    }
};

static bool complex_template_callback(
    AS_NAMESPACE_QUALIFIER asITypeInfo* ti, bool&
)
{
    int subtype_id = ti->GetSubTypeId();
    return subtype_id == AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT ||
           subtype_id == AS_NAMESPACE_QUALIFIER asTYPEID_DOUBLE;
}

template <bool UseGeneric>
void register_math_complex_impl(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
)
{
    using std::complex;

    template_value_class<complex_placeholder, UseGeneric>(
        engine,
        "complex<T>",
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_CD
    )
        .template_callback(fp<&complex_template_callback>)
        // Necessary placeholders
        .default_constructor()
        .destructor();

    constexpr AS_NAMESPACE_QUALIFIER asQWORD complex_flags =
        AS_NAMESPACE_QUALIFIER asOBJ_POD |
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS |
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLFLOATS;

    value_class<std::complex<float>, UseGeneric> cf(
        engine,
        "complex<float>",
        complex_flags
    );
    value_class<std::complex<double>, UseGeneric> cd(
        engine,
        "complex<double>",
        complex_flags | AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALIGN8
    );

#define ASBIND20_EXT_MATH_COMPLEX_MEMBERS(register_helper, float_t)                                    \
    register_helper                                                                                    \
        .default_constructor()                                                                         \
        .template constructor<float_t>(#float_t)                                                       \
        .template constructor<float_t, float_t>(#float_t "," #float_t)                                 \
        .template list_constructor<float_t, policies::apply_to<2>>(#float_t "," #float_t)              \
        .opEquals()                                                                                    \
        .opAddAssign()                                                                                 \
        .opSubAssign()                                                                                 \
        .opMulAssign()                                                                                 \
        .opDivAssign()                                                                                 \
        .opAdd()                                                                                       \
        .opSub()                                                                                       \
        .opMul()                                                                                       \
        .opDiv()                                                                                       \
        .opNeg()                                                                                       \
        .method(#float_t " get_squared_length() const property", fp<&complex_squared_length<float_t>>) \
        .method(#float_t " get_length() const property", fp<&complex_length<float_t>>)                 \
        .property(#float_t " real", 0)                                                                 \
        .property(#float_t " imag", sizeof(float_t))

    ASBIND20_EXT_MATH_COMPLEX_MEMBERS(cf, float);
    ASBIND20_EXT_MATH_COMPLEX_MEMBERS(cd, double);

#undef ASBIND20_EXT_MATH_COMPLEX_MEMBERS

    // Interchanging data between different element types
    cf.template constructor<const complex<double>&>("const complex<double>&in");
    cd.template constructor<const complex<float>&>("const complex<float>&in");

    global<UseGeneric>(engine)
        .function(
            "float abs(const complex<float>&in)",
            [](const std::complex<float>& c) -> float
            { return abs(c); }
        )
        .function(
            "double abs(const complex<double>&in)",
            [](const std::complex<double>& c) -> double
            { return abs(c); }
        );
}

void register_math_complex(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, bool use_generic
)
{
    if(use_generic)
        register_math_complex_impl<true>(engine);
    else
        register_math_complex_impl<false>(engine);
}
} // namespace asbind20::ext
