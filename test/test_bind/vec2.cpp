#include <sstream>
#include <stdexcept>
#include <shared_test_lib.hpp>
#include <asbind20/ext/stdstring.hpp>
#include <asbind20/ext/assert.hpp>

constexpr char vec2_test_script[] = R"AngelScript(
void test0()
{
    vec2<float> v1;
    assert(v1.x == 0);
    assert(v1.y == 0);
    assert(v1 == vec2<float>(0, 0));
    vec2<float> v2 = v1 + vec2<float>(1, 2);
    assert(v2 == vec2<float>(1, 2));
}

void test1()
{
    vec2<float> v1(1, 0);
    vec2<float> v2(0, 1);
    assert(v1 * v2 == 0);
}

void test2()
{
    vec2<float> v1 = {1, 0};
    assert(v1 == vec2<float>(1, 0));
    assert(string(v1) == "(1, 0)");
}
)AngelScript";

namespace test_bind
{
class vec2
{
public:
    vec2() noexcept
        : elements{0, 0} {}

    vec2(float x, float y) noexcept
        : elements{x, y} {}

    float& operator[](std::size_t i)
    {
        return elements[i];
    }

    const float& operator[](std::size_t i) const
    {
        return elements[i];
    }

    friend vec2 operator+(const vec2& lhs, const vec2& rhs)
    {
        return vec2{lhs[0] + rhs[0], lhs[1] + rhs[1]};
    }

    friend vec2 operator-(const vec2& lhs, const vec2& rhs)
    {
        return vec2{lhs[0] - rhs[0], lhs[1] - rhs[1]};
    }

    friend float operator*(const vec2& lhs, const vec2& rhs)
    {
        return lhs[0] * rhs[0] + lhs[1] * rhs[1];
    }

    vec2 operator-() const
    {
        return vec2{-elements[0], -elements[1]};
    }

    constexpr bool operator==(const vec2&) const = default;

    float elements[2];

    explicit operator std::string() const
    {
        std::stringstream ss;
        ss << '(' << elements[0] << ", " << elements[1] << ')';
        return std::move(ss).str();
    }
};

static float& vec2_opIndex(vec2& v, asUINT i)
{
    if(i >= 2)
        throw std::out_of_range("out of range");
    return v[i];
}

struct vec2_holder
{
    vec2_holder(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
    {
        (void)ti;
        []()
        {
            // FAIL() cannot be directly called from a constructor
            FAIL() << "unreachable";
        }();
    }

    ~vec2_holder() = default;

    static bool template_callback(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, bool&)
    {
        int subtype_id = ti->GetSubTypeId();
        return subtype_id == AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT;
    }
};

static constexpr AS_NAMESPACE_QUALIFIER asQWORD vec2_type_flags =
    AS_NAMESPACE_QUALIFIER asOBJ_POD |
    AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLFLOATS |
    AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS;

void register_vec2(asIScriptEngine* engine)
{
    using namespace asbind20;

    template_value_class<vec2_holder>(
        engine, "vec2<T>", AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_CD
    )
        .default_constructor()
        .destructor()
        .template_callback(fp<&vec2_holder::template_callback>);

    value_class<vec2>(
        engine, "vec2<float>", vec2_type_flags
    )
        .behaviours_by_traits(vec2_type_flags | AS_NAMESPACE_QUALIFIER asGetTypeTraits<vec2>())
        .constructor<float, float>("float,float")
        .list_constructor<float, policies::apply_to<2>>("float,float")
        .opEquals()
        .opAdd()
        .opNeg()
        .method(
            "float opMul(const vec2<float>&in) const",
            [](const vec2& lhs, const vec2& rhs) -> float
            { return lhs * rhs; }
        )
        .method("float& opIndex(uint)", &vec2_opIndex)
        .method("const float& opIndex(uint) const", &vec2_opIndex)
        .opConv<std::string>("string")
        .property("float x", 0)
        .property("float y", sizeof(float));
}

void register_vec2(asbind20::use_generic_t, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    template_value_class<vec2_holder, true>(engine, "vec2<T>", AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_CD)
        .default_constructor()
        .destructor()
        .template_callback(fp<&vec2_holder::template_callback>);

    value_class<vec2, true>(
        engine, "vec2<float>", vec2_type_flags
    )
        .behaviours_by_traits(vec2_type_flags | AS_NAMESPACE_QUALIFIER asGetTypeTraits<vec2>())
        .constructor<float, float>("float,float")
        .list_constructor<float, policies::apply_to<2>>("float,float")
        .opEquals()
        .opAdd()
        .opNeg()
        .method(
            "float opMul(const vec2<float>&in) const",
            [](const vec2& lhs, const vec2& rhs) -> float
            { return lhs * rhs; }
        )
        .method("float& opIndex(uint)", fp<&vec2_opIndex>)
        .method("const float& opIndex(uint) const", fp<&vec2_opIndex>)
        .opConv<std::string>("string")
        .property("float x", 0)
        .property("float y", sizeof(float));
}

void setup_bind_vec2_env(
    asIScriptEngine* engine,
    bool generic
)
{
    asbind_test::setup_message_callback(engine);
    asbind20::ext::register_std_string(engine, true, generic);
    asbind20::ext::register_script_assert(
        engine,
        [](std::string_view msg)
        {
            FAIL() << "vec2 assertion failed: " << msg;
        }
    );

    if(generic)
        register_vec2(asbind20::use_generic, engine);
    else
        register_vec2(engine);
}

static void run_vec2_test_script(asIScriptEngine* engine)
{
    using asbind_test::result_has_value;

    auto m = engine->GetModule("vec2_test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection("vec2_test_script.as", vec2_test_script);
    ASSERT_GE(m->Build(), 0);

    for(int idx : {0, 1, 2})
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

TEST(bind_vec2, native)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_bind_vec2_env(engine, false);

    test_bind::run_vec2_test_script(engine);
}

TEST(bind_vec2, generic)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_bind_vec2_env(engine, true);

    test_bind::run_vec2_test_script(engine);
}
