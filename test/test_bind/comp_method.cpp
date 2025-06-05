#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/assert.hpp>

namespace test_bind
{
class comp_helper
{
public:
    comp_helper(int data)
        : m_data(data) {}

    comp_helper(const comp_helper&) = default;

    comp_helper& operator=(const comp_helper&) = default;

    int exec() const
    {
        return m_data * 2;
    }

private:
    int m_data;
};

class val_comp
{
public:
    val_comp()
        : val_comp(0) {}

    val_comp(int data)
        : indirect(new comp_helper(data)) {}

    val_comp(const val_comp& other)
        : indirect(new comp_helper(*other.indirect)) {}

    ~val_comp()
    {
        delete indirect;
    }

    val_comp& operator=(const val_comp& rhs)
    {
        *indirect = *rhs.indirect;
        return *this;
    }

    comp_helper* const indirect;
};

template <bool UseMP>
void register_val_comp(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<val_comp> c(engine, "val_comp");
    c
        .behaviours_by_traits()
        .constructor<int>("int");

    if constexpr(UseMP)
    {
        c.method(
            "int exec() const",
            &comp_helper::exec,
            composite(&val_comp::indirect)
        );
    }
    else
    {
        c.method(
            "int exec() const",
            &comp_helper::exec,
            composite(offsetof(val_comp, indirect))
        );
    }
}

void check_val_comp(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "check_val_comp", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "check_val_comp",
        "int test(int arg)\n"
        "{\n"
        "    val_comp val(arg);\n"
        "    return val.exec();\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("test");

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<int>(ctx, f, 21);

    EXPECT_TRUE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.value(), 42);
}
} // namespace test_bind

TEST(val_comp, native_offset)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_val_comp<false>(engine);
}

TEST(val_comp, native_mp)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_val_comp<true>(engine);
}

namespace test_bind
{
class ref_comp
{
public:
    ref_comp()
        : ref_comp(0) {}

    ref_comp(int data)
        : indirect(new comp_helper(data)) {}

    ref_comp(const ref_comp&) = delete;

    ~ref_comp()
    {
        delete indirect;
    }

    comp_helper* const indirect;

    void addref()
    {
        ++m_counter;
    }

    void release()
    {
        assert(m_counter != 0);
        --m_counter;
        if(m_counter == 0)
            delete this;
    }

private:
    int m_counter = 1;
};

void register_ref_comp(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    ref_class<ref_comp>(engine, "ref_comp")
        .default_factory()
        .factory<int>("int")
        .addref(fp<&ref_comp::addref>)
        .release(fp<&ref_comp::release>)
        .method("int exec() const", &comp_helper::exec, composite(&ref_comp::indirect));
}

static void check_ref_comp(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "check_ref_comp", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "check_ref_comp",
        "int test(int arg)\n"
        "{\n"
        "    ref_comp val(arg);\n"
        "    return val.exec();\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("test");

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<int>(ctx, f, 21);

    EXPECT_TRUE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.value(), 42);
}
} // namespace test_bind

TEST(ref_comp, native_mp)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_ref_comp(engine);
    test_bind::check_ref_comp(engine);
}
