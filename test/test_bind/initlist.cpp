#include <asbind20/ext/assert.hpp>
#include <shared_test_lib.hpp>

namespace test_bind
{
static void setup_initlist_test_env(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    asbind_test::setup_message_callback(engine);
}

template <bool UseGeneric>
static void register_vector_of_ints(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    using vector_t = std::vector<int>;

    value_class<vector_t, UseGeneric>(engine, "vec_ints")
        .behaviours_by_traits()
        .template list_constructor<int>("repeat int", use_policy<policies::as_iterators>);
}

static void check_vector_ints(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
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

// Multipurpose
struct my_vec_ints
{
    my_vec_ints() = default;

    ~my_vec_ints() = default;

    my_vec_ints(asbind20::script_init_list_repeat list)
        : my_vec_ints((int*)list.data(), list.size())
    {}

    my_vec_ints(int* ptr, std::size_t count)
    {
        for(std::size_t i = 0; i < count; ++i)
            data.push_back(ptr[i]);
    }

#ifdef ASBIND20_HAS_CONTAINERS_RANGES

    template <typename Range>
    my_vec_ints(std::from_range_t, Range&& r)
        : data(std::from_range, std::forward<Range>(r))
    {}

#endif

    std::vector<int> data;
};

template <asbind20::policies::initialization_list_policy Policy, bool UseGeneric>
static void register_my_vec_ints(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    using vector_t = my_vec_ints;

    value_class<vector_t, UseGeneric>(engine, "my_vec_ints")
        .behaviours_by_traits()
        .template list_constructor<int>("repeat int", use_policy<Policy>);
}

static void check_my_vec_ints(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
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

struct from_init_list
{
    from_init_list() = default;

    from_init_list(std::initializer_list<int> il)
        : data(il) {}

    std::vector<int> data;
};

template <bool UseGeneric>
static void register_from_init_list(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<from_init_list, UseGeneric>(engine, "from_init_list")
        .behaviours_by_traits()
        .template list_constructor<int>("repeat int", use_policy<policies::as_initializer_list>);
}

static void check_from_init_list(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule("test_from_init_list", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_from_init_list",
        "from_init_list create0() { return {}; }\n"
        "from_init_list create1() { return {1}; }\n"
        "from_init_list create2() { return {1, 2}; }\n"
        "from_init_list create3() { return {1, 2, 3}; }\n"
    );
    ASSERT_GE(m->Build(), 0);

    auto create = [&](int idx) -> from_init_list
    {
        std::string decl = asbind20::string_concat("from_init_list create", std::to_string(idx), "()");
        auto* f = m->GetFunctionByDecl(decl.c_str());
        if(!f)
        {
            EXPECT_TRUE(f) << decl << ": not found";
            return {};
        }

        asbind20::request_context ctx(engine);
        return asbind20::script_invoke<from_init_list>(ctx, f).value();
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

struct from_span
{
    from_span() = default;

    from_span(std::span<int> sp)
        : data(sp.begin(), sp.end()) {}

    std::vector<int> data;
};

template <bool UseGeneric>
static void register_from_span(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<from_span, UseGeneric>(engine, "from_span")
        .behaviours_by_traits()
        .template list_constructor<int>("repeat int", use_policy<policies::as_span>);
}

static void check_from_span(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule("test_from_span", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_from_span",
        "from_span create0() { return {}; }\n"
        "from_span create1() { return {1}; }\n"
        "from_span create2() { return {1, 2}; }\n"
        "from_span create3() { return {1, 2, 3}; }\n"
    );
    ASSERT_GE(m->Build(), 0);

    auto create = [&](int idx) -> from_span
    {
        std::string decl = asbind20::string_concat("from_span create", std::to_string(idx), "()");
        auto* f = m->GetFunctionByDecl(decl.c_str());
        if(!f)
        {
            EXPECT_TRUE(f) << decl << ": not found";
            return {};
        }

        asbind20::request_context ctx(engine);
        return asbind20::script_invoke<from_span>(ctx, f).value();
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

TEST(initlist_native, value_as_iterators)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_vector_of_ints<false>(engine);
    test_bind::check_vector_ints(engine);
}

TEST(initlist_generic, value_as_iterators)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_vector_of_ints<true>(engine);
    test_bind::check_vector_ints(engine);
}

TEST(initlist_native, value_repeat_list_proxy)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_my_vec_ints<asbind20::policies::repeat_list_proxy, false>(
        engine
    );
    test_bind::check_my_vec_ints(engine);
}

TEST(initlist_generic, value_repeat_list_proxy)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_my_vec_ints<asbind20::policies::repeat_list_proxy, true>(
        engine
    );
    test_bind::check_my_vec_ints(engine);
}

TEST(initlist_native, value_pointer_size)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_my_vec_ints<asbind20::policies::repeat_list_proxy, false>(
        engine
    );
    test_bind::check_my_vec_ints(engine);
}

TEST(initlist_generic, value_pointer_size)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_my_vec_ints<asbind20::policies::repeat_list_proxy, true>(
        engine
    );
    test_bind::check_my_vec_ints(engine);
}

TEST(initlist_native, value_as_initializer_list)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_from_init_list<false>(engine);
    test_bind::check_from_init_list(engine);
}

