#ifndef ASBIND20_EXT_ASSERT_HPP
#define ASBIND20_EXT_ASSERT_HPP

#include "../asbind.hpp"
#include <functional>

namespace asbind20::ext
{
using assert_handler_t = void(std::string_view);

std::function<assert_handler_t> register_script_assert(
    asIScriptEngine* engine,
    std::function<assert_handler_t> callback,
    bool set_ex = true,
    asIStringFactory* str_factory = nullptr
);
} // namespace asbind20::ext

#endif
