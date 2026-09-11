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
    [[= asbind20::rename("new_name")]]
    ugly_name
{
    [[maybe_unused]]
    int placeholder;
};

int func(
    [[= asbind20::default_arg("42")]] int arg
)
{
    (void)arg;
    std::terminate();
};
} // namespace

TEST(Annotation, Rename)
{
    using namespace asbind20;
    static_assert(
        meta::detail::calc_type_name<^^ugly_name>() ==
        "new_name"
    );
}

TEST(Annotation, DefaultArg)
{
    GTEST_SKIP() << "Awaiting GCC 16.1";

    // constexpr std::string_view sv= std::meta::extract<asbind20::default_arg>(std::meta::annotations_of_with_type(
    //     std::meta::parameters_of(^^func)[0],
    //     ^^asbind20::default_arg
    // )[0]).get();
    // EXPECT_EQ(sv, "42");
}

#endif
