#include <asbind20/ext/assert.hpp>
#include <shared_test_lib.hpp>

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
)";

namespace test_bind
{
struct ref_string
{
    static ref_string* create()
    {
        return new ref_string(std::string(), 1);
    }

    static ref_string* from_str(std::string_view sv)
    {
        return new ref_string(std::string(sv), 1);
    }

    std::string str;
    AS_NAMESPACE_QUALIFIER asUINT refcount;

private:
    ~ref_string() = default;

public:
    ref_string& operator=(const ref_string& rhs)
    {
        str = rhs.str;
        return *this;
    }

    char get_opIndex(asUINT idx) const
    {
        if(idx > str.size())
            return '\0';
        return str[idx];
    }

    void addref()
    {
        ++refcount;
    }

    void release()
    {
        --refcount;
        if(refcount == 0)
            delete this;
    }
};

class ref_string_factory : public asIStringFactory
{
public:
    const void* GetStringConstant(const char* data, asUINT length) override
    {
        return ref_string::from_str(std::string_view(data, length));
    }

    int ReleaseStringConstant(const void* str) override
    {
        if(!str)
            return asERROR;

        auto* ptr = (ref_string*)str;
        ptr->release();
        return asSUCCESS;
    }

    int GetRawStringData(const void* str, char* data, asUINT* length) const override
    {
        if(!str)
            return asERROR;
        auto ptr = (ref_string*)str;
        if(length)
            *length = (asUINT)ptr->str.size();
        if(data)
            ptr->str.copy(data, ptr->str.size());
        return asSUCCESS;
    }

    static ref_string_factory& get()
    {
        static ref_string_factory instance{};
        return instance;
    }
};

void register_ref_string(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
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
            { return asUINT(ref->str.size()); }
        )
        .as_string(&ref_string_factory::get());
}

void register_ref_string(asbind20::use_generic_t, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
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
            { return asUINT(ref->str.size()); }
        )
        .as_string(&ref_string_factory::get());
}

void setup_bind_ref_string_env(
    asIScriptEngine* engine,
    bool generic
)
{
    asbind20::global(engine)
        .message_callback(
            +[](const asSMessageInfo* msg, void*)
            {
                std::cerr << msg->message << std::endl;
            }
        );

    if(generic)
        register_ref_string(asbind20::use_generic, engine);
    else
        register_ref_string(engine);

    asbind20::ext::register_script_assert(
        engine,
        [](std::string_view msg)
        {
            FAIL() << "vec2 assertion failed: " << msg;
        }
    );
}

static void run_script(asIScriptEngine* engine)
{
    using asbind_test::result_has_value;

    auto m = engine->GetModule("vec2_test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection("ref_string_test_script.as", ref_string_test_script);
    ASSERT_GE(m->Build(), 0);

    for(int idx : {0, 1})
    {
        auto f = m->GetFunctionByDecl(
            asbind20::string_concat("void test", std::to_string(idx), "()").c_str()
        );
        ASSERT_NE(f, nullptr)
            << "func index: " << idx;

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);

        EXPECT_TRUE(result_has_value(result))
            << "func index: " << idx;
    }
}
} // namespace test_bind

TEST(bind_ref_string, native)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_bind_ref_string_env(engine, false);

    test_bind::run_script(engine);
}

TEST(bind_ref_string, generic)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_bind_ref_string_env(engine, true);

    test_bind::run_script(engine);
}
