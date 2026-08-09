#include <asbind_test/framework.hpp>
#include <asbind20/io/section.hpp>
#include <gmock/gmock.h>

namespace test_bind
{
static void setup_initlist_test_env(asbind20::engine_pointer engine)
{
    asbind_test::setup_message_callback(engine);
    asbind_test::setup_exception_translator(engine);
}

template <typename T, typename DataGetter>
static void check_init_list(
    asbind20::engine_pointer engine,
    std::string_view type_name,
    DataGetter&& data_getter
)
{
    using namespace asbind20;
    using ::testing::ElementsAre;
    using ::testing::IsEmpty;

    auto* m = asbind20::create_module(
        engine, string_concat("test_", type_name)
    );

    io::load_string(
        m,
        string_concat("test_", type_name),
        string_concat(
            type_name,
            " create0() { return {}; }\n",
            type_name,
            " create1() { return {1}; }\n",
            type_name,
            " create2() { return {1, 2}; }\n",
            type_name,
            " create3() { return {1, 2, 3}; }\n"
        )
    );
    ASSERT_GE(m->Build(), 0);

    auto create = [&](int idx) -> T
    {
        std::string decl = asbind20::string_concat(
            type_name, " create", std::to_string(idx), "()"
        );
        auto* f = m->GetFunctionByDecl(decl.c_str());
        if(!f)
        {
            EXPECT_THAT(f, ::testing::NotNull())
                << decl << ": not found";
            return {};
        }
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<T>(ctx, f);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        return result.value();
    };

    auto v0 = create(0);
    EXPECT_THAT(data_getter(v0), IsEmpty());

    auto v1 = create(1);
    EXPECT_THAT(data_getter(v1), ElementsAre(1));

    auto v2 = create(2);
    EXPECT_THAT(data_getter(v2), ElementsAre(1, 2));

    auto v3 = create(3);
    EXPECT_THAT(data_getter(v3), ElementsAre(1, 2, 3));
}

// Multipurpose
struct my_vec_ints
{
    my_vec_ints() = default;

    ~my_vec_ints() = default;

    my_vec_ints(asbind20::script_init_list_repeat list)
        : my_vec_ints(static_cast<int*>(list.data()), list.size())
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

template <bool UseGeneric>
static void register_vector_of_ints(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    using vector_t = std::vector<int>;

    value_class<vector_t, UseGeneric>(engine, "vec_ints")
        .behaviours_by_traits()
        .template list_constructor<int>("repeat int", use_policy<policies::as_iterators>);
}

template <asbind20::policies::initialization_list_policy Policy, bool UseGeneric>
static void register_my_vec_ints(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    using vector_t = my_vec_ints;

    value_class<vector_t, UseGeneric>(engine, "my_vec_ints")
        .behaviours_by_traits()
        .template list_constructor<int>("repeat int", use_policy<Policy>);
}

struct from_init_list
{
    from_init_list() = default;

    from_init_list(std::initializer_list<int> il)
        : data(il) {}

    std::vector<int> data;
};

template <bool UseGeneric>
static void register_from_init_list(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    value_class<from_init_list, UseGeneric>(engine, "from_init_list")
        .behaviours_by_traits()
        .template list_constructor<int>("repeat int", use_policy<policies::as_initializer_list>);
}

struct from_span
{
    from_span() = default;

    from_span(std::span<int> sp)
        : data(sp.begin(), sp.end()) {}

    std::vector<int> data;
};

template <bool UseGeneric>
static void register_from_span(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    value_class<from_span, UseGeneric>(engine, "from_span")
        .behaviours_by_traits()
        .template list_constructor<int>("repeat int", use_policy<policies::as_span>);
}
} // namespace test_bind

TEST(InitListNative, ValueAsIterators)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_vector_of_ints<false>(engine);
    test_bind::check_init_list<std::vector<int>>(
        engine,
        "vec_ints",
        [](auto& v) -> auto&
        { return v; }
    );
}

TEST(InitListGeneric, ValueAsIterators)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_vector_of_ints<true>(engine);
    test_bind::check_init_list<std::vector<int>>(
        engine,
        "vec_ints",
        [](auto& v) -> auto&
        { return v; }
    );
}

TEST(InitListNative, ValueRepeatListProxy)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_my_vec_ints<asbind20::policies::repeat_list_proxy, false>(
        engine
    );
    test_bind::check_init_list<test_bind::my_vec_ints>(
        engine,
        "my_vec_ints",
        [](auto& v) -> auto&
        { return v.data; }
    );
}

TEST(InitListGeneric, ValueRepeatListProxy)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_my_vec_ints<asbind20::policies::repeat_list_proxy, true>(
        engine
    );
    test_bind::check_init_list<test_bind::my_vec_ints>(
        engine,
        "my_vec_ints",
        [](auto& v) -> auto&
        { return v.data; }
    );
}

TEST(InitListNative, ValuePointerAndSize)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_my_vec_ints<asbind20::policies::pointer_and_size, false>(
        engine
    );
    test_bind::check_init_list<test_bind::my_vec_ints>(
        engine, "my_vec_ints", [](auto& v) -> auto&
        { return v.data; }
    );
}

TEST(InitListGeneric, ValuePointerAndSize)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_my_vec_ints<asbind20::policies::pointer_and_size, true>(
        engine
    );
    test_bind::check_init_list<test_bind::my_vec_ints>(
        engine,
        "my_vec_ints",
        [](auto& v) -> auto&
        { return v.data; }
    );
}

