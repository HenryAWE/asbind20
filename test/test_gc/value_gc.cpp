#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/assert.hpp>
#include <asbind20/ext/vocabulary.hpp>

static constexpr char optional_gc_test_script[] = R"(bool test0()
{
    output_stat();
    optional<int> o = nullopt;
    return !o.has_value;
}

class foo
{
    optional<foo@> ref = nullopt;
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
class basic_optional_gc_test : public ::testing::Test
{
public:
    void SetUp() override
    {
        if constexpr(!UseGeneric)
        {
            if(asbind20::has_max_portability())
                GTEST_SKIP() << "max portability";
        }

        m_engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(m_engine, true);

        namespace ext = asbind20::ext;
        ext::register_script_assert(
            m_engine,
            [](std::string_view msg)
            {
                FAIL() << "optional_gc assertion failed: " << msg;
            }
        );
        ext::register_script_optional(m_engine, UseGeneric);

        asbind20::global<true>(m_engine)
            .function(
                "void output_stat()",
                []() -> void
                {
                    auto* ctx = asbind20::current_context();
                    ASSERT_TRUE(ctx);

                    auto* f = ctx->GetFunction();
                    std::cerr
                        << '['
                        << asbind20::debugging::get_function_section_name(f)
                        << ':'
                        << f->GetName()
                        << "] ";
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
    }

private:
    asbind20::script_engine m_engine;
};
} // namespace test_gc

using optional_gc_generic = test_gc::basic_optional_gc_test<true>;
using optional_gc_native = test_gc::basic_optional_gc_test<false>;

TEST_F(optional_gc_generic, run_script)
{
    this->run_script();
}

TEST_F(optional_gc_native, run_script)
{
    this->run_script();
}
