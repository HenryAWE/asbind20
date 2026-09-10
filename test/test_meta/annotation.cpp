#include <asbind_test/framework.hpp>
#include <asbind20/meta/reflection.hpp>

#ifdef ASBIND20_HAS_LIB_REFLECTION

namespace
{
struct
    [[= asbind20::rename("new_name")]]
    ugly_name
{
    [[maybe_unused]]
    int placeholder;
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

#endif
