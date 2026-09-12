#include <asbind_test/framework.hpp>
#include <asbind20/meta/reflection.hpp>

#ifdef ASBIND20_HAS_LIB_REFLECTION

#    if defined(__GNUC__) && !defined(__clang__)
// False positive if functions are only used in reflection
#        pragma GCC diagnostic ignored "-Wunused-function"
#    endif

namespace
{
struct
    [[= asbind20::rename("narcissus")]]
    daffodil
{
    [[maybe_unused]]
    [[= asbind20::rename("cross")]] int x;
};

[[= asbind20::rename("decorated")]]
int func(
    const daffodil&,
    [[= asbind20::default_arg("42")]]
    [[= asbind20::rename("val")]]
    int arg
)
{
    (void)arg;
    std::terminate();
}

enum renamed_enum
{
    start[[= asbind20::rename("zero")]] = 0
};
} // namespace

TEST(Annotation, Rename)
{
    using namespace asbind20;
    static_assert(
        meta::detail::calc_type_name<^^daffodil>() ==
        "narcissus"
    );

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    value_class<daffodil, true>(
        engine, "narcissus", AS_NAMESPACE_QUALIFIER asOBJ_POD
    )
        .behaviours_by_traits()
        .property(reflect<^^daffodil::x>());

    enum_<renamed_enum>(engine, "renamed_enum")
        .value(reflect<^^renamed_enum::start>());

    auto* m = create_module(engine, "rename_test");
    m->AddScriptSection(
        "rename_test",
        "int get_cross_val(const narcissus&in v)\n"
        "{\n"
        "    return v.cross + int(renamed_enum::zero);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* get_cross_val = m->GetFunctionByName("get_cross_val");
    ASSERT_THAT(get_cross_val, ::testing::NotNull());
    request_context ctx(engine);
    daffodil d{7};
    auto result = script_invoke<int>(ctx, get_cross_val, std::cref(d));
    ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 7);
}

TEST(Annotation, DefaultArg)
{
    using namespace asbind20;

    constexpr std::string_view sv = std::meta::extract<asbind20::default_arg>(
                                        std::meta::annotations_of_with_type(
                                            std::meta::parameters_of(^^func)[1],
                                            ^^asbind20::default_arg
                                        )[0]
    )
                                        .get();
    EXPECT_EQ(sv, "42");

    EXPECT_EQ(
        (asbind20::meta::refl_function_sig<^^func, false, false, true>()),
        "int decorated(const narcissus&in,int val=42)"
    );
}

#endif
