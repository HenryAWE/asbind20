#include <sstream>
#include <stdexcept>
#include <shared_test_lib.hpp>
#include <asbind20/ext/stdstring.hpp>
#include <asbind20/ext/assert.hpp>

constexpr char vec2_test_script[] = R"(void test0()
{
    vec2 v1;
    assert(v1.x == 0);
    assert(v1.y == 0);
    assert(v1 == vec2(0, 0));
    vec2 v2 = v1 + vec2(1, 2);
    assert(v2 == vec2(1, 2));
}

void test1()
{
    vec2 v1(1, 0);
    vec2 v2(0, 1);
    assert(v1 * v2 == 0);
}
)";

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

void register_vec2(asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<vec2>(
        engine, "vec2", asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    )
        .behaviours_by_traits()
        .constructor<float, float>("float,float")
        .opEquals()
        .opAdd()
        .opNeg()
        .method(
            "float opMul(const vec2&in) const",
            [](const vec2& lhs, const vec2& rhs) -> float
            { return lhs * rhs; }
        )
        .method("float& opIndex(uint)", &vec2_opIndex)
        .method("const float& opIndex(uint) const", &vec2_opIndex)
        .opConv<std::string>("string")
        .property("float x", 0)
        .property("float y", sizeof(float));
}

void register_vec2(asbind20::use_generic_t, asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<vec2, true>(
        engine, "vec2", asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    )
        .behaviours_by_traits()
        .constructor<float, float>("float,float")
        .opEquals()
        .opAdd()
        .opNeg()
        .method(
            "float opMul(const vec2&in) const",
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
    asbind20::global(engine)
        .message_callback(
            +[](const asSMessageInfo* msg, void*)
            {
                std::cerr << msg->message << std::endl;
            }
        );
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

static void run_script(asIScriptEngine* engine)
{
    using asbind_test::result_has_value;

    auto m = engine->GetModule("vec2_test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection("vec2_test_script.as", vec2_test_script);
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

TEST(bind_vec2, native)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "max portability";

    auto engine = asbind20::make_script_engine();
    test_bind::setup_bind_vec2_env(engine, false);

    test_bind::run_script(engine);
}

TEST(bind_vec2, generic)
{
    auto engine = asbind20::make_script_engine();
    test_bind::setup_bind_vec2_env(engine, true);

    test_bind::run_script(engine);
}
