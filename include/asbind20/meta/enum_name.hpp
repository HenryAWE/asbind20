#ifndef ASBIND20_META_ENUM_NAME_HPP
#define ASBIND20_META_ENUM_NAME_HPP

#include "../detail/config.hpp"
#include <string_view>
#ifdef ASBIND20_HAS_LIB_REFLECTION
#    include <meta>
#endif

namespace asbind20::meta
{
/**
 * @brief Get string from an enum value
 *
 * @note This function uses compiler extension to get name of enum.
 *       It cannot handle enum value that has same underlying value with another enum.
 *
 * @tparam Value Enum value
 */
template <auto Value>
requires(std::is_enum_v<decltype(Value)>)
consteval std::string_view static_enum_name()
{
#ifdef ASBIND20_HAS_LIB_REFLECTION

#    define ASBIND20_HAS_STATIC_ENUM_NAME "__cpp_lib_reflection"

    using enum_type = decltype(Value);

    static constexpr auto enumerators =
        std::define_static_array(std::meta::enumerators_of(^^enum_type));

    template for(constexpr auto enumerator : enumerators)
    {
        if([:enumerator:] == Value)
        {
            return std::meta::identifier_of(enumerator);
        }
    }

    throw "bad enum value";

#else
    std::string_view name;

#    if defined(__clang__) || defined(__GNUC__)
    name = __PRETTY_FUNCTION__;

    std::size_t start = name.find("Value = ") + 8;

#        ifdef __clang__
#            define ASBIND20_HAS_STATIC_ENUM_NAME "__PRETTY_FUNCTION__ (Clang)"

    std::size_t end = name.find_last_of(']');
#        else // GCC
#            define ASBIND20_HAS_STATIC_ENUM_NAME "__PRETTY_FUNCTION__ (GCC)"

    std::size_t end = std::min(name.find(';', start), name.find_last_of(']'));
#        endif

    name = std::string_view(name.data() + start, end - start);

#    elif defined(_MSC_VER)
#        define ASBIND20_HAS_STATIC_ENUM_NAME "__FUNCSIG__"

    name = __FUNCSIG__;
    std::size_t start = name.find("static_enum_name<") + 17;
    std::size_t end = name.find_last_of('>');
    name = std::string_view(name.data() + start, end - start);

#    else
    static_assert(false, "Not supported");

#    endif

    // Remove qualifier
    std::size_t qual_end = name.rfind("::");
    if(qual_end != std::string_view::npos)
    {
        qual_end += 2; // skip "::"
        return name.substr(qual_end);
    }

    return name;

#endif
}
} // namespace asbind20::meta

#endif
