#include <asbind_test/framework.hpp>
#include <asbind_test/weakref.hpp>
#include <asbind20/concurrent/mutex.hpp>
#include <mutex>

namespace test_weakref
{
template <bool UseGeneric>
class weakref_suite : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if constexpr(!UseGeneric)
            ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

        m_engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(m_engine);
        asbind_test::setup_exception_translator(m_engine);
        asbind_test::setup_script_assertion(m_engine);
        asbind_test::register_weakref(m_engine, UseGeneric);
    }

    void TearDown() override
    {
        m_engine.reset();
    }

public:
    [[nodiscard]]
    asbind20::engine_pointer get_engine() const
    {
        return m_engine.get();
    }

private:
    asbind20::script_engine m_engine;
};

static void check_script_class(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "test_weakref");
    ASSERT_THAT(m, ::testing::NotNull());

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
        "    assert(cr.get() !is null);\n"
        "    const test@ ct = cr;\n"
        "    assert(ct !is null);\n"
        "    assert(cr !is null);\n"
        "    assert(cr is ct);\n"
        "    @cr = null;\n"
        "    assert(cr is null);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByDecl("void main()");
    ASSERT_THAT(f, ::testing::NotNull());

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<void>(ctx, f);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
}

class host_weakref_support
{
public:
    void addref()
    {
        m_counter.inc();
    }

    void release()
    {
        if(static_cast<int>(m_counter) == 1 && m_weakref_flag && !m_weakref_flag.get_flag())
            m_weakref_flag.set_flag(true);

        if(m_counter.dec() == 0)
            delete this;
    }

    AS_NAMESPACE_QUALIFIER asILockableSharedBool* get_weakref_flag()
    {
        if(!m_weakref_flag)
        {
            std::lock_guard lock(asbind20::script_lock);
            // Double-check because other thread may have created it
            if(!m_weakref_flag)
                m_weakref_flag = asbind20::make_lockable_shared_bool();
        }

        return m_weakref_flag.get();
    }

protected:
    ~host_weakref_support() = default;

private:
    asbind20::atomic_counter m_counter;
    asbind20::lockable_shared_bool m_weakref_flag;
};

template <bool UseGeneric>
static void register_host_weakref_support(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    ref_class<host_weakref_support, UseGeneric>(engine, "host_weakref_support")
        .default_factory()
        .addref(fp<&host_weakref_support::addref>)
        .release(fp<&host_weakref_support::release>)
        .get_weakref_flag(fp<&host_weakref_support::get_weakref_flag>);
}

static void check_host_class(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "test_host_weakref");
    ASSERT_THAT(m, ::testing::NotNull());

    // Copied from the official test case in test_addon_weakref.cpp
    m->AddScriptSection(
        "test_host_weakref",
        "weakref<host_weakref_support> r;\n"
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
    ASSERT_THAT(f, ::testing::NotNull());

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<void*>(ctx, f);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
}

#ifndef ASBIND20_NO_EXCEPTIONS
// Throws an exception when trying to obtain the flag
class host_weakref_ex : public host_weakref_support
{
public:
    AS_NAMESPACE_QUALIFIER asILockableSharedBool* get_weakref_flag()
    {
        throw asbind_test::expected_ex{};
    }
};

template <bool UseGeneric>
static void register_host_weakref_ex(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    ref_class<host_weakref_ex, UseGeneric>(engine, "host_weakref_ex")
        .default_factory()
        .addref(fp<&host_weakref_ex::addref>)
        .release(fp<&host_weakref_ex::release>)
        .get_weakref_flag(fp<&host_weakref_ex::get_weakref_flag>);
}

static void check_host_weakref_ex(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "test_host_weakref_ex");
    ASSERT_THAT(m, ::testing::NotNull());

    m->AddScriptSection(
        "test_host_weakref_ex",
        "weakref<host_weakref_ex> r;\n"
        "host_weakref_ex@ get()\n"
        "{\n"
        "    host_weakref_ex@ host_class;\n"
        "    @host_class = host_weakref_ex();\n"
        "    assert(host_class !is null);\n"
        "    @r = @host_class;\n"
        "    assert(false);\n"
        "    return host_class;\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("get");
    ASSERT_THAT(f, ::testing::NotNull());

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<void*>(ctx, f);
    EXPECT_FALSE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);
}
#endif // !ASBIND20_NO_EXCEPTIONS
} // namespace test_weakref

using WeakrefNative = test_weakref::weakref_suite<false>;
using WeakrefGeneric = test_weakref::weakref_suite<true>;

TEST_F(WeakrefNative, ScriptClass)
{
    test_weakref::check_script_class(get_engine());
}

TEST_F(WeakrefGeneric, ScriptClass)
{
    test_weakref::check_script_class(get_engine());
}

TEST_F(WeakrefNative, HostClass)
{
    test_weakref::register_host_weakref_support<false>(get_engine());
    test_weakref::check_host_class(get_engine());
}

TEST_F(WeakrefGeneric, HostClass)
{
    test_weakref::register_host_weakref_support<true>(get_engine());
    test_weakref::check_host_class(get_engine());
}

#ifndef ASBIND20_NO_EXCEPTIONS

TEST_F(WeakrefNative, HostClassException)
{
    // AngelScript 2.38's x64 native calling convention trampoline cannot
    // unwind the C++ exception thrown by the host weakref flag callback when
    // AddressSanitizer is enabled, so the exception is not translated into a
    // script exception (see TestArrayNative.ExceptionSafety as well).
    ASBIND_TEST_SKIP_IF_ADDRESS_SANITIZER();

    test_weakref::register_host_weakref_ex<false>(get_engine());
    test_weakref::check_host_weakref_ex(get_engine());
}

TEST_F(WeakrefGeneric, HostClassException)
{
    test_weakref::register_host_weakref_ex<true>(get_engine());
    test_weakref::check_host_weakref_ex(get_engine());
}

#endif // !ASBIND20_NO_EXCEPTIONS