TEST(initlist_generic, value_as_initializer_list)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_from_init_list<true>(engine);
    test_bind::check_from_init_list(engine);
}

TEST(initlist_native, value_as_span)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_from_span<false>(engine);
    test_bind::check_from_span(engine);
}

TEST(initlist_generic, value_as_span)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_from_span<true>(engine);
    test_bind::check_from_span(engine);
}

#ifdef ASBIND20_HAS_CONTAINERS_RANGES

TEST(initlist_native, value_from_range)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_my_vec_ints<asbind20::policies::as_from_range, false>(
        engine
    );
    test_bind::check_my_vec_ints(engine);
}

TEST(initlist_generic, value_from_range)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_my_vec_ints<asbind20::policies::as_from_range, true>(
        engine
    );
    test_bind::check_my_vec_ints(engine);
}

#endif

namespace test_bind
{
class ref_initlist_test_base
{
public:
    virtual ~ref_initlist_test_base() = default;

    void addref()
    {
        ++m_counter;
    }

    void release()
    {
        assert(m_counter >= 0);
        if(--m_counter == 0)
            delete this;
    }

    int use_count() const noexcept
    {
        return m_counter;
    }

private:
    int m_counter = 1;
};

class ref_test_apply : public ref_initlist_test_base
{
public:
    ref_test_apply(int x, int y)
        : data{x, y} {}

    int data[2];
};

template <bool UseGeneric>
void register_ref_test_apply(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;
    ref_class<ref_test_apply, UseGeneric>(engine, "ref_test_apply")
        .addref(fp<&ref_test_apply::addref>)
        .release(fp<&ref_test_apply::release>)
        .template list_factory<int>("int,int", use_policy<policies::apply_to<2>>);
}

void check_ref_test_apply(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule("ref_test_apply", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "ref_test_apply",
        "ref_test_apply@ create0() { return {0, 0}; }\n"
        "ref_test_apply@ create1() { return {10, 13}; }"
    );
    ASSERT_GE(m->Build(), 0);

    auto create = [&](int idx) -> ref_test_apply*
    {
        std::string decl = asbind20::string_concat("ref_test_apply@ create", std::to_string(idx), "()");
        auto* f = m->GetFunctionByDecl(decl.c_str());

        if(!f)
        {
            EXPECT_TRUE(f) << decl << ": not found";
            return nullptr;
        }

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<ref_test_apply*>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));

        auto* val = result.value();
        EXPECT_EQ(val->use_count(), 1);
        val->addref();
        return val;
    };

    {
        auto* val = create(0);
        ASSERT_EQ(val->use_count(), 1);

        EXPECT_EQ(val->data[0], 0);
        EXPECT_EQ(val->data[1], 0);

        val->release();
    }

    {
        auto* val = create(1);
        ASSERT_EQ(val->use_count(), 1);

        EXPECT_EQ(val->data[0], 10);
        EXPECT_EQ(val->data[1], 13);

        val->release();
    }
}

