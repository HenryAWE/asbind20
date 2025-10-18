#include <shared_test_lib.hpp>

#ifdef ASBIND20_HAS_AS_FOREACH

#    include <asbind20/foreach.hpp>

namespace test_bind
{
class int_generator
{
public:
    struct iterator
    {
        iterator() = default;
        iterator(const iterator&) = default;
        iterator& operator=(const iterator&) = default;

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

        int value;
    };

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

template <bool UseGeneric>
void register_int_generator(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<int_generator::iterator, UseGeneric> iter(
        engine,
        "int_generator_iterator",
        AS_NAMESPACE_QUALIFIER asOBJ_POD
    );
    iter
        .default_constructor()
        .opAssign()
        .copy_constructor()
        .destructor();

    ref_class<int_generator, UseGeneric>(
        engine,
        "int_generator",
        AS_NAMESPACE_QUALIFIER asOBJ_NOCOUNT
    )
        .use(basic_foreach_register<int, decltype(iter), true>(iter));

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

    test_bind::register_int_generator<true>(engine);
    auto* f = test_bind::prepare_int_seq_test(engine);

    request_context ctx(engine);
    auto result = script_invoke<int>(ctx, f);
    EXPECT_TRUE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.value(), 10 + 11 + 12 + 13 + 14);
}

// TODO: native test

#endif
