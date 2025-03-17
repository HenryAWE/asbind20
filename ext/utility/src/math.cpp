#include <asbind20/ext/math.hpp>
#include <cmath>
#include <limits>
#include <numbers>
#include <complex>
#include <type_traits>

namespace asbind20::ext
{
template <typename T>
static constexpr T constant_inf_v = std::numeric_limits<T>::infinity();

template <typename T>
static constexpr T constant_nan_v = std::numeric_limits<T>::quiet_NaN();

void register_math_constants(asIScriptEngine* engine, const char* ns)
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
    return math_close_to<T>(a, b, std::numeric_limits<T>::epsilon());
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
void register_math_function_impl(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
)
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
        .function("bool close_to(" #float_t " a, " #float_t " b, " #float_t " epsilon)", fp<&math_close_to<float_t>>)   \
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

void register_math_function(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, bool generic
)
{
    if(generic)
        register_math_function_impl<true>(engine);
    else
        register_math_function_impl<false>(engine);
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
