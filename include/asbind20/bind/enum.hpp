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
class enum_register_base
{
public:
#ifndef ASBIND20_HAS_ENUM_UNDERLYING_TYPE
    using script_enum_value_type = int;
#else
    using script_enum_value_type = AS_NAMESPACE_QUALIFIER asINT64;
#endif
};

template <typename Enum, typename UnderlyingType = int>
requires(std::is_enum_v<Enum> || std::integral<Enum>)
class enum_ : public enum_register_base
{
public:
#ifndef ASBIND20_HAS_ENUM_UNDERLYING_TYPE
    static_assert(
        std::same_as<UnderlyingType, int>, "Older AngelScript (<= 2.38) only allows int as underlying type of enum"
    );
#endif

    using enum_type = Enum;

    enum_() = delete;
    enum_(const enum_&) = default;

    enum_(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, std::string name
    )
        : m_engine(engine), m_name(std::move(name))
    {
        register_enum_type(m_name.c_str(), get_underlying());
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
            static_cast<script_enum_value_type>(val)
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
            static_cast<script_enum_value_type>(Value)
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

    /**
     * @brief Get the name of underlying type in fixed string
     */
    [[nodiscard]]
    static constexpr auto get_underlying() noexcept
    {
        return name_of<UnderlyingType>();
    }

private:
    AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine;
    std::string m_name;

    void register_enum_type(
        const char* name,
        [[maybe_unused]] const char* underlying
    )
    {
        [[maybe_unused]]
        int r = 0;
#ifdef ASBIND20_HAS_ENUM_UNDERLYING_TYPE
        r = m_engine->RegisterEnum(name, underlying);
#else // AngelScript <= 2.38
        r = m_engine->RegisterEnum(name);
#endif
        assert(r >= 0);
    }
};

/**
 * @brief Helper for register enum with underlying type
 */
template <typename Enum>
using enum_underlying = enum_<Enum, std::underlying_type_t<Enum>>;
} // namespace asbind20

#endif
