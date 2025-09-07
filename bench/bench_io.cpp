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

static void save_byte_code_sstream(benchmark::State& state)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    auto* m = bench_io::prepare_module(engine);

    for(auto&& _ : state)
    {
        std::stringstream ss;
        int r = save_byte_code(ss, m);
        assert(r >= 0);
    }
}

static void save_byte_code_sstream_stripped(benchmark::State& state)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    auto* m = bench_io::prepare_module(engine);

    for(auto&& _ : state)
    {
        std::stringstream ss;
        int r = save_byte_code(ss, m, true);
        assert(r >= 0);
    }
}

static void save_byte_code_output_it(benchmark::State& state)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    auto* m = bench_io::prepare_module(engine);

    for(auto&& _ : state)
    {
        std::vector<std::byte> vec;
        int r = save_byte_code(std::back_inserter(vec), m);
        assert(r >= 0);
    }
}

static void save_byte_code_output_it_stripped(benchmark::State& state)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    auto* m = bench_io::prepare_module(engine);

    for(auto&& _ : state)
    {
        std::vector<std::byte> vec;
        [[maybe_unused]]
        int r = save_byte_code(std::back_inserter(vec), m, true);
        assert(r >= 0);
    }
}

BENCHMARK(save_byte_code_sstream);
BENCHMARK(save_byte_code_sstream_stripped);
BENCHMARK(save_byte_code_output_it);
BENCHMARK(save_byte_code_output_it_stripped);

namespace bench_io
{
template <typename Result>
static void prepare_byte_code(Result&& result, bool strip_debug_info = false)
{
    // temp engine
    auto engine = asbind20::make_script_engine();

    auto* m = prepare_module(engine);
    [[maybe_unused]]
    int r = asbind20::save_byte_code(std::forward<Result>(result), m, strip_debug_info);
    assert(r >= 0);
}
} // namespace bench_io

static void load_byte_code_sstream(benchmark::State& state)
{
    using namespace asbind20;

    const std::string s = []()
    {
        std::stringstream buf;
        bench_io::prepare_byte_code(buf);
        return std::move(buf).str();
    }();

    auto engine = make_script_engine();

    for(auto&& _ : state)
    {
        auto* m = engine->GetModule(
            "test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        std::stringstream ss(s);
        [[maybe_unused]]
        auto result = asbind20::load_byte_code(ss, m);
        assert(!result.debug_info_stripped);
        assert(result);
    }
}

static void load_byte_code_sstream_stripped(benchmark::State& state)
{
    using namespace asbind20;

    const std::string s = []()
    {
        std::stringstream buf;
        bench_io::prepare_byte_code(buf, true);
        return std::move(buf).str();
    }();

    auto engine = make_script_engine();

    for(auto&& _ : state)
    {
        auto* m = engine->GetModule(
            "test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        std::stringstream ss(s);
        [[maybe_unused]]
        auto result = asbind20::load_byte_code(ss, m);
        assert(result.debug_info_stripped);
        assert(result);
    }
}

static void load_byte_code_mem(benchmark::State& state)
{
    using namespace asbind20;

    std::vector<std::byte> bc;
    bench_io::prepare_byte_code(std::back_inserter(bc));
    auto engine = make_script_engine();

    for(auto&& _ : state)
    {
        auto* m = engine->GetModule(
            "test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        [[maybe_unused]]
        auto result = asbind20::load_byte_code(bc.data(), bc.size(), m);
        assert(!result.debug_info_stripped);
        assert(result);
    }
}

static void load_byte_code_mem_stripped(benchmark::State& state)
{
    using namespace asbind20;

    std::vector<std::byte> bc;
    bench_io::prepare_byte_code(std::back_inserter(bc), true);
    auto engine = make_script_engine();

    for(auto&& _ : state)
    {
        auto* m = engine->GetModule(
            "test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        [[maybe_unused]]
        auto result = asbind20::load_byte_code(bc.data(), bc.size(), m);
        assert(result.debug_info_stripped);
        assert(result);
    }
}

BENCHMARK(load_byte_code_sstream);
BENCHMARK(load_byte_code_sstream_stripped);
BENCHMARK(load_byte_code_mem);
BENCHMARK(load_byte_code_mem_stripped);

BENCHMARK_MAIN();
