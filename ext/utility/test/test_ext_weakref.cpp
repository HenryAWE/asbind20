#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/assert.hpp>
#include <asbind20/ext/weakref.hpp>

namespace test_ext_weakref
{
static void setup_env(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    asbind_test::setup_message_callback(engine);
    asbind20::ext::register_script_assert(
        engine,
        [](std::string_view msg)
        {
            FAIL() << "weakref assertion failed: " << msg;
        }
    );
}

static void check_weakref(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule("test_weakref", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);
    m->AddScriptSection(
        "test_weakref",
        "class test{};\n"
        "void main()\n"
        "{\n"
        "    test@ t = test();\n"
        "    weakref<test> r(t);\n"
        "    assert(r.get() !is null);\n"
        "    const_weakref<test> cr;\n"
        "    @cr = r;\n"
        "    assert(cr.get() !is null);\n"
        "    @t = null;\n"
        "    assert(r.get() is null);\n"
        "    assert(cr.get() is null);\n"
        "    @t = test();\n"
        "    @cr = t;\n"
        "    assert(cr.get() !is null); \n"
        "    const test@ ct = cr; \n"
        "    assert(ct !is null); \n"
        "    assert(cr !is null); \n"
        "    assert(cr is ct); \n"
        "    @cr = null; \n"
        "    assert(cr is null);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByDecl("void main()");
    ASSERT_NE(f, nullptr);

    {
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));
    }
}
} // namespace test_ext_weakref

TEST(ext_weakref, weakref_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = make_script_engine();
    test_ext_weakref::setup_env(engine);
    ext::register_weakref(engine, false);

    test_ext_weakref::check_weakref(engine);
}

TEST(ext_weakref, weakref_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    test_ext_weakref::setup_env(engine);
    ext::register_weakref(engine, true);

    test_ext_weakref::check_weakref(engine);
}

namespace test_ext_weakref
{
class host_weakref_support
{
public:
    host_weakref_support()
    {
        std::cout << (void*)this << ": created" << std::endl;
    }

    void addref()
    {
        std::cout << (void*)this << ": addref, " << m_counter << " += 1" << std::endl;
        AS_NAMESPACE_QUALIFIER asAtomicInc(m_counter);
    }

    void release()
    {
        std::cout << (void*)this << ": release, " << m_counter << " -= 1" << std::endl;

        if(m_counter == 1 && m_weakref_flag)
        {
            std::cout << (void*)this << ": set weakref flag" << std::endl;
            m_weakref_flag->Set(true);
        }

        if(AS_NAMESPACE_QUALIFIER asAtomicDec(m_counter) == 0)
            delete this;
    }

    AS_NAMESPACE_QUALIFIER asILockableSharedBool* get_weakref_flag()
    {
        if(!m_weakref_flag)
        {
            std::cout << (void*)this << ": creating weakref flag" << std::endl;
            std::lock_guard lock(asbind20::as_exclusive_lock);
            if(!m_weakref_flag) // Double-check because other thread may have created it
                m_weakref_flag = asbind20::make_lockable_shared_bool();
        }
        return m_weakref_flag.get();
    }

private:
    ~host_weakref_support() = default;

    int m_counter = 1;
    asbind20::lockable_shared_bool m_weakref_flag;
};

static void register_host_weakref_support(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    ref_class<host_weakref_support, false>(engine, "host_weakref_support")
        .default_factory()
        .addref(&host_weakref_support::addref)
        .release(&host_weakref_support::release)
        .get_weakref_flag(&host_weakref_support::get_weakref_flag);
}

static void register_host_weakref_support(asbind20::use_generic_t, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    ref_class<host_weakref_support, true>(engine, "host_weakref_support")
        .default_factory()
        .addref(fp<&host_weakref_support::addref>)
        .release(fp<&host_weakref_support::release>)
        .get_weakref_flag(fp<&host_weakref_support::get_weakref_flag>);
}

void check_host_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule("test_host_weakref", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    // Copied from the official test case in test_addon_weakref.cpp
    m->AddScriptSection(
        "test_host_weakref",
        "weakref<host_weakref_support> r; \n"
        "host_weakref_support@ get()\n"
        "{\n"
        "    host_weakref_support@ host_class;\n"
        "    @host_class = r.get();\n"
        "    if (host_class !is null) return host_class;\n"
        "    @r = @host_class = host_weakref_support();\n"
        "    assert(host_class !is null && host_class is r.get());\n"
        "    @host_class = @r = host_weakref_support();\n"
        "    assert(host_class is null && host_class is r.get());\n"
        "    return host_class;\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("get");
    ASSERT_NE(f, nullptr);

    {
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void*>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));
    }
}
} // namespace test_ext_weakref

TEST(ext_weakref, host_weakref_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = make_script_engine();
    test_ext_weakref::setup_env(engine);
    test_ext_weakref::register_host_weakref_support(engine);
    ext::register_weakref(engine, false);

    test_ext_weakref::check_host_class(engine);
}

TEST(ext_weakref, host_weakref_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    test_ext_weakref::setup_env(engine);
    test_ext_weakref::register_host_weakref_support(use_generic, engine);
    ext::register_weakref(engine, true);

    test_ext_weakref::check_host_class(engine);
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
