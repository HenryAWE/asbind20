#include <asbind20/ext/math.hpp>
#include <limits>

namespace asbind20::ext
{
template <typename T>
static std::string close_to_decl()
{
    std::string_view type_name;
    if constexpr(std::is_same_v<T, float>)
        type_name = "float";
    else if constexpr(std::is_same_v<T, double>)
        type_name = "double";
    else
        static_assert(!sizeof(T), "Invalid type");
    return asbind20::detail::concat(
        type_name,
        " close_to(",
        type_name,
        ',',
        type_name,
        ',',
        type_name,
        '=',
        std::to_string(std::numeric_limits<T>::epsilon()),
        ')'
    );
}

void register_math_function(asIScriptEngine* engine, bool disable_double)
{
    global(engine)
        .function("int abs(int)", static_cast<int (*)(int)>(&std::abs));

    global(engine)
        .function(close_to_decl<float>().c_str(), &script_close_to<float>)
        .function("float ceil(float)", static_cast<float (*)(float)>(&std::ceil))
        .function("float floor(float)", static_cast<float (*)(float)>(&std::floor))
        .function("float abs(float)", static_cast<float (*)(float)>(&std::abs))
        .function("float sqrt(float)", static_cast<float (*)(float)>(&std::sqrt))
        .function("float exp(float)", static_cast<float (*)(float)>(&std::exp))
        .function("float cos(float)", static_cast<float (*)(float)>(&std::cos))
        .function("float sin(float)", static_cast<float (*)(float)>(&std::sin))
        .function("float tan(float)", static_cast<float (*)(float)>(&std::tan))
        .function("float acos(float)", static_cast<float (*)(float)>(&std::acos))
        .function("float asin(float)", static_cast<float (*)(float)>(&std::asin))
        .function("float atan(float)", static_cast<float (*)(float)>(&std::atan))
        .function("float cosh(float)", static_cast<float (*)(float)>(&std::cosh))
        .function("float sinh(float)", static_cast<float (*)(float)>(&std::sinh))
        .function("float tanh(float)", static_cast<float (*)(float)>(&std::tanh));

    if(disable_double)
        return;
}
} // namespace asbind20::ext
