#include <asbind_test/framework.hpp>
#include <vector>

namespace
{
template <bool UseGeneric>
void register_vector_bool(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    ref_class<std::vector<bool>, UseGeneric> vec_bool(
        engine,
        "vec_bool",
        AS_NAMESPACE_QUALIFIER asOBJ_NOCOUNT
    );

    vec_bool
        .method(
            "void push_back(bool)",
            [](std::vector<bool>& vec, bool val) -> void
            { vec.push_back(val); }
        )
        .method(
            "bool get_opIndex(uint) const property",
            [](const std::vector<bool>& vec, arg_index_type idx) -> bool
            { return vec.at(idx); }
        )
        .method(
            "void set_opIndex(uint, bool) property",
            [](std::vector<bool>& vec, arg_index_type idx, bool val) -> void
            { vec.at(idx) = val; }
        );
}

void check_vector_bool(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "check_vector_bool");

    m->AddScriptSection(
        "check_vector_bool",
        "void test()\n"
        "{\n"
        "    v.push_back(true);\n"
        "    v.push_back(false);\n"
        "    assert(v[0] == true);\n"
        "    assert(v[1] == false);\n"
        "    v[1] = true;\n"
        "    assert(v[1] == true);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("test");
    ASSERT_THAT(f, ::testing::NotNull());

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<void>(ctx, f);

    ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
}
} // namespace

TEST(TestBind, VectorBoolNative)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    asbind_test::setup_script_assertion(engine);

    std::vector<bool> v;
    register_vector_bool<false>(engine);
    global<true>(engine)
        .property("vec_bool v", v);
    check_vector_bool(engine);
}

TEST(TestBind, VectorBoolGeneric)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    asbind_test::setup_script_assertion(engine);

    std::vector<bool> v;
    register_vector_bool<true>(engine);
    global<true>(engine)
        .property("vec_bool v", v);
    check_vector_bool(engine);
}
