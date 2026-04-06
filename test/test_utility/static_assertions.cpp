#include <asbind20/asbind.hpp>
#include <asbind20/bind/validator.hpp>

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

static_assert(test_utility_concepts());

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

consteval bool test_detail_validator()
{
    {
        using asbind20::detail::ptrref_of;

        static_assert(!ptrref_of<int, int>);
        static_assert(ptrref_of<int*, int>);
        static_assert(ptrref_of<int&, int>);
        static_assert(ptrref_of<const int&, int>);

        static_assert(!ptrref_of<bool&, int>);
    }

    using asbind20::detail::signature_matcher;
    using namespace asbind20::detail::validator;

    {
        using templ_cb_matcher = signature_matcher<
            by_value<bool>,
            by_addr<AS_NAMESPACE_QUALIFIER asITypeInfo>,
            by_addr<bool>>;

        using fn_0 = bool();
        static_assert(!templ_cb_matcher{}(std::in_place_type<fn_0>));

        using fn_1 = bool(AS_NAMESPACE_QUALIFIER asITypeInfo*, bool&);
        static_assert(templ_cb_matcher{}(std::in_place_type<fn_1>));

        using fn_2 = int(AS_NAMESPACE_QUALIFIER asITypeInfo*, bool&);
        static_assert(!templ_cb_matcher{}(std::in_place_type<fn_2>));

        using fn_3 = bool(AS_NAMESPACE_QUALIFIER asITypeInfo*, int&);
        static_assert(!templ_cb_matcher{}(std::in_place_type<fn_3>));
    }

    {
        using my_matcher = signature_matcher<void_>;

        using fn_0 = bool();
        static_assert(!my_matcher{}(std::in_place_type<fn_0>));

        using fn_1 = void();
        static_assert(my_matcher{}(std::in_place_type<fn_1>));

        using fn_2 = void(int);
        static_assert(!my_matcher{}(std::in_place_type<fn_2>));
    }

    {
        using my_matcher = signature_matcher<void_, by_addr<AS_NAMESPACE_QUALIFIER asIScriptEngine>>;

        using fn_0 = void(AS_NAMESPACE_QUALIFIER asIScriptEngine*);
        static_assert(my_matcher{}(std::in_place_type<fn_0>));

        using fn_1 = void();
        static_assert(!my_matcher{}(std::in_place_type<fn_1>));

        using fn_2 = void(AS_NAMESPACE_QUALIFIER asIScriptEngine&);
        static_assert(my_matcher{}(std::in_place_type<fn_2>));
    }

    return true;
}

consteval bool test_detail_validator_obj()
{
    using asbind20::detail::signature_matcher;
    using namespace asbind20::detail::validator;

    {
        struct my_struct
        {};

        using my_matcher = signature_matcher<void_>;

        using fn_0 = void(my_struct*);
        static_assert(my_matcher{}(
            std::in_place_type<fn_0>,
            asbind20::detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
        ));
        static_assert(my_matcher{}(
            std::in_place_type<fn_0>,
            asbind20::detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
        ));

        using fn_1 = void(my_struct*, AS_NAMESPACE_QUALIFIER asIScriptEngine*);
        static_assert(!my_matcher{}(
            std::in_place_type<fn_1>,
            asbind20::detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
        ));
        static_assert(!my_matcher{}(
            std::in_place_type<fn_1>,
            asbind20::detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
        ));

        using fn_2 = void(my_struct&, AS_NAMESPACE_QUALIFIER asIScriptEngine*);
        static_assert(!my_matcher{}(
            std::in_place_type<fn_2>,
            asbind20::detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
        ));
    }

    {
        struct my_struct
        {};

        using my_matcher = signature_matcher<
            void_,
            by_addr<AS_NAMESPACE_QUALIFIER asIScriptEngine>>;

        using fn_0 = void(my_struct*);
        static_assert(!my_matcher{}(
            std::in_place_type<fn_0>,
            asbind20::detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
        ));
        static_assert(!my_matcher{}(
            std::in_place_type<fn_0>,
            asbind20::detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
        ));

        using fn_1 = void(my_struct*, AS_NAMESPACE_QUALIFIER asIScriptEngine*);
        static_assert(!my_matcher{}(
            std::in_place_type<fn_1>,
            asbind20::detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
        ));
        static_assert(my_matcher{}(
            std::in_place_type<fn_1>,
            asbind20::detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
        ));

        using fn_2 = void(AS_NAMESPACE_QUALIFIER asIScriptEngine*, my_struct*);
        static_assert(my_matcher{}(
            std::in_place_type<fn_2>,
            asbind20::detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
        ));
        static_assert(!my_matcher{}(
            std::in_place_type<fn_2>,
            asbind20::detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
        ));
    }

    return true;
}

static_assert(test_detail_validator());
static_assert(test_detail_validator_obj());