// Multipurpose
class ref_test_vector : public ref_initlist_test_base
{
public:
    template <typename Iterator>
    ref_test_vector(Iterator start, Iterator sentinel)
        : data(start, sentinel)
    {}

    ref_test_vector(std::initializer_list<int> il)
        : data(il) {}

    ref_test_vector(std::span<int> sp)
        : ref_test_vector(sp.begin(), sp.end())
    {}

    ref_test_vector(int* data, std::size_t count)
        : ref_test_vector(std::span<int>(data, count))
    {}

    ref_test_vector(asbind20::script_init_list_repeat list)
        : ref_test_vector((int*)list.data(), list.size())
    {}


#ifdef ASBIND20_HAS_CONTAINERS_RANGES

    template <typename Range>
    ref_test_vector(std::from_range_t, Range&& r)
        : data(std::from_range, std::forward<Range>(r))
    {}

#endif

    std::vector<int> data;
};

template <asbind20::policies::initialization_list_policy IListPolicy, bool UseGeneric>
static void register_ref_test_vector_with(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;
    ref_class<ref_test_vector, UseGeneric>(engine, "ref_test_vector")
        .addref(fp<&ref_test_apply::addref>)
        .release(fp<&ref_test_apply::release>)
        .template list_factory<int>("repeat int", use_policy<IListPolicy>);
}

static void check_ref_test_vector(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule("ref_test_vector", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "ref_test_vector",
        "ref_test_vector@ create0() { return {}; }\n"
        "ref_test_vector@ create1() { return {1013}; }\n"
        "ref_test_vector@ create2() { return {10, 13}; }"
    );
    ASSERT_GE(m->Build(), 0);

    auto create = [&](int idx) -> ref_test_vector*
    {
        std::string decl = asbind20::string_concat("ref_test_vector@ create", std::to_string(idx), "()");
        auto* f = m->GetFunctionByDecl(decl.c_str());

        if(!f)
        {
            EXPECT_TRUE(f) << decl << ": not found";
            return nullptr;
        }

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<ref_test_vector*>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));

        auto* val = result.value();
        EXPECT_EQ(val->use_count(), 1);
        val->addref();
        return val;
    };

    {
        auto* val = create(0);
        ASSERT_EQ(val->use_count(), 1);

        EXPECT_EQ(val->data.size(), 0);

        val->release();
    }

    {
        auto* val = create(1);
        ASSERT_EQ(val->use_count(), 1);

        EXPECT_EQ(val->data.at(0), 1013);
        EXPECT_EQ(val->data.size(), 1);

        val->release();
    }

    {
        auto* val = create(2);
        ASSERT_EQ(val->use_count(), 1);

        EXPECT_EQ(val->data.at(0), 10);
        EXPECT_EQ(val->data.at(1), 13);
        EXPECT_EQ(val->data.size(), 2);

        val->release();
    }
}
} // namespace test_bind

TEST(initlist_native, ref_apply_to)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_apply<false>(engine);
    test_bind::check_ref_test_apply(engine);
}

TEST(initlist_generic, ref_apply_to)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_apply<true>(engine);
    test_bind::check_ref_test_apply(engine);
}

TEST(initlist_native, ref_test_vector)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_iterators, false>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(initlist_generic, ref_test_vector)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_iterators, true>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(initlist_native, ref_test_repeat_list_proxy)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::repeat_list_proxy, false>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(initlist_generic, ref_test_repeat_list_proxy)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::repeat_list_proxy, true>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(initlist_native, ref_test_pointer_and_size)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::pointer_and_size, false>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(initlist_generic, ref_test_pointer_and_size)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::pointer_and_size, true>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(initlist_native, ref_test_as_initializer_list)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_initializer_list, false>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(initlist_generic, ref_test_as_initializer_list)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_initializer_list, true>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(initlist_native, ref_test_as_span)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_span, false>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(initlist_generic, ref_test_as_span)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_span, true>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

#ifdef ASBIND20_HAS_CONTAINERS_RANGES

TEST(initlist_native, ref_test_from_range)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_from_range, false>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(initlist_generic, ref_test_from_range)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_from_range, true>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

#endif
