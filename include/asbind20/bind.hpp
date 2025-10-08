/**
 * @file bind.hpp
 * @author HenryAWE
 * @brief Binding generators
 */

#ifndef ASBIND20_BIND_HPP
#define ASBIND20_BIND_HPP

#pragma once

#include <cassert>
#include <concepts>
#include "utility.hpp"

// IWYU pragma: begin_exports

#include "bind/global.hpp"
#include "bind/class.hpp"
#include "bind/enum.hpp"

// IWYU pragma: end_exports

namespace asbind20
{
class [[nodiscard]] access_mask
{
public:
    using mask_type = AS_NAMESPACE_QUALIFIER asDWORD;

    access_mask() = delete;
    access_mask(const access_mask&) = delete;

    access_mask(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        mask_type mask
    )
        : m_engine(engine)
    {
        m_prev = m_engine->SetDefaultAccessMask(mask);
    }

    ~access_mask()
    {
        m_engine->SetDefaultAccessMask(m_prev);
    }

    [[nodiscard]]
    auto get_engine() const noexcept
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine;
    }

private:
    AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine;
    mask_type m_prev;
};

class [[nodiscard]] namespace_
{
public:
    namespace_() = delete;
    namespace_(const namespace_&) = delete;

    namespace_& operator=(const namespace_&) = delete;

    namespace_(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
        : m_engine(engine), m_prev(engine->GetDefaultNamespace())
    {
        set_ns_impl("");
    }

    namespace_(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        std::string_view ns,
        bool nested = true
    )
        : m_engine(engine), m_prev(engine->GetDefaultNamespace())
    {
        if(nested)
        {
            if(!ns.empty()) [[likely]]
            {
                if(m_prev.empty())
                {
                    with_cstr(
                        [this](const char* ns)
                        { set_ns_impl(ns); },
                        ns
                    );
                }
                else
                {
                    set_ns_impl(
                        string_concat(m_prev, "::", ns).c_str()
                    );
                }
            }
        }
        else
        {
            with_cstr(
                [this](const char* ns)
                { set_ns_impl(ns); },
                ns
            );
        }
    }

    ~namespace_()
    {
        set_ns_impl(m_prev.c_str());
    }

    [[nodiscard]]
    auto get_engine() const noexcept
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine;
    }

private:
    AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine;
    std::string m_prev;

    void set_ns_impl(const char* ns)
    {
        [[maybe_unused]]
        int r = m_engine->SetDefaultNamespace(
            ns
        );
        assert(r >= 0);
    }
};

class interface
{
public:
    interface() = delete;
    interface(const interface&) = default;

    interface(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, std::string name
    )
        : m_engine(engine), m_name(std::move(name))
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterInterface(m_name.c_str());
        assert(r >= 0);
    }

    template <std::convertible_to<std::string_view> StringView>
    interface(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        StringView name
    )
        : interface(
              engine,
              std::string(static_cast<std::string_view>(name))
          )
    {}

    interface& method(std::string_view decl)
    {
        [[maybe_unused]]
        int r = with_cstr(
            &AS_NAMESPACE_QUALIFIER asIScriptEngine::RegisterInterfaceMethod,
            m_engine,
            m_name,
            decl
        );
        assert(r >= 0);

        return *this;
    }

    interface& funcdef(std::string_view decl)
    {
        std::string full_decl = detail::generate_member_funcdef(
            m_name, decl
        );

        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterFuncdef(full_decl.data());
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
