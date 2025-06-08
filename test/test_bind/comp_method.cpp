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

    bool vexec(void* ref, int type_id)
    {
        if(type_id != AS_NAMESPACE_QUALIFIER asTYPEID_INT32)
            return false;

        int arg = *static_cast<const int*>(ref);
        return m_data == arg;
    }

private:
    int m_data;
};
} // namespace test_bind

// Testing for value types

namespace test_bind
{
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

template <bool UseMP, bool Nontype>
static void register_val_comp(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<val_comp> c(engine, "val_comp");
    c
        .behaviours_by_traits()
        .constructor<int>("int");

    if constexpr(UseMP)
    {
        if constexpr(Nontype)
        {
            c
                .method(
                    "int exec() const",
                    &comp_helper::exec,
                    composite<&val_comp::indirect>()
                )
                .method(
                    "bool vexec(const?&in)",
                    &comp_helper::vexec,
                    composite<&val_comp::indirect>()
                );
        }
        else
        {
            c
                .method(
                    "int exec() const",
                    &comp_helper::exec,
                    composite(&val_comp::indirect)
                )
                .method(
                    "bool vexec(const?&in)",
                    &comp_helper::vexec,
                    composite(&val_comp::indirect)
                );
        }
    }
    else
    {
        if constexpr(Nontype)
        {
            c
                .method(
                    "int exec() const",
                    &comp_helper::exec,
                    composite<offsetof(val_comp, indirect)>()
                )
                .method(
                    "bool vexec(const?&in)",
                    &comp_helper::vexec,
                    composite<offsetof(val_comp, indirect)>()
                );
        }
        else
        {
            c
                .method(
                    "int exec() const",
                    &comp_helper::exec,
                    composite(offsetof(val_comp, indirect))
                )
                .method(
                    "bool vexec(const?&in)",
                    &comp_helper::vexec,
                    composite<offsetof(val_comp, indirect)>()
                );
        }
    }
}

template <bool UseMP, bool Explicitly>
static void register_val_comp(asbind20::use_generic_t, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<val_comp, true> c(engine, "val_comp");
    c
        .behaviours_by_traits()
        .constructor<int>("int");

    if constexpr(UseMP)
    {
        if constexpr(Explicitly)
        {
            c
                .method(
                    use_generic,
                    "int exec() const",
                    fp<&comp_helper::exec>,
                    composite<&val_comp::indirect>()
                )
                .method(
                    use_generic,
                    "bool vexec(const?&in)",
                    fp<&comp_helper::vexec>,
                    composite<&val_comp::indirect>(),
                    var_type<0>
                );
        }
        else
        {
            c
                .method(
                    "int exec() const",
                    fp<&comp_helper::exec>,
                    composite<&val_comp::indirect>()
                )
                .method(
                    "bool vexec(const?&in)",
                    fp<&comp_helper::vexec>,
                    composite<&val_comp::indirect>(),
                    var_type<0>
                );
        }
    }
    else
    {
        if constexpr(Explicitly)
        {
            c
                .method(
                    use_generic,
                    "int exec() const",
                    fp<&comp_helper::exec>,
                    composite<offsetof(val_comp, indirect)>()
                )
                .method(
                    use_generic,
                    "bool vexec(const?&in)",
                    fp<&comp_helper::vexec>,
                    composite<offsetof(val_comp, indirect)>(),
                    var_type<0>
                );
        }
        else
        {
            c
                .method(
                    "int exec() const",
                    fp<&comp_helper::exec>,
                    composite<offsetof(val_comp, indirect)>()
                )
                .method(
                    "bool vexec(const?&in)",
                    fp<&comp_helper::vexec>,
                    composite<offsetof(val_comp, indirect)>(),
                    var_type<0>
                );
        }
    }
}

static void check_val_comp(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "check_val_comp", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "check_val_comp",
        "int test0(int arg)\n"
        "{\n"
        "    val_comp val(arg);\n"
        "    return val.exec();\n"
        "}\n"
        "bool test1()\n"
        "{\n"
        "    val_comp val(21);\n"
        "    return val.vexec(21);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* f = m->GetFunctionByName("test0");

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f, 21);

        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 42);
    }

    {
        auto* f = m->GetFunctionByName("test1");

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<bool>(ctx, f);

        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_TRUE(result.value());
    }
}
} // namespace test_bind

TEST(val_comp, native_offset)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_val_comp<false, false>(engine);
    test_bind::check_val_comp(engine);
}

TEST(val_comp, native_mp)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_val_comp<true, false>(engine);
    test_bind::check_val_comp(engine);
}

TEST(val_comp, native_offset_nontype)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_val_comp<false, true>(engine);
    test_bind::check_val_comp(engine);
}

TEST(val_comp, native_mp_nontype)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_val_comp<true, true>(engine);
    test_bind::check_val_comp(engine);
}

TEST(val_comp, generic_offset)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP();

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_val_comp<false, false>(use_generic, engine);
    test_bind::check_val_comp(engine);
}

TEST(val_comp, generic_mp)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_val_comp<true, false>(use_generic, engine);
    test_bind::check_val_comp(engine);
}

TEST(val_comp, generic_offset_explicitly)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_val_comp<false, true>(use_generic, engine);
    test_bind::check_val_comp(engine);
}

TEST(val_comp, generic_mp_explicitly)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_val_comp<true, true>(use_generic, engine);
    test_bind::check_val_comp(engine);
}

// Testing for reference types

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

template <bool UseGeneric>
static void register_ref_comp(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    ref_class<ref_comp, UseGeneric>(engine, "ref_comp")
        .default_factory()
        .template factory<int>("int")
        .addref(fp<&ref_comp::addref>)
        .release(fp<&ref_comp::release>)
        .method("int exec() const", fp<&comp_helper::exec>, composite<&ref_comp::indirect>());
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
    test_bind::register_ref_comp<false>(engine);
    test_bind::check_ref_comp(engine);
}

TEST(ref_comp, generic_mp)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    test_bind::register_ref_comp<true>(engine);
    test_bind::check_ref_comp(engine);
}
