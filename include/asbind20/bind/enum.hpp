/**
 * @file bind/enum.hpp
 * @author HenryAWE
 * @brief Bind generator for enumerations
 */

#ifndef ASBIND20_BIND_ENUM_HPP
#define ASBIND20_BIND_ENUM_HPP

#pragma once

#include <cassert>
#include <concepts>
#include <string>
#include "../detail/compat.hpp"
#include "../meta.hpp"
#include "common.hpp"

namespace asbind20
{
template <
    typename Enum,
    typename UnderlyingType = int,
    typename Listener = default_listener>
requires(std::is_enum_v<Enum> || std::integral<Enum>)
class enum_ : public binding_generator_base<Listener>
{
    using my_base = binding_generator_base<Listener>;
    using listener_type_traits = listener_traits<Listener>;

public:
#ifndef ASBIND20_HAS_ENUM_UNDERLYING_TYPE
    static_assert(
        std::same_as<UnderlyingType, int>, "Older AngelScript (<= 2.38) only allows int as underlying type of enum"
    );
#endif

    using enum_type = Enum;
    using underlying_type = UnderlyingType;
    using engine_pointer = AS_NAMESPACE_QUALIFIER asIScriptEngine*;

    enum_() = delete;
    enum_(const enum_&) = default;

    enum_& operator=(const enum_&) = delete;

    enum_(engine_pointer engine, std::string name)
        : my_base(engine), m_name(std::move(name))
    {
        register_enum_type(m_name, get_underlying_name());
    }

    template <bool AppendOnly>
    enum_(appending_t<AppendOnly>, engine_pointer engine, std::string name)
        : my_base(engine), m_name(std::move(name))
    {
        append_to_enum(AppendOnly, m_name);
    }

    template <string_like StringLike>
    enum_(engine_pointer engine, StringLike&& name)
        : enum_(
              engine,
              util::string_like_to_string(std::forward<StringLike>(name))
          )
    {}

    template <bool AppendOnly,string_like StringLike>
    enum_(appending_t<AppendOnly>, engine_pointer engine, StringLike&& name)
        : enum_(
              appending_t<AppendOnly>{},
              engine,
              util::string_like_to_string(std::forward<StringLike>(name))
          )
    {}

    enum_& value(cstring_ref decl, Enum val)
    {
        register_enum_val(
            decl,
            static_cast<compat::script_enum_value_type>(val)
        );

        return *this;
    }

    // Kept for consistency with .value<EnumVal>(name)
    enum_& value(Enum val, cstring_ref decl)
    {
        this->value(decl, val);

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
        register_enum_val(
            meta::fixed_enum_name<Value>(),
            static_cast<compat::script_enum_value_type>(Value)
        );

        return *this;
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
    static constexpr auto get_underlying_name() noexcept
    {
        return name_of<UnderlyingType>();
    }

private:
    std::string m_name;

    void register_enum_type(
        cstring_ref name,
        [[maybe_unused]] cstring_ref underlying
    )
    {
        // clang-format off
        int r = this->get_engine()->RegisterEnum(
            name.c_str()
#ifdef ASBIND20_HAS_ENUM_UNDERLYING_TYPE
            , underlying.c_str()
#endif
        );
        // clang-format on

        listener_type_traits::on_enum(
            this->get_listener(), *this, r
        );
    }

    void append_to_enum(bool append_only, cstring_ref name)
    {
        AS_NAMESPACE_QUALIFIER asITypeInfo* ti = this->get_engine()->GetTypeInfoByName(name.c_str());
        if(ti)
        {
            [[maybe_unused]]
            int r = ti->GetTypeId();
            ASBIND20_ASSERT(is_enum_type(r));
        }
        else if(!append_only)
        {
            register_enum_type(name, get_underlying_name());
        }
    }

    void register_enum_val(
        cstring_ref name,
        compat::script_enum_value_type val
    )
    {
        int r = this->get_engine()->RegisterEnumValue(
            m_name.c_str(),
            name.c_str(),
            val
        );
        listener_type_traits::on_enum_value(
            this->get_listener(), *this, r
        );
    }
};

/**
 * @brief Helper for register enum with underlying type
 */
template <typename Enum>
using enum_underlying = enum_<Enum, std::underlying_type_t<Enum>>;
} // namespace asbind20

#endif
