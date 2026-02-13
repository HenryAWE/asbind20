#include <asbind20/asbind.hpp>
#include <asbind20/meta/validator.hpp>

static consteval bool test_utility_concepts()
{
    {
        struct callable_struct
        {
            void operator()() const {}
        };

        using asbind20::noncapturing_native_lambda;

        static_assert(!noncapturing_native_lambda<callable_struct>);

        int tmp = 0;
        auto lambda1 = [&tmp](int)
        {
            return tmp;
        };

        static_assert(!noncapturing_native_lambda<decltype(lambda1)>);

        auto lambda2 = [](int)
        {
            return 42;
        };

        static_assert(noncapturing_native_lambda<decltype(lambda2)>);
        static_assert((+lambda2)(0) == 42);
    }

    return true;
}

static consteval bool test_utility_meta_helpers()
{
    using namespace asbind20;

    static_assert(!meta::contains<void, std::tuple<>>::value);
    static_assert(!meta::contains<void, std::tuple<int, float>>::value);
    static_assert(meta::contains<void, std::tuple<int, float, void>>::value);

    return true;
}

static_assert(test_utility_concepts());
static_assert(test_utility_meta_helpers());

#ifdef ASBIND20_HAS_STANDALONE_STDCALL

namespace test_utility
{
static int cdecl_func(int, float)
{
    return 0;
}

static int ASBIND20_STDCALL stdcall_func(int, float)
{
    return 0;
}
} // namespace test_utility

static consteval bool test_stdcall_helpers()
{
    using namespace asbind20;

    static_assert(!meta::is_stdcall_v<decltype(test_utility::cdecl_func)>);
    static_assert(meta::is_stdcall_v<decltype(test_utility::stdcall_func)>);

    return true;
}

static_assert(test_stdcall_helpers());

#endif

consteval bool test_meta_validator()
{
    {
        using asbind20::meta::validator::ptrref_of;

        static_assert(!ptrref_of<int, int>);
        static_assert(ptrref_of<int*, int>);
        static_assert(ptrref_of<int&, int>);
        static_assert(ptrref_of<const int&, int>);
    }

    return true;
}

static_assert(test_meta_validator());
