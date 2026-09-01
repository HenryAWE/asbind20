#ifndef ASBIND20_META_TYPE_NAME_HPP
#define ASBIND20_META_TYPE_NAME_HPP

#include <string_view>
#include "refl_common.hpp"

namespace asbind20::meta
{
namespace detail
{
    template <typename T>
    consteval std::string_view type_name_of_impl()
    {
        // Original implementation:
        // https://gist.github.com/HenryAWE/e7f6a2274be307ac04e557fe062e9ebd

        std::string_view result;

#if defined _MSC_VER && !defined __clang__ // clang-cl also defines _MSC_VER
        {
            result = __FUNCSIG__;
            auto start = result.find("type_name_of_impl<");
            start += 15; // strlen("type_name_of_impl<")
            auto stop = result.rfind('>');

            result = result.substr(start, stop - start);
        }

#elif defined __clang__
        {
            result = __PRETTY_FUNCTION__;
            auto start = result.find("T = ");
            start += 4; // strlen("T = ")
            auto stop = result.rfind(']');

            result = result.substr(start, stop - start);
        }

#elif defined __GNUC__
        {
            result = __PRETTY_FUNCTION__;
            auto start = result.find("with T = ");
            start += 9; // strlen("with T = ")
            auto stop = result.find(';', start);
            result = result.substr(start, stop - start);
        }

#else
        static_assert(!sizeof(T), "Unknown compiler");

#endif

        // Remove namespace prefix and template arguments
        {
            result = result.substr(0, result.find('<'));

            auto i = result.find("::");
            while(i != result.npos)
            {
                i += 2; // strlen("::")
                result = result.substr(i);
                i = result.find("::");
            }
        }

        return result;
    }
} // namespace detail

template <typename T>
constexpr auto fixed_type_name() noexcept
{
    constexpr std::string_view type_name = detail::type_name_of_impl<std::remove_cvref_t<T>>();
    constexpr std::size_t size = type_name.size();

    return [&]<std::size_t... Is>(std::index_sequence<Is...>)
    {
        return util::fixed_string<size>(type_name[Is]...);
    }(std::make_index_sequence<size>());
}


} // namespace asbind20

#endif
