#ifndef ASBIND20_EXT_HELPER_HPP
#define ASBIND20_EXT_HELPER_HPP

#include <variant>
#include <compare>
#include "../asbind.hpp"

namespace asbind20::ext
{
bool is_primitive_type(int type_id);

asUINT sizeof_script_type(asIScriptEngine* engine, int type_id);

using primitive_t = std::variant<
    std::monostate, // void
    bool,
    std::int8_t,
    std::int16_t,
    std::int32_t,
    std::int64_t,
    std::uint8_t,
    std::uint16_t,
    std::uint32_t,
    std::uint64_t,
    float,
    double>;

primitive_t extract_primitive_value(asIScriptEngine* engine, void* ref, int type_id);

std::partial_ordering script_compare(
    asIScriptEngine* engine,
    void* lhs_ref,
    int lhs_type_id,
    void* rhs_ref,
    int rhs_type_id
);
} // namespace asbind20::ext

#endif
