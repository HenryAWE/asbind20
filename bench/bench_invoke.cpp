#include "shared_bench_lib.hpp"
#include <asbind20/ext/stdstring.hpp>
#include <cassert>
#include <iostream>

namespace bench_invoke
{
static auto prepare_get_int(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
    -> AS_NAMESPACE_QUALIFIER asIScriptFunction*
{
    auto* m = engine->GetModule(
        "bench_get_int", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "bench_to_lower",
        "int run()"
        "{\n"
        "    return 42;"
        "}"
    );

    BENCHMARK_UNUSED
    int r = m->Build();
    assert(r >= 0);

    auto* f = m->GetFunctionByName("run");
    assert(f != nullptr);
    return f;
}
} // namespace bench_invoke

static void get_int_auto_get(benchmark::State& state)
{
    using namespace asbind20;
    using namespace std::string_literals;

    auto engine = make_script_engine();
    auto* f = bench_invoke::prepare_get_int(engine);

    request_context ctx(engine);
    for(auto&& _ : state)
    {
        auto result = script_invoke<int>(ctx, f);
        assert(result.has_value());
        if(int val = *result; val != 42)
            throw std::runtime_error("bad result=" + std::to_string(val));
    }
}

BENCHMARK(get_int_auto_get);

static void get_int_manual_get(benchmark::State& state)
{
    using namespace asbind20;
    using namespace std::string_literals;

    auto engine = make_script_engine();
    auto* f = bench_invoke::prepare_get_int(engine);

    request_context ctx(engine);
    for(auto&& _ : state)
    {
        ctx->Prepare(f);
        ctx->Execute();
        BENCHMARK_UNUSED
        int r = ctx->GetState();
        assert(r == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED);

        auto result = static_cast<int>(ctx->GetReturnDWord());
        if(result != 42)
            throw std::runtime_error("bad result=" + std::to_string(result));
    }
}

BENCHMARK(get_int_manual_get);

namespace bench_invoke
{
static std::string str_to_lower(const std::string& str)
{
    std::string result;
    std::transform(
        str.begin(),
        str.end(),
        std::back_inserter(result),
        [](char c) -> char
        {
            if(c >= 'A' && c <= 'Z')
                return c + ('a' - 'A');
            return c;
        }
    );
    return result;
}

template <bool UseGeneric>
static void setup_to_lower_env(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    ext::register_std_string(engine, true, UseGeneric);
    global<UseGeneric>(engine)
        .function("string to_lower(const string&in)", fp<&str_to_lower>)
        .message_callback(
            +[](const asSMessageInfo* msg)
            { std::cerr << msg->message << std::endl; }
        );
}

static auto prepare_to_lower(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
    -> AS_NAMESPACE_QUALIFIER asIScriptFunction*
{
    auto* m = engine->GetModule(
        "bench_to_lower", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "bench_to_lower",
        "string run(const string&in s1)"
        "{\n"
        "    string str = to_lower(s1);\n"
        "    return str;\n"
        "}"
    );

    BENCHMARK_UNUSED
    int r = m->Build();
    assert(r >= 0);

    // Return the raw script function,
    // because we'll compare the performance for retrieving result from script later
    auto* f = m->GetFunctionByName("run");
    assert(f != nullptr);
    return f;
}

static const std::string to_lower_input_arg = R"(TEST:
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.
)";
static const std::string to_lower_input_expected = R"(test:
lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.
ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.
)";
} // namespace bench_invoke

#ifndef ASBIND_BENCH_NO_NATIVE

static void native_to_lower_auto_get(benchmark::State& state)
{
    using namespace asbind20;
    using namespace std::string_literals;

    auto engine = make_script_engine();
    bench_invoke::setup_to_lower_env<false>(engine);
    script_function<std::string(const std::string&)> run(
        bench_invoke::prepare_to_lower(engine)
    );

    request_context ctx(engine);
    for(auto&& _ : state)
    {
        auto result = run(ctx, bench_invoke::to_lower_input_arg);
        assert(result.has_value());
        if(result.value() != bench_invoke::to_lower_input_expected)
            throw std::runtime_error("bad result=" + result.value());
    }
}

BENCHMARK(native_to_lower_auto_get);

static void native_to_lower_manual_get(benchmark::State& state)
{
    using namespace asbind20;
    using namespace std::string_literals;

    auto engine = make_script_engine();
    bench_invoke::setup_to_lower_env<false>(engine);
    auto* f = bench_invoke::prepare_to_lower(engine);

    request_context ctx(engine);
    for(auto&& _ : state)
    {
        ctx->Prepare(f);
        set_script_arg(ctx, 0, bench_invoke::to_lower_input_arg);

        ctx->Execute();
        BENCHMARK_UNUSED
        int r = ctx->GetState();
        assert(r == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED);

        auto* result = static_cast<std::string*>(ctx->GetReturnObject());
        assert(result != nullptr);
        if(*result != bench_invoke::to_lower_input_expected)
            throw std::runtime_error("bad result=" + *result);
    }
}

BENCHMARK(native_to_lower_manual_get);

#endif

static void generic_to_lower_auto_get(benchmark::State& state)
{
    using namespace asbind20;
    using namespace std::string_literals;

    auto engine = make_script_engine();
    bench_invoke::setup_to_lower_env<true>(engine);
    script_function<std::string(const std::string&)> run(
        bench_invoke::prepare_to_lower(engine)
    );

    request_context ctx(engine);
    for(auto&& _ : state)
    {
        auto result = run(ctx, bench_invoke::to_lower_input_arg);
        assert(result.has_value());
        if(result.value() != bench_invoke::to_lower_input_expected)
            throw std::runtime_error("bad result=" + result.value());
    }
}

BENCHMARK(generic_to_lower_auto_get);

static void generic_to_lower_manual_get(benchmark::State& state)
{
    using namespace asbind20;
    using namespace std::string_literals;

    auto engine = make_script_engine();
    bench_invoke::setup_to_lower_env<true>(engine);
    auto* f = bench_invoke::prepare_to_lower(engine);

    request_context ctx(engine);
    for(auto&& _ : state)
    {
        ctx->Prepare(f);
        set_script_arg(ctx, 0, bench_invoke::to_lower_input_arg);

        ctx->Execute();
        BENCHMARK_UNUSED
        int r = ctx->GetState();
        assert(r == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED);

        auto* result = static_cast<std::string*>(ctx->GetReturnObject());
        assert(result != nullptr);
        if(*result != bench_invoke::to_lower_input_expected)
            throw std::runtime_error("bad result=" + *result);
    }
}

BENCHMARK(generic_to_lower_manual_get);

BENCHMARK_MAIN();
