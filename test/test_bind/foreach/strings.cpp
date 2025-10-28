#include <shared_test_lib.hpp>
#include <asbind20/ext/stdstring.hpp>

#ifdef ASBIND20_HAS_AS_FOREACH

#    include <asbind20/foreach.hpp>

namespace test_bind
{
class string_generator
{
public:
    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::string;
        using reference = std::string; // Placeholder to make std::iterator_traits happy
        using difference_type = std::ptrdiff_t;

        iterator() = default;
        iterator(const iterator&) = default;
        iterator& operator=(const iterator&) = default;

        bool operator==(const iterator&) const noexcept = default;

        iterator(int val)
            : value(val) {}

        std::string operator*() const
        {
            return std::to_string(value);
        }

        iterator& operator++()
        {
            ++value;
            return *this;
        }

        void operator++(int)
        {
            ++*this;
        }

        int value;
    };

    static_assert(std::input_iterator<iterator>);

    struct sentinel
    {
        int value;

        friend bool operator==(const iterator& lhs, const sentinel& rhs) noexcept
        {
            return lhs.value == rhs.value;
        }
    };

    iterator begin() const
    {
        return iterator(10);
    }

    sentinel end() const
    {
        return sentinel{.value = 15};
    }
};

class foreach_string_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_engine = asbind20::make_script_engine();
    }

    void TearDown() override
    {
        m_engine.reset();
    }

    auto get_engine() const
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine.get();
    }

    template <bool Const, bool UseGeneric>
    void prepare_env()
    {
        using namespace asbind20;

        asbind_test::setup_message_callback(m_engine, true);
        ext::register_std_string(m_engine, UseGeneric);

        value_class<string_generator::iterator, UseGeneric> iter(
            m_engine,
            "int_generator_iterator",
            AS_NAMESPACE_QUALIFIER asOBJ_POD | AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS
        );
        iter
            .default_constructor()
            .opAssign()
            .copy_constructor()
            .destructor();

        ref_class<string_generator, UseGeneric> c(
            m_engine,
            "string_generator",
            AS_NAMESPACE_QUALIFIER asOBJ_NOCOUNT
        );
        if constexpr(Const)
            c.use(const_foreach(iter)->template value<std::string>("string"));
        else
            c.use(foreach(iter)->template value<std::string>("string"));

        global(m_engine)
            .property("string_generator str_gen", m_instance);
    }

    auto get_script_func() const
        -> AS_NAMESPACE_QUALIFIER asIScriptFunction*
    {
        auto m = m_engine->GetModule(
            "foreach_string", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        m->AddScriptSection(
            "foreach_string",
            "string run_foreach()\n"
            "{\n"
            "    string result;\n"
            "    foreach(auto i : str_gen)\n"
            "        result = result + i;\n"
            "    return result;\n"
            "}"
        );

        if(m->Build() < 0)
        {
            ADD_FAILURE() << "Failed to build script";
            return nullptr;
        }

        auto* f = m->GetFunctionByName("run_foreach");
        if(f == nullptr)
        {
            ADD_FAILURE() << "Failed to get script function";
            return nullptr;
        }
        return f;
    }

private:
    asbind20::script_engine m_engine;
    static string_generator m_instance;
};

string_generator foreach_string_suite::m_instance{};
} // namespace test_bind

using test_bind::foreach_string_suite;

TEST_F(foreach_string_suite, run_script_generic)
{
    prepare_env<false, true>();
    auto* f = get_script_func();

    asbind20::request_context ctx(get_engine());
    auto result = asbind20::script_invoke<std::string>(ctx, f);

    ASSERT_TRUE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.value(), "1011121314");
}

TEST_F(foreach_string_suite, run_script_native)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    prepare_env<false, false>();
    auto* f = get_script_func();

    asbind20::request_context ctx(get_engine());
    auto result = asbind20::script_invoke<std::string>(ctx, f);

    ASSERT_TRUE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.value(), "1011121314");
}

TEST_F(foreach_string_suite, const_run_script_generic)
{
    prepare_env<true, true>();
    auto* f = get_script_func();

    asbind20::request_context ctx(get_engine());
    auto result = asbind20::script_invoke<std::string>(ctx, f);

    ASSERT_TRUE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.value(), "1011121314");
}

TEST_F(foreach_string_suite, const_run_script_native)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    prepare_env<true, false>();
    auto* f = get_script_func();

    asbind20::request_context ctx(get_engine());
    auto result = asbind20::script_invoke<std::string>(ctx, f);

    ASSERT_TRUE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.value(), "1011121314");
}


#endif
