#include <gtest/gtest.h>
#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>

namespace test_gc
{
static bool val_gc_template_callback(
    AS_NAMESPACE_QUALIFIER asITypeInfo* ti, bool& no_gc
)
{
    int subtype_id = ti->GetSubTypeId();
    if(asbind20::is_void_type(subtype_id))
        return false;

    if(asbind20::is_primitive_type(subtype_id))
        no_gc = true;
    else
        no_gc = !asbind20::type_requires_gc(ti->GetSubType());

    return true;
}

class val_gc
{
public:
    val_gc(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        : m_ti(ti) {}

    val_gc(const val_gc& other)
        : m_ti(other.m_ti)
    {
        if(other.has_value())
        {
            int type_id = m_ti->GetSubTypeId();
            bool no_ex = asbind20::container::single::copy_construct(
                m_data,
                m_ti->GetEngine(),
                type_id,
                asbind20::container::single::data_address(m_data, type_id)
            );
            if(!no_ex) [[unlikely]]
                return;
            m_has_value = no_ex;
        }
    }

    ~val_gc()
    {
        reset();
    }

    bool has_value() const noexcept
    {
        return m_has_value;
    }

    void assign(const void* val)
    {
        EXPECT_NE(m_ti, nullptr);

        if(has_value())
        {
            asbind20::container::single::copy_assign_from(
                m_data, m_ti->GetEngine(), m_ti->GetSubTypeId(), val
            );
        }
        else
        {
            m_has_value = asbind20::container::single::copy_construct(
                m_data, m_ti->GetEngine(), m_ti->GetSubTypeId(), val
            );
        }
    }

    const void* value() const
    {
        if(!has_value())
        {
            asbind20::set_script_exception("bad optional access");
            return nullptr;
        }

        return asbind20::container::single::data_address(
            m_data, m_ti->GetSubTypeId()
        );
    }

    void reset()
    {
        if(!m_has_value)
            return;

        asbind20::container::single::destroy(
            m_data, m_ti->GetEngine(), m_ti->GetSubTypeId()
        );
        m_has_value = false;
    }

    void enum_refs_impl(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
    {
        EXPECT_TRUE((m_ti->GetFlags() & AS_NAMESPACE_QUALIFIER asOBJ_GC))
            << "m_ti->GetFlags(): " << m_ti->GetFlags();
        EXPECT_EQ(engine, m_ti->GetEngine());

        asbind20::container::single::enum_refs(
            m_data, m_ti->GetSubType()
        );
    }

    friend void val_gc_enum_refs(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, val_gc& this_
    );

private:
    void release_refs_impl(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
    {
        EXPECT_EQ(engine, m_ti->GetEngine());
        reset();
    }

public:
    friend void val_gc_release_refs(
        val_gc& this_, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
    );

    template <bool UseGeneric>
    friend void register_val_gc(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine);

private:
    asbind20::script_typeinfo m_ti;

    asbind20::container::single::data_type m_data;
    bool m_has_value = false;
};

// Object last
void val_gc_enum_refs(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, val_gc& this_
)
{
    this_.enum_refs_impl(engine);
}

// Object first
void val_gc_release_refs(val_gc& this_, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    this_.release_refs_impl(engine);
}

template <bool UseGeneric>
void register_val_gc(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    template_value_class<val_gc, UseGeneric>(engine, "val_gc<T>", AS_NAMESPACE_QUALIFIER asOBJ_GC)
        .template_callback(fp<&val_gc_template_callback>)
        .default_constructor()
        .destructor()
        .enum_refs(fp<&val_gc_enum_refs>)
        .release_refs(fp<&val_gc_release_refs>)
        .property("const bool has_value", &val_gc::m_has_value)
        .method("const T& get_value() const property", fp<overload_cast<>(&val_gc::value, const_)>)
        .method("void set_value(const T&in) property", fp<&val_gc::assign>);
}
} // namespace test_gc

static constexpr char optional_gc_test_script[] = R"(bool test0()
{
    output_stat();
    val_gc<int> o;
    return !o.has_value;
}

class foo
{
    val_gc<foo@> ref;
}

bool test1()
{
    output_stat();
    foo f;
    @f.ref.value = @f;
    output_stat();
    return f.ref.has_value;
}
)";

namespace test_gc
{
template <bool UseGeneric>
class basic_value_gc_test : public ::testing::Test
{
public:
    void SetUp() override
    {
        if constexpr(!UseGeneric)
            ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

        m_engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(m_engine, true);

        asbind_test::setup_script_assertion(m_engine);
        register_val_gc<UseGeneric>(m_engine);

        asbind20::global<true>(m_engine)
            .function(
                "void output_stat()",
                []() -> void
                {
                    auto* ctx = asbind20::current_context();
                    ASSERT_TRUE(ctx);

                    auto* f = ctx->GetFunction();
                    const char* section = "(null)";
                    int line = ctx->GetLineNumber(0, nullptr, &section);
                    std::cerr
                        << '['
                        << section << ':' << line
                        << " (" << f->GetName()
                        << ")] ";
                    asbind_test::output_gc_statistics(
                        std::cerr,
                        ctx->GetEngine(),
                        ';'
                    );
                }
            );
    }

    void TearDown() override
    {
        m_engine.reset();
    }

    auto get_engine() const noexcept
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine.get();
    }

    void run_script()
    {
        auto* m = m_engine->GetModule(
            "optional_gc_test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );

        m->AddScriptSection(
            "optional_gc_test_script",
            optional_gc_test_script
        );
        ASSERT_GE(m->Build(), 0);

        constexpr int max_test_idx = 1;
        for(int i = 0; i <= max_test_idx; ++i)
        {
            std::string decl = asbind20::string_concat(
                "bool test", std::to_string(i), "()"
            );
            SCOPED_TRACE("Decl: " + decl);
            auto* f = m->GetFunctionByDecl(decl.c_str());
            EXPECT_NE(f, nullptr);
            if(!f)
                continue;

            asbind20::request_context ctx(m_engine);
            auto result = asbind20::script_invoke<bool>(ctx, f);
            EXPECT_TRUE(asbind_test::result_has_value(result));
            EXPECT_TRUE(result.value());
        }

        std::cerr << "After tests: ";
        asbind_test::output_gc_statistics(
            std::cerr, m_engine, ';'
        );

        m_engine->GarbageCollect(AS_NAMESPACE_QUALIFIER asGC_FULL_CYCLE);

        {
            auto collected = asbind20::debugging::get_gc_statistics(m_engine);
            auto [curr_size, total_destroyed, total_detected, new_obj, total_new_destroyed] =
                collected;

            EXPECT_EQ(curr_size, 0);
        }

        std::cerr << "Collected: ";
        asbind_test::output_gc_statistics(
            std::cerr, m_engine, ';'
        );
    }

private:
    asbind20::script_engine m_engine;
};
} // namespace test_gc

using ValueGCGeneric = test_gc::basic_value_gc_test<true>;
using ValueGCNative = test_gc::basic_value_gc_test<false>;

TEST_F(ValueGCGeneric, RunScript)
{
    this->run_script();
}

TEST_F(ValueGCNative, RunScript)
{
    this->run_script();
}