TEST(InitListNative, ValueAsInitializerList)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_from_init_list<false>(engine);
    test_bind::check_init_list<test_bind::from_init_list>(
        engine,
        "from_init_list",
        [](auto& v) -> auto&
        { return v.data; }
    );
}

TEST(InitListGeneric, ValueAsInitializerList)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_from_init_list<true>(engine);
    test_bind::check_init_list<test_bind::from_init_list>(
        engine,
        "from_init_list",
        [](auto& v) -> auto&
        { return v.data; }
    );
}

TEST(InitListNative, ValueAsSpan)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_from_span<false>(engine);
    test_bind::check_init_list<test_bind::from_span>(
        engine,
        "from_span",
        [](auto& v) -> auto&
        { return v.data; }
    );
}

TEST(InitListGeneric, ValueAsSpan)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_from_span<true>(engine);
    test_bind::check_init_list<test_bind::from_span>(
        engine,
        "from_span",
        [](auto& v) -> auto&
        { return v.data; }
    );
}

#ifdef ASBIND20_HAS_CONTAINERS_RANGES

TEST(InitListNative, ValueFromRange)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_my_vec_ints<asbind20::policies::as_from_range, false>(
        engine
    );
    test_bind::check_init_list<test_bind::my_vec_ints>(
        engine,
        "my_vec_ints",
        [](auto& v) -> auto&
        { return v.data; }
    );
}

TEST(InitListGeneric, ValueFromRange)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_my_vec_ints<asbind20::policies::as_from_range, true>(
        engine
    );
    test_bind::check_init_list<test_bind::my_vec_ints>(
        engine,
        "my_vec_ints",
        [](auto& v) -> auto&
        { return v.data; }
    );
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
void register_ref_test_apply(asbind20::engine_pointer engine)
{
    using namespace asbind20;
    ref_class<ref_test_apply, UseGeneric>(engine, "ref_test_apply")
        .addref(fp<&ref_test_apply::addref>)
        .release(fp<&ref_test_apply::release>)
        .template list_factory<int>("int,int", use_policy<policies::apply_to<2>>);
}

void check_ref_test_apply(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "ref_test_apply");

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
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);

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
        : ref_test_vector(static_cast<int*>(list.data()), list.size())
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
static void register_ref_test_vector_with(asbind20::engine_pointer engine)
{
    using namespace asbind20;
    ref_class<ref_test_vector, UseGeneric>(engine, "ref_test_vector")
        .addref(fp<&ref_test_apply::addref>)
        .release(fp<&ref_test_apply::release>)
        .template list_factory<int>("repeat int", use_policy<IListPolicy>);
}

static void check_ref_test_vector(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "ref_test_vector");

    m->AddScriptSection(
        "ref_test_vector",
        "ref_test_vector@ create0() { return {}; }\n"
        "ref_test_vector@ create1() { return {1013}; }\n"
        "ref_test_vector@ create2() { return {10, 13}; }"
    );
    ASSERT_GE(m->Build(), 0);

    auto create = [&](int idx) -> ref_test_vector*
    {
        std::string decl = asbind20::string_concat(
            "ref_test_vector@ create", std::to_string(idx), "()"
        );
        auto* f = m->GetFunctionByDecl(decl.c_str());

        if(!f)
        {
            EXPECT_THAT(f, ::testing::NotNull())
                << decl << ": not found";
            return nullptr;
        }

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<ref_test_vector*>(ctx, f);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);

        auto* val = result.value();
        EXPECT_EQ(val->use_count(), 1);
        val->addref();
        return val;
    };

    {
        auto* val = create(0);
        EXPECT_EQ(val->use_count(), 1);

        EXPECT_THAT(val->data, ::testing::IsEmpty());

        val->release();
    }

    {
        auto* val = create(1);
        ASSERT_EQ(val->use_count(), 1);

        EXPECT_THAT(val->data, ::testing::ElementsAre(1013));

        val->release();
    }

    {
        auto* val = create(2);
        ASSERT_EQ(val->use_count(), 1);

        EXPECT_THAT(val->data, ::testing::ElementsAre(10, 13));

        val->release();
    }
}
} // namespace test_bind

TEST(InitListNative, RefApplyTo)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_apply<false>(engine);
    test_bind::check_ref_test_apply(engine);
}

TEST(InitListGeneric, RefApplyTo)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_apply<true>(engine);
    test_bind::check_ref_test_apply(engine);
}

TEST(InitListNative, RefAsIterators)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_iterators, false>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(InitListGeneric, RefAsIterators)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_iterators, true>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(InitListNative, RefRepeatListProxy)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::repeat_list_proxy, false>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(InitListGeneric, RefRepeatListProxy)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::repeat_list_proxy, true>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(InitListNative, RefPointerAndSize)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::pointer_and_size, false>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(InitListGeneric, RefPointerAndSize)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::pointer_and_size, true>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(InitListNative, RefAsInitializerList)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_initializer_list, false>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(InitListGeneric, RefAsInitializerList)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_initializer_list, true>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(InitListNative, RefAsSpan)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_span, false>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(InitListGeneric, RefAsSpan)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_span, true>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

#ifdef ASBIND20_HAS_CONTAINERS_RANGES

TEST(InitListNative, RefFromRange)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_from_range, false>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

TEST(InitListGeneric, RefFromRange)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_initlist_test_env(engine);

    test_bind::register_ref_test_vector_with<asbind20::policies::as_from_range, true>(
        engine
    );
    test_bind::check_ref_test_vector(engine);
}

#endif
