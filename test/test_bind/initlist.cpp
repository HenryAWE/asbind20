#include <asbind20/ext/assert.hpp>
#include <shared_test_lib.hpp>

namespace test_bind
{
template <bool UseGeneric>
void register_vector_of_ints(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    using vector_t = std::vector<int>;

    value_class<vector_t, UseGeneric>(engine, "vec_ints")
        .behaviours_by_traits()
        .template list_constructor<int, policies::as_iterators>("repeat int");
}

void check_vector_ints(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule("test_vec_ints", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_vec_ints",
        "vec_ints create0() { return {}; }\n"
        "vec_ints create1() { return {1}; }\n"
        "vec_ints create2() { return {1, 2}; }\n"
        "vec_ints create3() { return {1, 2, 3}; }\n"
    );
    ASSERT_GE(m->Build(), 0);

    auto create = [&](int idx) -> std::vector<int>
    {
        std::string decl = asbind20::string_concat("vec_ints create", std::to_string(idx), "()");
        auto* f = m->GetFunctionByDecl(decl.c_str());
        if(!f)
        {
            EXPECT_TRUE(f) << decl << ": not found";
            return {};
        }

        asbind20::request_context ctx(engine);
        return asbind20::script_invoke<std::vector<int>>(ctx, f).value();
    };

    {
        auto v0 = create(0);
        EXPECT_TRUE(v0.empty());
    }

    {
        auto v1 = create(1);
        EXPECT_EQ(v1.at(0), 1);
        EXPECT_EQ(v1.size(), 1);
    }

    {
        auto v1 = create(2);
        EXPECT_EQ(v1.at(0), 1);
        EXPECT_EQ(v1.at(1), 2);
        EXPECT_EQ(v1.size(), 2);
    }

    {
        auto v1 = create(3);
        EXPECT_EQ(v1.at(0), 1);
        EXPECT_EQ(v1.at(1), 2);
        EXPECT_EQ(v1.at(2), 3);
        EXPECT_EQ(v1.size(), 3);
    }
}

struct my_vec_ints
{
    my_vec_ints() = default;

    ~my_vec_ints() = default;

    my_vec_ints(int* ptr, std::size_t count)
    {
        for(std::size_t i = 0; i < count; ++i)
            data.push_back(ptr[i]);
    }

    std::vector<int> data;
};

template <bool UseGeneric>
void register_my_vec_ints(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    using vector_t = my_vec_ints;

    value_class<vector_t, UseGeneric>(engine, "my_vec_ints")
        .behaviours_by_traits()
        .template list_constructor<int, policies::pointer_and_size>("repeat int");
}

void check_my_vec_ints(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule("test_my_vec_ints", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_my_vec_ints",
        "my_vec_ints create0() { return {}; }\n"
        "my_vec_ints create1() { return {1}; }\n"
        "my_vec_ints create2() { return {1, 2}; }\n"
        "my_vec_ints create3() { return {1, 2, 3}; }\n"
    );
    ASSERT_GE(m->Build(), 0);

    auto create = [&](int idx) -> my_vec_ints
    {
        std::string decl = asbind20::string_concat("my_vec_ints create", std::to_string(idx), "()");
        auto* f = m->GetFunctionByDecl(decl.c_str());
        if(!f)
        {
            EXPECT_TRUE(f) << decl << ": not found";
            return {};
        }

        asbind20::request_context ctx(engine);
        return asbind20::script_invoke<my_vec_ints>(ctx, f).value();
    };

    {
        auto v0 = create(0);
        EXPECT_TRUE(v0.data.empty());
    }

    {
        auto v1 = create(1);
        EXPECT_EQ(v1.data.at(0), 1);
        EXPECT_EQ(v1.data.size(), 1);
    }

    {
        auto v1 = create(2);
        EXPECT_EQ(v1.data.at(0), 1);
        EXPECT_EQ(v1.data.at(1), 2);
        EXPECT_EQ(v1.data.size(), 2);
    }

    {
        auto v1 = create(3);
        EXPECT_EQ(v1.data.at(0), 1);
        EXPECT_EQ(v1.data.at(1), 2);
        EXPECT_EQ(v1.data.at(2), 3);
        EXPECT_EQ(v1.data.size(), 3);
    }
}
} // namespace test_bind

TEST(initlist, value_as_iterators)
{
    auto engine = asbind20::make_script_engine();

    asbind20::global(engine)
        .message_callback(
            +[](const asSMessageInfo* msg, void*)
            {
                std::cerr << msg->message << std::endl;
            }
        );

    test_bind::register_vector_of_ints<true>(engine);
    test_bind::check_vector_ints(engine);
}

TEST(initlist, value_pointer_size)
{
    auto engine = asbind20::make_script_engine();

    asbind20::global(engine)
        .message_callback(
            +[](const asSMessageInfo* msg, void*)
            {
                std::cerr << msg->message << std::endl;
            }
        );

    test_bind::register_my_vec_ints<true>(engine);
    test_bind::check_my_vec_ints(engine);
}
