#include <shared_test_lib.hpp>

#ifdef ASBIND20_HAS_AS_FOREACH

#    include <asbind20/foreach.hpp>

namespace test_bind
{
class int_generator
{
public:
    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = int;
        using reference = int; // Placeholder to make std::iterator_traits happy
        using difference_type = std::ptrdiff_t;

        iterator() = default;
        iterator(const iterator&) = default;
        iterator& operator=(const iterator&) = default;

        bool operator==(const iterator&) const noexcept = default;

        iterator(int val)
            : value(val) {}

        int operator*() const
        {
            return value;
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

static inline int_generator g_int_gen{};

template <bool Const, bool Explicit, bool UseGeneric>
void register_int_generator(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<int_generator::iterator, UseGeneric> iter(
        engine,
        "int_generator_iterator",
        AS_NAMESPACE_QUALIFIER asOBJ_POD | AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS
    );
    iter
        .default_constructor()
        .opAssign()
        .copy_constructor()
        .destructor();

    ref_class<int_generator, UseGeneric> c(
        engine,
        "int_generator",
        AS_NAMESPACE_QUALIFIER asOBJ_NOCOUNT
    );
    if constexpr(Const)
    {
        if constexpr(Explicit)
            c.use(const_foreach(iter));
        else
            c.use(const_foreach(iter)->template value<int>());
    }
    else
    {
        if constexpr(Explicit)
            c.use(foreach(iter));
        else
            c.use(foreach(iter)->template value<int>());
    }
    global(engine)
        .property("int_generator int_gen", g_int_gen);
}

auto prepare_int_seq_test(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
    -> AS_NAMESPACE_QUALIFIER asIScriptFunction*
{
    auto* m = engine->GetModule(
        "", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "test_int_seq",
        "int run_foreach()\n"
        "{\n"
        "    int result = 0;\n"
        "    foreach(int i : int_gen)\n"
        "        result += i;\n"
        "    return result;\n"
        "}"
    );
    EXPECT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("run_foreach");
    EXPECT_TRUE(f);
    return f;
}
} // namespace test_bind

TEST(test_foreach, int_seq_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    test_bind::register_int_generator<true, false, true>(engine);
    auto* f = test_bind::prepare_int_seq_test(engine);

    request_context ctx(engine);
    auto result = script_invoke<int>(ctx, f);
    EXPECT_TRUE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.value(), 10 + 11 + 12 + 13 + 14);
}

TEST(test_foreach, int_seq_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    test_bind::register_int_generator<true, false, false>(engine);
    auto* f = test_bind::prepare_int_seq_test(engine);

    request_context ctx(engine);
    auto result = script_invoke<int>(ctx, f);
    EXPECT_TRUE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.value(), 10 + 11 + 12 + 13 + 14);
}

TEST(test_foreach, int_seq_explicit_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    test_bind::register_int_generator<true, true, true>(engine);
    auto* f = test_bind::prepare_int_seq_test(engine);

    request_context ctx(engine);
    auto result = script_invoke<int>(ctx, f);
    EXPECT_TRUE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.value(), 10 + 11 + 12 + 13 + 14);
}

TEST(test_foreach, int_seq_explicit_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    test_bind::register_int_generator<true, true, false>(engine);
    auto* f = test_bind::prepare_int_seq_test(engine);

    request_context ctx(engine);
    auto result = script_invoke<int>(ctx, f);
    EXPECT_TRUE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.value(), 10 + 11 + 12 + 13 + 14);
}

#endif
