#include <asbind20/bind.hpp>
#include <algorithm>

namespace asbind20
{
namespace detail
{
    std::string generate_member_funcdef(
        const char* type, std::string_view funcdef
    )
    {
        // Firstly, find the begin of parameters
        auto param_being = std::find(funcdef.rbegin(), funcdef.rend(), '(');
        if(param_being != funcdef.rend())
        {
            ++param_being; // Skip '('

            // Skip possible whitespaces between the function name and the parameters
            param_being = std::find_if_not(
                param_being,
                funcdef.rend(),
                [](char ch) -> bool
                { return ch == ' '; }
            );
        }
        auto name_begin = std::find_if_not(
            param_being,
            funcdef.rend(),
            [](char ch) -> bool
            {
                return ('0' <= ch && ch <= '9') ||
                       ('a' <= ch && ch <= 'z') ||
                       ('A' <= ch && ch <= 'Z') ||
                       ch == '_' ||
                       static_cast<std::uint8_t>(ch) > 127;
            }
        );

        using namespace std::literals;

        std::string_view return_type(funcdef.begin(), name_begin.base());
        if(return_type.back() == ' ')
            return_type.remove_suffix(1);
        return string_concat(
            return_type,
            ' ',
            type,
            "::"sv,
            std::string_view(name_begin.base(), funcdef.end())
        );
    }
} // namespace detail
} // namespace asbind20
