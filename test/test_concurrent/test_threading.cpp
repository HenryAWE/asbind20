#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <thread>
#include <asbind20/concurrent/threading.hpp>

TEST(threading, auto_clean_up)
{
    using namespace asbind20;
    using namespace std::chrono_literals;
    concurrent::prepare_multithread();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    auto* m = engine->GetModule(
        "script_multithreading", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "script_multithreading",
        "int fn(int arg) { return arg * 2; }"
    );
    ASSERT_GE(m->Build(), 0);
    auto* f = m->GetFunctionByName("fn");
    ASSERT_NE(f, nullptr);

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
