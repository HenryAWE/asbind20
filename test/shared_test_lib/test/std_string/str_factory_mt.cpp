// Testing string factory in multithreaded environment

#include "test_script_string.hpp"

#include <thread>
#include <asbind20/concurrent/threading.hpp>

namespace test_script_string
{
template <bool UseGeneric>
class mt_string_factory_suite : public script_string_suite_base<UseGeneric>
{
    using my_base = script_string_suite_base<UseGeneric>;

public:
    void SetUp() override
    {
        if(!asbind20::has_threads())
            GTEST_SKIP() << "No multithread environment";

        asbind20::concurrent::prepare_multithread();
        my_base::SetUp();
        asbind_test::setup_script_assertion(this->get_engine());
    }

    void TearDown() override
    {
        my_base::TearDown();
    }

    asbind20::module_pointer get_module()
    {
        asbind20::engine_pointer engine = this->get_engine();
        auto* m = engine->GetModule(
            "mt_string_test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        EXPECT_NE(m, nullptr);

        m->AddScriptSection(
            "mt_string_test",
            "void f()\n"
            "{\n"
            "    string s = \"test\";\n"
            "    assert(s.size == 4);\n"
            "}"
        );
        EXPECT_GE(m->Build(), 0);

        return m;
    }

    auto prepare_contexts() const
    {
        std::vector<asbind20::script_context> contexts;

        auto* engine = this->get_engine();
        for(std::size_t i = 0; i < 10; ++i)
        {
            auto& created = contexts.emplace_back(engine);
            EXPECT_NE(created.get(), nullptr)
                << "Failed to create context. i = " << i;
        }

        return contexts;
    }
};

static void mt_test_thread_main(
    std::atomic_int& counter,
    asbind20::context_pointer ctx,
    asbind20::function_pointer f
)
{
    asbind20::concurrent::auto_thread_cleanup();

    auto result = asbind20::script_invoke<void>(ctx, f);
    ASBIND_TEST_EXPECT_INVOKE_RESULT(result);

    counter.fetch_add(1);
}
} // namespace test_script_string

using MTStrFactoryTestNative = test_script_string::mt_string_factory_suite<false>;
using MTStrFactoryTestGeneric = test_script_string::mt_string_factory_suite<true>;

TEST_F(MTStrFactoryTestNative, RunScript)
{
    using namespace asbind20;

    auto* m = this->get_module();
    auto* f = m->GetFunctionByName("f");
    ASSERT_NE(f, nullptr);

    auto contexts = this->prepare_contexts();
    std::atomic_int counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(contexts.size());
    for(auto& ctx : contexts)
    {
        threads.emplace_back(
            &test_script_string::mt_test_thread_main,
            std::ref(counter),
            ctx.get(),
            f
        );
    }
    for(auto& t : threads)
        t.detach();

    while(counter.load(std::memory_order_consume) != static_cast<int>(contexts.size()))
        std::this_thread::yield();
}

TEST_F(MTStrFactoryTestGeneric, RunScript)
{
    using namespace asbind20;

    auto* m = this->get_module();
    auto* f = m->GetFunctionByName("f");
    ASSERT_NE(f, nullptr);

    auto contexts = this->prepare_contexts();
    std::atomic_int counter = 0;

    std::vector<std::thread> threads;
    threads.reserve(contexts.size());
    for(auto& ctx : contexts)
    {
        threads.emplace_back(
            &test_script_string::mt_test_thread_main,
            std::ref(counter),
            ctx.get(),
            f
        );
    }
    for(auto& t : threads)
        t.detach();

    while(counter.load(std::memory_order_consume) != static_cast<int>(contexts.size()))
        std::this_thread::yield();
}
