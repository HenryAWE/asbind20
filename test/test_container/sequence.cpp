#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/stdstring.hpp>
#include <asbind20/container.hpp>

namespace test_container
{
class refcounting_base
{
public:
    refcounting_base() = default;

    void addref()
    {
        ++m_counter;
    }

    void release()
    {
        m_counter.dec_and_try_destroy(
            [](auto* p)
            { delete p; },
            this
        );
    }

protected:
    virtual ~refcounting_base() = default;

private:
    asbind20::atomic_counter m_counter;
};

template <template <typename...> typename Sequence>
class seq_wrapper : public refcounting_base
{
public:
    seq_wrapper(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        : m_vec(ti->GetEngine(), ti->GetSubTypeId())
    {}

    using size_type = AS_NAMESPACE_QUALIFIER asUINT;

    size_type size() const noexcept
    {
        return static_cast<size_type>(m_vec.size());
    }

    void push_front(const void* ref)
    {
        m_vec.push_front(ref);
    }

    void push_back(const void* ref)
    {
        m_vec.push_back(ref);
    }

    void* opIndex(size_type idx)
    {
        void* ref = m_vec.address_at(idx);
        if(!ref)
            throw std::out_of_range("out of range");
        return ref;
    }

private:
    asbind20::container::sequence<Sequence> m_vec;
};

template <template <typename...> typename Sequence, bool UseGeneric>
void register_seq_wrapper(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    using seq_t = seq_wrapper<Sequence>;
    asbind20::template_ref_class<seq_t, UseGeneric>(engine, "sequence<T>")
        .addref(fp<&seq_t::addref>)
        .release(fp<&seq_t::release>)
        .default_factory()
        .method("uint get_size() const property", fp<&seq_t::size>)
        .method("void push_front(const T&in)", fp<&seq_t::push_front>)
        .method("void push_back(const T&in)", fp<&seq_t::push_back>)
        .method("T& opIndex(uint)", fp<&seq_t::opIndex>)
        .method("const T& opIndex(uint) const", fp<&seq_t::opIndex>);
}

void check_sequence_wrapper(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule("test_vector", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_vector",
        "bool test0()\n"
        "{\n"
        "    sequence<int> v;\n"
        "    v.push_back(42);\n"
        "    v.push_front(0);\n"
        "    return v[0] == 0 && v[1] == 42;\n"
        "}\n"
        "bool test1()\n"
        "{\n"
        "    sequence<string> v;\n"
        "    v.push_back(\"hello\");\n"
        "    v.push_back(\"AngelScript\");\n"
        "    return v.size == 2 && v[0].size == 5;\n"
        "}\n"
        "class foo{};\n"
        "bool test2()\n"
        "{\n"
        "    sequence<foo@> v;\n"
        "    v.push_back(foo());\n"
        "    v.push_back(null);\n"
        "    return v[1] is null;\n"
        "}\n"
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

    test(0);
    test(1);
    test(2);
}
} // namespace test_container

TEST(sequence, vector_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    ext::register_std_string(engine, true, false);

    test_container::register_seq_wrapper<std::vector, false>(engine);
    test_container::check_sequence_wrapper(engine);
}

TEST(sequence, vector_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    ext::register_std_string(engine, true, true);

    test_container::register_seq_wrapper<std::vector, true>(engine);
    test_container::check_sequence_wrapper(engine);
}

TEST(sequence, deque_native)
{
    using namespace asbind20;

    if(has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    ext::register_std_string(engine, true, false);

    test_container::register_seq_wrapper<std::deque, false>(engine);
    test_container::check_sequence_wrapper(engine);
}

TEST(sequence, deque_generic)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    ext::register_std_string(engine, true, true);

    test_container::register_seq_wrapper<std::deque, true>(engine);
    test_container::check_sequence_wrapper(engine);
}
