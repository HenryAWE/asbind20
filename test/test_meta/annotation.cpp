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

class
    [[= asbind20::rename("marionette")]]
    puppet
{
public:
    puppet() = default;

    void inc_ref()
    {
        m_counter.inc();
    }

    void dec_ref()
    {
        m_counter.dec_and_try_delete(this);
    }

private:
    asbind20::atomic_counter m_counter;
    [[maybe_unused]]
    int m_placeholder[4];
};

[[= asbind20::rename("decorated")]] int func(
    const daffodil&,
    [[= asbind20::default_arg("42")]][[= asbind20::rename("val")]] int arg
)
{
    (void)arg;
    std::terminate();
}

enum renamed_enum
{
    start[[= asbind20::rename("zero")]] = 0
};

void check_renamed_entities(asbind20::engine_pointer engine)
{
    using namespace asbind20;

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
} // namespace

TEST(Annotation, RenameGeneric)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    {
        value_class<daffodil, true> v(
            *engine, AS_NAMESPACE_QUALIFIER asOBJ_POD
        );
        v
            .behaviours_by_traits()
            .property(reflect<^^daffodil::x>());
        EXPECT_EQ(v.get_name(), "narcissus");
    }

    {
        enum_<renamed_enum> e(*engine);
        e
            .value(reflect<^^renamed_enum::start>());
        EXPECT_EQ(e.get_name(), "renamed_enum");
    }

    {
        ref_class<puppet, true> r(engine);
        r
            .addref(fp<&puppet::inc_ref>)
            .release(fp<&puppet::dec_ref>);
        EXPECT_EQ(r.get_name(), "marionette");
    }

    check_renamed_entities(engine);
}

TEST(Annotation, RenameNative)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    {
        value_class<daffodil, false> v(
            *engine, AS_NAMESPACE_QUALIFIER asOBJ_POD
        );
        v
            .behaviours_by_traits()
            .property(reflect<^^daffodil::x>());
        EXPECT_EQ(v.get_name(), "narcissus");
    }

    {
        enum_<renamed_enum> e(*engine);
        e
            .value(reflect<^^renamed_enum::start>());
        EXPECT_EQ(e.get_name(), "renamed_enum");
    }

    {
        ref_class<puppet, false> r(engine);
        r
            .addref(fp<&puppet::inc_ref>)
            .release(fp<&puppet::dec_ref>);
        EXPECT_EQ(r.get_name(), "marionette");
    }

    check_renamed_entities(engine);
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
