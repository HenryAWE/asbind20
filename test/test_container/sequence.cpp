#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/assert.hpp>
#include "test_container.hpp"

#ifndef ASBIND20_TEST_SKIP_SEQUENCE_TEST
#    include <asbind20/ext/stdstring.hpp>
#    include <asbind20/container/sequence.hpp>

namespace test_container
{
class refcounting_base
{
public:
    refcounting_base() = default;

    void addref()
    {
        m_gc_flag = false;
        ++m_counter;
    }

    void release()
    {
        m_gc_flag = false;
        m_counter.dec_and_try_destroy(
            [](auto* p)
            { delete p; },
            this
        );
    }

    bool get_gc_flag() const noexcept
    {
        return m_gc_flag;
    }

    void set_gc_flag() noexcept
    {
        m_gc_flag = true;
    }

    int get_refcount() const
    {
        return m_counter;
    }

protected:
    virtual ~refcounting_base() = default;

private:
    asbind20::atomic_counter m_counter;
    bool m_gc_flag = false;
};

static bool template_callback(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, bool& no_gc)
{
    int subtype_id = ti->GetSubTypeId();
    if(asbind20::is_void_type(subtype_id))
        return false;

    no_gc = !asbind20::type_requires_gc(ti->GetSubType());

    return true;
}

template <template <typename...> typename Sequence, typename Allocator = asbind20::as_allocator<void>>
class seq_wrapper : public refcounting_base
{
public:
    seq_wrapper(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        : c(ti->GetEngine(), ti->GetSubTypeId())
    {}

    seq_wrapper(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, asbind20::script_init_list_repeat ilist)
        : c(ti->GetEngine(), ti->GetSubTypeId(), ilist)
    {}

    using size_type = AS_NAMESPACE_QUALIFIER asUINT;

    size_type size() const noexcept
    {
        return static_cast<size_type>(c.size());
    }

    void clear() noexcept
    {
        c.clear();
    }

    bool empty() const noexcept
    {
        return c.empty();
    }

    void push_front(const void* ref)
    {
        c.push_front(ref);
    }

    void push_back(const void* ref)
    {
        c.push_back(ref);
    }

    void emplace_front()
    {
        c.emplace_front();
    }

    void emplace_back()
    {
        c.emplace_back();
    }

    void pop_front()
    {
        c.pop_front();
    }

    void pop_back()
    {
        c.pop_back();
    }

    void* opIndex(size_type idx)
    {
        void* ref = c.address_at(idx);
        if(!ref)
            throw std::out_of_range("out of range");
        return ref;
    }

    void enum_refs(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
    {
        (void)engine;
        assert(engine == c.get_engine());
        c.enum_refs();
    }

    void release_refs(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
    {
        (void)engine;
        assert(engine == c.get_engine());
        c.clear();
    }

    using container_type = asbind20::container::sequence<Sequence, Allocator>;

    container_type c;
};

template <template <typename...> typename Sequence, bool UseGeneric>
void register_seq_wrapper(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    using seq_t = seq_wrapper<Sequence>;
    asbind20::template_ref_class<seq_t, UseGeneric>(engine, "sequence<T>", AS_NAMESPACE_QUALIFIER asOBJ_GC)
        .template_callback(fp<&template_callback>)
        .addref(fp<&seq_t::addref>)
        .release(fp<&seq_t::release>)
        .get_refcount(fp<&seq_t::get_refcount>)
        .get_gc_flag(fp<&seq_t::get_gc_flag>)
        .set_gc_flag(fp<&seq_t::set_gc_flag>)
        .enum_refs(fp<&seq_t::enum_refs>)
        .release_refs(fp<&seq_t::release_refs>)
        .default_factory(use_policy<policies::notify_gc>)
        .list_factory("repeat T", use_policy<policies::repeat_list_proxy, policies::notify_gc>)
        .method("uint get_size() const property", fp<&seq_t::size>)
        .method("bool empty() const", fp<&seq_t::empty>)
        .method("void clear()", fp<&seq_t::clear>)
        .method("void push_front(const T&in)", fp<&seq_t::push_front>)
        .method("void push_back(const T&in)", fp<&seq_t::push_back>)
        .method("void pop_front()", fp<&seq_t::pop_front>)
        .method("void pop_back()", fp<&seq_t::pop_back>)
        .method("void emplace_front()", fp<&seq_t::emplace_front>)
        .method("void emplace_back()", fp<&seq_t::emplace_back>)
        .method("T& opIndex(uint)", fp<&seq_t::opIndex>)
        .method("const T& opIndex(uint) const", fp<&seq_t::opIndex>);
}

static constexpr char test_script[] = R"AngelScript(bool test0()
{
    sequence<int> v;
    v.push_back(42);
    v.push_front(0);
    v.push_back(0);
    v.push_back(42);
    v.pop_back();
    return v[0] == 0 && v[1] == 42 && v.size == 3;
}

bool test1()
{
    sequence<string> v;
    v.push_back("to be removed");
    v.push_back("hello");
    v.pop_front();
    v.push_back("AngelScript");
    return v.size == 2 && v[0].size == 5;
}

class foo{};

bool test2()
{
    sequence<foo@> v;
    v.push_back(foo());
    v.push_back(null);
    return v[1] is null;
}

bool test3()
{
    sequence<foo@> v;
    v.push_back(foo());
    v.push_back(foo());
    v.pop_front();
    return v.size == 1 && v[0] !is null;
}

bool test4()
{
    sequence<int> v = {0, 1, 2, 3};
    assert(v[0] == 0);
    assert(v[1] == 1);
    assert(v[2] == 2);
    assert(v[3] == 3);
    return v.size == 4;
}

bool test5()
{
    sequence<string> v = {"hello", "world"};
    assert(v[0] == "hello");
    assert(v[1] == "world");
    v.pop_front();
    v.push_back("is");
    v.push_back("beautiful");
    assert(v[0] == "world");
    assert(v[1] == "is");
    assert(v[2] == "beautiful");

    v.emplace_back();
    assert(v.size == 4);
    assert(v[3] == "");
    v.pop_back();

    return v.size == 3;
}

class bar
{
    sequence<bar@> refs;
};

bool test6()
{
    bar@ b = bar();
    b.refs.push_back(@b);
    return b.refs.size == 1 && b.refs[0] !is null;
}

bool test7()
{
    sequence<bar@> v = {null, null, bar()};
    assert(v.size == 3);
    assert(v[v.size - 1] !is null);
    v[2].refs.push_back(@v[2]);
    v.clear();
    return v.empty();
}

class foobar
{
    sequence<foobar@>@ refs;
};

bool test8()
{
    foobar@ fb = foobar();
    {
        sequence<foobar@> seq = {@fb};
        @fb.refs = seq;
    }
    assert(fb.refs[0] is fb);
    fb.refs.push_back(@fb);
    assert(fb.refs[1] is fb.refs[0]);

    sequence<foobar@>@ refs = @fb.refs;
    assert(refs !is null);
    assert(refs.size == 2);
    @fb = null;
    return refs !is null;
}
)AngelScript";

void check_sequence_wrapper(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule("test_vector", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_sequence",
        test_script
    );
    ASSERT_GE(m->Build(), 0);

    auto test = [&](int idx)
    {
        std::string decl = asbind20::string_concat("bool test", std::to_string(idx), "()");

        SCOPED_TRACE(decl);

        auto* f = m->GetFunctionByDecl(decl.c_str());
        ASSERT_NE(f, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<bool>(ctx, f);

        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_TRUE(result.value());
    };

    for(int i = 0; i <= 8; ++i)
    {
        test(i);
    }
}

template <template <typename...> typename Container>
void check_seq_iterator_int(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* ti = engine->GetTypeInfoByDecl("sequence<int>");
    ASSERT_NE(ti, nullptr);
    auto* seq = (seq_wrapper<Container>*)engine->CreateScriptObject(ti);
    ASSERT_NE(seq, nullptr);
    EXPECT_EQ(seq->c.element_type_id(), AS_NAMESPACE_QUALIFIER asTYPEID_INT32);

    auto push_back_int = [seq](int val)
    {
        seq->c.push_back(&val);
    };

    push_back_int(10);
    push_back_int(13);

    {
        using iter_t = typename seq_wrapper<Container>::container_type::const_iterator;
        static_assert(
            std::convertible_to<typename std::iterator_traits<iter_t>::iterator_category, std::bidirectional_iterator_tag>
        );
    }

    {
        ASSERT_EQ(seq->size(), 2);
        int buf[2]{};
        std::transform(
            std::as_const(seq->c).begin(),
            std::as_const(seq->c).end(),
            std::begin(buf),
            [](const void* ref) -> int
            {
                return *static_cast<const int*>(ref);
            }
        );

        EXPECT_EQ(buf[0], 10);
        EXPECT_EQ(buf[1], 13);
    }

    {
        auto it = seq->c.erase(seq->c.begin());
        ASSERT_EQ(seq->size(), 1);
        EXPECT_EQ(it, seq->c.begin());
        EXPECT_EQ(*static_cast<const int*>(*it), 13);
    }

    {
        int arg = 10;
        auto it = seq->c.insert(seq->c.begin(), &arg);
        EXPECT_EQ(it, seq->c.begin());

        ASSERT_EQ(seq->size(), 2);
        int buf[2]{};
        std::transform(
            std::as_const(seq->c).begin(),
            std::as_const(seq->c).end(),
            std::begin(buf),
            [](const void* ref) -> int
            {
                return *static_cast<const int*>(ref);
            }
        );

        EXPECT_EQ(buf[0], 10);
        EXPECT_EQ(buf[1], 13);
    }

    {
        seq->c.clear();
        ASSERT_TRUE(seq->empty());

        push_back_int(10);

        int arg = 13;
        auto it = seq->c.insert(seq->c.end(), &arg);
        EXPECT_EQ(it, std::next(seq->c.begin()));
        EXPECT_EQ(it, std::prev(seq->c.end()));

        ASSERT_EQ(seq->size(), 2);
        int buf[2]{};
        std::transform(
            std::as_const(seq->c).begin(),
            std::as_const(seq->c).end(),
            std::begin(buf),
            [](const void* ref) -> int
            {
                return *static_cast<const int*>(ref);
            }
        );

        EXPECT_EQ(buf[0], 10);
        EXPECT_EQ(buf[1], 13);
    }

    engine->ReleaseScriptObject(seq, ti);
}

template <template <typename...> typename Container>
void check_seq_iterator_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    constexpr char group_name[] = "seq_iterator_test_cfg";

    int counter = 0;

    engine->BeginConfigGroup(group_name);
    asbind20::global(engine)
        .function(
            "int seq_iterator_test_helper()",
            [](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
            {
                int result = asbind20::get_generic_auxiliary<int>(gen)++;
                asbind20::set_generic_return<int>(gen, std::move(result));
            },
            asbind20::auxiliary(counter)
        );
    engine->EndConfigGroup();

    auto* m = engine->GetModule(
        "seq_iterator_test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "seq_iterator_test",
        "class elem\n"
        "{\n"
        "    int data;\n"
        "    elem() { data = seq_iterator_test_helper(); }\n"
        "    int elem_val() const { return data; }\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);
    EXPECT_EQ(counter, 0);

    auto* ti = m->GetTypeInfoByDecl("sequence<elem>");
    ASSERT_NE(ti, nullptr);
    auto* seq = (seq_wrapper<Container>*)engine->CreateScriptObject(ti);
    ASSERT_NE(seq, nullptr);

    SCOPED_TRACE("elem id: " + std::to_string(seq->c.element_type_id()));

    for(int i = 0; i < 10; ++i)
        seq->emplace_back();
    EXPECT_EQ(counter, 10);

    {
        AS_NAMESPACE_QUALIFIER asITypeInfo* elem_ti = m->GetTypeInfoByDecl("elem");
        ASSERT_NE(elem_ti, nullptr);
        EXPECT_EQ(elem_ti->GetTypeId(), ti->GetSubTypeId());

        auto* elem_val = elem_ti->GetMethodByDecl("int elem_val() const");
        ASSERT_NE(elem_val, nullptr);

        {
            asbind20::request_context ctx(engine);
            auto it = seq->c.begin();
            for(int i = 0; i < 10; ++i)
            {
                auto result = asbind20::script_invoke<int>(ctx, *it, elem_val);
                EXPECT_TRUE(asbind_test::result_has_value(result));
                EXPECT_EQ(result.value(), i);

                ++it;
            }
        }

        {
            auto* elem = (AS_NAMESPACE_QUALIFIER asIScriptObject*)engine->CreateScriptObject(elem_ti);
            EXPECT_EQ(counter, 11);

            const char* prop_name;
            int off;
            int prop_type_id;
            elem_ti->GetProperty(
                0, &prop_name, &prop_type_id, nullptr, nullptr, &off
            );
            EXPECT_STREQ(prop_name, "data");
            ASSERT_EQ(prop_type_id, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
            ASSERT_EQ(prop_type_id, elem->GetPropertyTypeId(0));

            int* elem_data = (int*)elem->GetAddressOfProperty(0);
            ASSERT_NE(elem_data, nullptr);
            EXPECT_EQ(*elem_data, 10);
            *elem_data = -1;

            seq->c.insert(seq->c.begin(), elem);
            elem->Release();

            EXPECT_EQ(seq->size(), 11);
            // The copy-constructor of script should not increase the counter
            EXPECT_EQ(counter, 11);
        }

        // Check the previously inserted value
        {
            asbind20::request_context ctx(engine);
            auto it = seq->c.begin();
            auto result = asbind20::script_invoke<int>(ctx, *it, elem_val);
            EXPECT_TRUE(asbind_test::result_has_value(result));
            EXPECT_EQ(result.value(), -1);
        }
    }

    engine->ReleaseScriptObject(seq, ti);

    engine->RemoveConfigGroup(group_name);
}

void setup_seq_test_env(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, bool use_generic)
{
    using namespace asbind20;
    engine->SetEngineProperty(
        AS_NAMESPACE_QUALIFIER asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE, true
    );
    asbind_test::setup_message_callback(engine, true);
    ext::register_std_string(engine, true, use_generic);
    ext::register_script_assert(
        engine,
        [](std::string_view msg)
        {
            FAIL() << "seq assertion failed: " << msg << std::endl;
        }
    );
}
} // namespace test_container

TEST(sequence, vector_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = make_script_engine();
    test_container::setup_seq_test_env(engine, false);

    test_container::register_seq_wrapper<std::vector, false>(engine);
    test_container::check_sequence_wrapper(engine);

    test_container::check_seq_iterator_int<std::vector>(engine);
    test_container::check_seq_iterator_class<std::vector>(engine);
}

TEST(sequence, vector_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    test_container::setup_seq_test_env(engine, true);

    static_assert(std::same_as<test_container::seq_wrapper<std::vector>::container_type, container::vector<>>);

    test_container::register_seq_wrapper<std::vector, true>(engine);
    test_container::check_sequence_wrapper(engine);

    test_container::check_seq_iterator_int<std::vector>(engine);
    test_container::check_seq_iterator_class<std::vector>(engine);
}

TEST(sequence, deque_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = make_script_engine();
    test_container::setup_seq_test_env(engine, false);

    test_container::register_seq_wrapper<std::deque, false>(engine);
    test_container::check_sequence_wrapper(engine);

    test_container::check_seq_iterator_int<std::deque>(engine);
    test_container::check_seq_iterator_class<std::deque>(engine);
}

TEST(sequence, deque_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    test_container::setup_seq_test_env(engine, true);

    static_assert(std::same_as<test_container::seq_wrapper<std::deque>::container_type, container::deque<>>);

    test_container::register_seq_wrapper<std::deque, true>(engine);
    test_container::check_sequence_wrapper(engine);

    test_container::check_seq_iterator_int<std::deque>(engine);
    test_container::check_seq_iterator_class<std::deque>(engine);
}

#endif
