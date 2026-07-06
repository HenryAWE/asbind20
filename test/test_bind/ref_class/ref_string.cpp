#include <asbind_test/framework.hpp>

constexpr char ref_string_test_script[] = R"(void test0()
{
    string@ str = "hello";
    assert(str.size == 5);
}

void test1()
{
    string str = "hello";
    assert(str.size == 5);
}

string@ get_str()
{
    return "test";
}
)";

namespace test_bind
{
class ref_string
{
public:
    static ref_string* create()
    {
        return new ref_string(std::string());
    }

    static ref_string* from_str(std::string_view sv)
    {
        return new ref_string(std::string(sv));
    }

    ref_string(std::string s)
        : str(std::move(s)) {}

    std::string str;

private:
    ~ref_string() = default;

    AS_NAMESPACE_QUALIFIER asUINT m_refcount = 1;

public:
    ref_string& operator=(const ref_string& rhs)
    {
        str = rhs.str;
        return *this;
    }

    char get_opIndex(AS_NAMESPACE_QUALIFIER asUINT idx) const
    {
        if(idx > str.size())
            return '\0';
        return str[idx];
    }

    void addref()
    {
        ++m_refcount;
    }

    void release()
    {
        assert(m_refcount >= 1);
        --m_refcount;
        if(m_refcount == 0)
            delete this;
    }
};

class ref_string_factory : public AS_NAMESPACE_QUALIFIER asIStringFactory
{
public:
    const void* GetStringConstant(const char* data, AS_NAMESPACE_QUALIFIER asUINT length) override
    {
        return ref_string::from_str(std::string_view(data, length));
    }

    int ReleaseStringConstant(const void* str) override
    {
        if(!str)
            return AS_NAMESPACE_QUALIFIER asERROR;

        auto* ptr = (ref_string*)str;
        ptr->release();
        return AS_NAMESPACE_QUALIFIER asSUCCESS;
    }

    int GetRawStringData(const void* str, char* data, AS_NAMESPACE_QUALIFIER asUINT* length) const override
    {
        if(!str)
            return AS_NAMESPACE_QUALIFIER asERROR;
        auto ptr = (ref_string*)str;
        if(length)
            *length = static_cast<AS_NAMESPACE_QUALIFIER asUINT>(ptr->str.size());
        if(data)
            ptr->str.copy(data, ptr->str.size());
        return AS_NAMESPACE_QUALIFIER asSUCCESS;
    }

    static ref_string_factory& get()
    {
        static ref_string_factory instance{};
        return instance;
    }
};

void register_ref_string(asbind20::engine_pointer engine)
{
    using namespace asbind20;
    using test_bind::ref_string;

    ref_class<ref_string>(engine, "string")
        .factory_function("", &ref_string::create)
        .addref(&ref_string::addref)
        .release(&ref_string::release)
        .opAssign()
        .method(
            "uint get_size() const property",
            [](ref_string* ref)
            { return AS_NAMESPACE_QUALIFIER asUINT(ref->str.size()); }
        )
        .as_string(&ref_string_factory::get());
}

static void register_ref_string(asbind20::use_generic_t, asbind20::engine_pointer engine)
{
    using namespace asbind20;
    using test_bind::ref_string;

    ref_class<ref_string, true>(engine, "string")
        .factory_function("", fp<&ref_string::create>)
        .addref(fp<&ref_string::addref>)
        .release(fp<&ref_string::release>)
        .opAssign()
        .method(
            "uint get_size() const property",
            [](ref_string* ref)
            { return static_cast<AS_NAMESPACE_QUALIFIER asUINT>(ref->str.size()); }
        )
        .as_string(&ref_string_factory::get());
}

static void setup_bind_ref_string_env(
    asbind20::engine_pointer engine,
    bool generic
)
{
    asbind_test::setup_message_callback(engine);

    if(generic)
        register_ref_string(asbind20::use_generic, engine);
    else
        register_ref_string(engine);

    asbind_test::setup_script_assertion(engine);
}

static asbind20::module_pointer build_module(asbind20::engine_pointer engine)
{
    auto m = asbind20::create_module(engine, "vec2_test");

    m->AddScriptSection("ref_string_test_script.as", ref_string_test_script);
    int r = m->Build();
    if(r < 0)
    {
        ADD_FAILURE() << "failed to build module, r = " << r;
        return nullptr;
    }

    return m;
}

static void run_script(asbind20::engine_pointer engine)
{
    auto* m = build_module(engine);
    if(!m)
        return;

    for(int idx : {0, 1})
    {
        SCOPED_TRACE("func index = " + std::to_string(idx));

        auto f = m->GetFunctionByDecl(
            asbind20::string_concat("void test", std::to_string(idx), "()").c_str()
        );
        ASSERT_NE(f, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);

        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
    }
}
} // namespace test_bind

TEST(BindRefString, Native)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    test_bind::setup_bind_ref_string_env(engine, false);

    test_bind::run_script(engine);
}

TEST(BindRefString, Generic)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_bind_ref_string_env(engine, true);

    test_bind::run_script(engine);
}

TEST(BindRefString, Extract)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_bind_ref_string_env(
        engine, asbind20::has_max_portability()
    );

    auto* m = test_bind::build_module(engine);
    if(!m)
        return;

    auto* f = m->GetFunctionByDecl("string@ get_str()");
    ASSERT_NE(f, nullptr);

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<test_bind::ref_string*>(ctx, f);
    ASBIND_TEST_EXPECT_INVOKE_RESULT(result);

    namespace debugging = asbind20::debugging;

    EXPECT_NE(result.value(), nullptr);
    auto extracted = debugging::extract_string(
        test_bind::ref_string_factory::get(), *result
    );
    ASSERT_TRUE(extracted.has_value());
    EXPECT_EQ(extracted.value(), "test");
}
