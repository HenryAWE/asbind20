/**
 * @file enum.hpp
 * @author HenryAWE
 * @brief Bind generator for enumerations
 */
#ifndef ASBIND20_BIND_ENUM_HPP
#define ASBIND20_BIND_ENUM_HPP

#pragma once

#include <cassert>
#include <concepts>
#include <string>
#include "../detail/include_as.hpp"
#include "../meta.hpp"

namespace asbind20
{
template <typename Enum>
requires(std::is_enum_v<Enum> || std::integral<Enum>)
class enum_
{
public:
    using enum_type = Enum;

    enum_() = delete;
    enum_(const enum_&) = default;

    enum_(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, std::string name
    )
        : m_engine(engine), m_name(std::move(name))
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterEnum(m_name.c_str());
        assert(r >= 0);
    }

    template <std::convertible_to<std::string_view> StringView>
    enum_(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        StringView name
    )
        : enum_(
              engine,
              std::string(static_cast<std::string_view>(name))
          )
    {}

    enum_& value(Enum val, std::string_view decl)
    {
        [[maybe_unused]]
        int r = with_cstr(
            &AS_NAMESPACE_QUALIFIER asIScriptEngine::RegisterEnumValue,
            m_engine,
            m_name,
            decl,
            static_cast<int>(val)
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Registering an enum value. Its declaration will be generated from its name in C++.
     *
     * @note This function has some limitations. @sa static_enum_name
     *
     * @tparam Value Enum value
     */
    template <Enum Value>
    requires(std::is_enum_v<Enum>)
    enum_& value()
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterEnumValue(
            m_name.c_str(),
            meta::fixed_enum_name<Value>().c_str(),
            static_cast<int>(Value)
        );
        assert(r >= 0);

        return *this;
    }

    [[nodiscard]]
    auto get_engine() const noexcept
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine;
    }

    [[nodiscard]]
    const std::string& get_name() const noexcept
    {
        return m_name;
    }

private:
    AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine;
    std::string m_name;
};
} // namespace asbind20

#endif
