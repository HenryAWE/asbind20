#include <gtest/gtest.h>
#include <asbind_test/framework.hpp>
#include <thread>
#include <chrono>
#include <asbind20/concurrent/threading.hpp>

namespace
{
asbind20::module_pointer make_helper_module(asbind20::engine_reference engine)
{
    auto* m = asbind20::create_module(engine, "script_multithreading");
    if(!m)
    {
        ADD_FAILURE() << "failed to create module";
        std::terminate();
    }
    m->AddScriptSection(
        "script_multithreading",
        "int fn(int arg) { return arg * 2; }"
    );
    if(int r = m->Build(); r < 0)
    {
        ADD_FAILURE() << "build failed: r = " << r;
        std::terminate();
    }

    return m;
}
} // namespace

TEST(Threading, AutoCleanUp)
{
    ASBIND_TEST_SKIP_IF_NO_THREADS();

    using namespace asbind20;
    using namespace std::chrono_literals;
    concurrent::prepare_multithread();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    auto* m = make_helper_module(*engine);
    auto* f = m->GetFunctionByName("fn");
    ASSERT_THAT(f, ::testing::NotNull());

    std::condition_variable cv;
    std::mutex mx;
    int result = -1;

    auto helper = [&, f](int arg)
    {
        concurrent::auto_thread_cleanup();
        {
            request_context ctx(engine);
            std::this_thread::sleep_for(3ms);
            auto r = script_invoke<int>(ctx, f, arg);
            std::unique_lock lock(mx);
            result = r.value();
        }

        cv.notify_all();
    };

    std::thread thr(helper, 10);
    EXPECT_EQ(result, -1);
    thr.detach();
    {
        std::unique_lock lock(mx);
        std::cv_status st = cv.wait_until(lock, std::chrono::system_clock::now() + 10s);
        EXPECT_EQ(st, std::cv_status::no_timeout);
    }
    EXPECT_EQ(result, 20);
}

TEST(Threading, Async)
{
    ASBIND_TEST_SKIP_IF_NO_THREADS();

    using namespace asbind20;
    using namespace std::chrono_literals;
    concurrent::prepare_multithread();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    auto* m = make_helper_module(*engine);
    auto* f = m->GetFunctionByName("fn");
    ASSERT_THAT(f, ::testing::NotNull());

    auto result = concurrent::async(
        [f, &engine]()
        {
            request_context ctx(engine);
            auto result = script_invoke<int>(ctx, f, 21);
            return result.value();
        }
    );
    std::this_thread::sleep_for(1ms);
    result.wait();

    EXPECT_TRUE(result.valid());
    EXPECT_EQ(result.get(), 42);
}
