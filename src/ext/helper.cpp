#include <asbind20/ext/helper.hpp>
#include <functional>

namespace asbind20::ext
{
template <typename T>
static std::uint64_t std_hash_wrapper(T val)
{
    return std::hash<T>{}(val);
}

template <bool UseGeneric>
static bool register_script_hash_impl(asIScriptEngine* engine)
{
    global_t<UseGeneric> g(engine);
    g
        .typedef_("uint64", "hash_result_t")
        .template function<&std_hash_wrapper<std::int8_t>>("uint64 hash(int8)")
        .template function<&std_hash_wrapper<std::int16_t>>("uint64 hash(int16)")
        .template function<&std_hash_wrapper<std::int32_t>>("uint64 hash(int)")
        .template function<&std_hash_wrapper<std::int64_t>>("uint64 hash(int64)")
        .template function<&std_hash_wrapper<std::uint8_t>>("uint64 hash(uint8)")
        .template function<&std_hash_wrapper<std::uint16_t>>("uint64 hash(uint16)")
        .template function<&std_hash_wrapper<std::uint32_t>>("uint64 hash(uint)")
        .template function<&std_hash_wrapper<std::uint64_t>>("uint64 hash(uint64)")
        .template function<&std_hash_wrapper<float>>("uint64 hash(float)")
        .template function<&std_hash_wrapper<double>>("uint64 hash(double)");
}

void register_script_hash(asIScriptEngine* engine, bool generic)
{
    if(generic)
        register_script_hash_impl<true>(engine);
    else
        register_script_hash_impl<false>(engine);
}
} // namespace asbind20::ext
