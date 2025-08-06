#include "shared_bench_lib.hpp"
#include <sstream>
#include <asbind20/io/stream.hpp>

namespace bench_io
{
static auto prepare_module(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
    -> AS_NAMESPACE_QUALIFIER asIScriptModule*
{
    auto* m = engine->GetModule(
        "test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "test",
        "int test() { return 42; }"
    );
    BENCHMARK_UNUSED
    int r = m->Build();
    assert(r >= 0);

    return m;
}
} // namespace bench_io

static void byte_code_sstream(benchmark::State& state)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    auto* m = bench_io::prepare_module(engine);

    for(auto&& _ : state)
    {
        std::stringstream ss;
        save_byte_code(ss, m);
    }
}

static void byte_code_output_it(benchmark::State& state)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    auto* m = bench_io::prepare_module(engine);

    for(auto&& _ : state)
    {
        std::vector<std::byte> vec;
        save_byte_code(std::back_inserter(vec), m);
    }
}

BENCHMARK(byte_code_sstream);
BENCHMARK(byte_code_output_it);

BENCHMARK_MAIN();
