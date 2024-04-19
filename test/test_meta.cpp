#include <gtest/gtest.h>
#include <asbind20/meta.hpp>

void check_compile_fp()
{
    using asbind20::function_traits;

    unsigned int (*fp)(float, int) = nullptr;

    using func_t = function_traits<decltype(fp)>;
    static_assert(!func_t::is_method());
    static_assert(!func_t::is_noexcept());
    static_assert(std::is_same_v<func_t::return_type, unsigned int>);
    static_assert(std::is_same_v<func_t::arg_type<0>, float>);
    static_assert(std::is_same_v<func_t::arg_type<1>, int>);
    static_assert(func_t::arg_count() == 2);
}

void check_compile_fp_noexcept()
{
    using asbind20::function_traits;

    unsigned int (*fp)(float, int) noexcept = nullptr;

    using func_t = function_traits<decltype(fp)>;
    static_assert(!func_t::is_method());
    static_assert(func_t::is_noexcept());
    static_assert(std::is_same_v<func_t::return_type, unsigned int>);
    static_assert(std::is_same_v<func_t::arg_type<0>, float>);
    static_assert(std::is_same_v<func_t::arg_type<1>, int>);
    static_assert(func_t::arg_count() == 2);
}

void check_compile_member_fp()
{
    using asbind20::function_traits;

    struct my_type
    {
        int func(float arg)
        {
            return 0;
        }
    };

    using func_t = function_traits<decltype(&my_type::func)>;
    static_assert(func_t::is_method());
    static_assert(std::is_same_v<func_t::class_type, my_type>);
    static_assert(std::is_same_v<func_t::return_type, int>);
    static_assert(std::is_same_v<func_t::arg_type<0>, float>);
    static_assert(func_t::arg_count() == 1);
}

void check_compile_lambda()
{
    using asbind20::function_traits;

    auto lambda = [](int, float, double) -> unsigned int
    {
        return 0;
    };
    using lambda_t = decltype(lambda);

    using func_t = function_traits<lambda_t>;
    static_assert(func_t::is_method());
    static_assert(std::is_same_v<func_t::class_type, lambda_t>);
    static_assert(std::is_same_v<func_t::return_type, unsigned int>);
    static_assert(std::is_same_v<func_t::arg_type<0>, int>);
    static_assert(std::is_same_v<func_t::arg_type<1>, float>);
    static_assert(func_t::arg_count() == 3);
    static_assert(std::is_same_v<func_t::first_arg_type, int>);
    static_assert(std::is_same_v<func_t::last_arg_type, double>);
}

void check_func_empty_arg()
{
    using func_t = asbind20::function_traits<int()>;

    static_assert(func_t::arg_count() == 0);
    static_assert(std::is_void_v<func_t::first_arg_type>);
    static_assert(std::is_void_v<func_t::last_arg_type>);
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
