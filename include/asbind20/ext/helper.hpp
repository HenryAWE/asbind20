#ifndef ASBIND20_EXT_HELPER_HPP
#define ASBIND20_EXT_HELPER_HPP

#pragma once

#include <variant>
#include <compare>
#include "../asbind.hpp"

namespace asbind20::ext
{
bool is_primitive_type(int type_id) noexcept;

asUINT sizeof_script_type(asIScriptEngine* engine, int type_id);

/**
 * @brief Extracts the contents from a script string
 *
 * @param factory The string factory
 * @param str The script string
 *
 * @return std::string Result string
 */
std::string extract_string(asIStringFactory* factory, void* str);

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

/**
 * @brief Extracts the value of a primitive type
 *
 * @param engine The script engine
 * @param ref The reference
 * @param type_id The type id
 *
 * @return primitive_t Result value
 *
 * @exception std::invalid_argument if the type is not a primitive type
 */
primitive_t extract_primitive_value(asIScriptEngine* engine, void* ref, int type_id);

std::partial_ordering script_compare(
    asIScriptEngine* engine,
    void* lhs_ref,
    int lhs_type_id,
    void* rhs_ref,
    int rhs_type_id
);

int exec(
    asIScriptEngine* engine,
    std::string_view code,
    asIScriptContext* ctx = nullptr
);

/**
 * @brief Register hash support for primitive types
 */
void register_script_hash(asIScriptEngine* engine);
} // namespace asbind20::ext

#endif
