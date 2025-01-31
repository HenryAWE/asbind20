#include <asbind20/ext/hash.hpp>
#include <functional>

namespace asbind20::ext
{
template <typename T>
static std::uint64_t std_hash_wrapper(T val)
{
    return std::hash<T>{}(val);
}

template <bool UseGeneric>
static void register_script_hash_impl(asIScriptEngine* engine)
{
    global<UseGeneric> g(engine);
    g
        .typedef_("uint64", "hash_result_t")
        .function("uint64 hash(int8)", fp<&std_hash_wrapper<std::int8_t>>)
        .function("uint64 hash(int16)", fp<&std_hash_wrapper<std::int16_t>>)
        .function("uint64 hash(int)", fp<&std_hash_wrapper<std::int32_t>>)
        .function("uint64 hash(int64)", fp<&std_hash_wrapper<std::int64_t>>)
        .function("uint64 hash(uint8)", fp<&std_hash_wrapper<std::uint8_t>>)
        .function("uint64 hash(uint16)", fp<&std_hash_wrapper<std::uint16_t>>)
        .function("uint64 hash(uint)", fp<&std_hash_wrapper<std::uint32_t>>)
        .function("uint64 hash(uint64)", fp<&std_hash_wrapper<std::uint64_t>>)
        .function("uint64 hash(float)", fp<&std_hash_wrapper<float>>)
        .function("uint64 hash(double)", fp<&std_hash_wrapper<double>>);
}

void register_script_hash(asIScriptEngine* engine, bool generic)
{
    if(generic)
        register_script_hash_impl<true>(engine);
    else
        register_script_hash_impl<false>(engine);
}
} // namespace asbind20::ext
