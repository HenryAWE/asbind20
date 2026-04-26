/**
 * @file bind.hpp
 * @author HenryAWE
 * @brief Binding generators
 */

#ifndef ASBIND20_BIND_HPP
#define ASBIND20_BIND_HPP

#pragma once

#include <cassert>
#include "utility.hpp"
#include "bind/common.hpp"

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

    access_mask& operator=(const access_mask&) = delete;

    access_mask(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        mask_type mask
    )
        : m_engine(engine)
    {
        ASBIND20_ASSERT(engine != nullptr);
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

    [[nodiscard]]
    mask_type previous() const noexcept
    {
        return m_prev;
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
        ASBIND20_ASSERT(engine != nullptr);
        set_ns_impl("");
    }

    namespace_(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        std::string_view ns,
        bool nested = true
    )
        : m_engine(engine), m_prev(engine->GetDefaultNamespace())
    {
        ASBIND20_ASSERT(engine != nullptr);
        if(nested)
        {
            if(!ns.empty()) [[likely]]
            {
                if(m_prev.empty())
                {
                    util::with_cstr(
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
            util::with_cstr(
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

    [[nodiscard]]
    const std::string& previous() const noexcept
    {
        return m_prev;
    }

private:
    AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine;
    std::string m_prev;

    void set_ns_impl(const char* ns) const
    {
        [[maybe_unused]]
        int r = m_engine->SetDefaultNamespace(
            ns
        );
        ASBIND20_ASSERT(r >= 0);
    }
};

template <typename Listener = default_listener>
class basic_interface : public binding_generator_base<Listener>
{
    using my_base = binding_generator_base<Listener>;
    using listener_type_traits = listener_traits<Listener>;

public:
    using engine_pointer = AS_NAMESPACE_QUALIFIER asIScriptEngine*;

    basic_interface() = delete;
    basic_interface(const basic_interface&) = default;

    basic_interface& operator=(const basic_interface&) = delete;

    basic_interface(engine_pointer engine, std::string name)
        : my_base(engine), m_name(std::move(name))
    {
        int r = this->get_engine()->RegisterInterface(m_name.c_str());
        listener_type_traits::on_interface(
            this->get_listener(), *this, r
        );
    }

    template <string_like StringLike>
    basic_interface(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        StringLike&& name
    )
        : basic_interface(
              engine,
              util::string_like_to_string(std::forward<StringLike>(name))
          )
    {}

    basic_interface& method(cstring_ref decl)
    {
        int r = this->get_engine()->RegisterInterfaceMethod(
            m_name.c_str(),
            decl.c_str()
        );
        listener_type_traits::on_method(
            this->get_listener(), *this, r
        );
        return *this;
    }

    basic_interface& funcdef(std::string_view decl)
    {
        std::string full_decl = detail::generate_member_funcdef(
            m_name, decl
        );

        int r = this->get_engine()->RegisterFuncdef(full_decl.c_str());
        listener_type_traits::on_funcdef(
            this->get_listener(), *this, r
        );

        return *this;
    }

    [[nodiscard]]
    const std::string& get_name() const noexcept
    {
        return m_name;
    }

private:
    std::string m_name;
};

using interface = basic_interface<>;

/**
 * @brief Generic calling convention for message callback is not supported.
 */
int set_message_callback(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    generic_function gfn,
    void* obj = nullptr
) = delete;

/**
 * @brief Set the message callback.
 */
template <native_function Callback>
requires(!std::is_member_function_pointer_v<Callback>)
int set_message_callback(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    Callback fn,
    void* obj = nullptr
)
{
#ifndef ASBIND20_CONFIG_NO_COMPILE_TIME_CHECKS
    using matcher = detail::signature_matcher<
        detail::validator::void_,
        detail::validator::by_addr<AS_NAMESPACE_QUALIFIER asSMessageInfo>,
        detail::validator::by_addr<void>>;
    static_assert(
        matcher{}(std::in_place_type<Callback>),
        "Invalid message callback. The signature must be similar to void(asSMessageInfo*, void*)"
    );

#endif

    if(!engine) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return engine->SetMessageCallback(
        detail::to_asSFuncPtr(fn),
        obj,
        detail::deduce_function_callconv<Callback>()
    );
}

/**
 * @brief Set a member function as the message callback.
 */
template <native_function Callback, typename T>
requires(std::is_member_function_pointer_v<Callback>)
int set_message_callback(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    Callback fn,
    auxiliary_wrapper<T> aux
)
{
#ifndef ASBIND20_CONFIG_NO_COMPILE_TIME_CHECKS
    using matcher = detail::signature_matcher<
        detail::validator::void_,
        detail::validator::by_addr<AS_NAMESPACE_QUALIFIER asSMessageInfo>>;
    static_assert(
        matcher{}(std::in_place_type<Callback>),
        "Invalid message callback. The signature must be similar to void (T::*)(asSMessageInfo*)"
    );

#endif

    if(!engine) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return engine->SetMessageCallback(
        detail::to_asSFuncPtr(fn),
        aux.get_address(),
        AS_NAMESPACE_QUALIFIER asCALL_THISCALL
    );
}

/**
 * @brief Generic calling convention for exception translator is not supported.
 */
int set_exception_translator(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    generic_function gfn,
    void* obj = nullptr
) = delete;

/**
 * @brief Set the exception translator.
 */
template <native_function Callback>
requires(!std::is_member_function_pointer_v<Callback>)
int set_exception_translator(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    Callback fn,
    void* obj = nullptr
)
{
#ifndef ASBIND20_CONFIG_NO_COMPILE_TIME_CHECKS
    using matcher = detail::signature_matcher<
        detail::validator::void_,
        detail::validator::by_addr<AS_NAMESPACE_QUALIFIER asIScriptContext>,
        detail::validator::by_addr<void>>;
    static_assert(
        matcher{}(std::in_place_type<Callback>),
        "Invalid exception translator. The signature must be similar to void(asIScriptContext*, void*)"
    );

#endif

    if(!engine) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return engine->SetTranslateAppExceptionCallback(
        detail::to_asSFuncPtr(fn),
        obj,
        detail::deduce_function_callconv<Callback>()
    );
}

/**
 * @brief Set a member function as the exception translator.
 */
template <native_function Callback, typename T>
requires(std::is_member_function_pointer_v<Callback>)
int set_exception_translator(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    Callback fn,
    auxiliary_wrapper<T> aux
)
{
#ifndef ASBIND20_CONFIG_NO_COMPILE_TIME_CHECKS
    using matcher = detail::signature_matcher<
        detail::validator::void_,
        detail::validator::by_addr<AS_NAMESPACE_QUALIFIER asIScriptContext>>;
    static_assert(
        matcher{}(std::in_place_type<Callback>),
        "Invalid exception translator. The signature must be similar to void (T::*)(asIScriptContext*)"
    );

#endif

    if(!engine) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return engine->SetTranslateAppExceptionCallback(
        detail::to_asSFuncPtr(fn),
        aux.get_address(),
        AS_NAMESPACE_QUALIFIER asCALL_THISCALL
    );
}
} // namespace asbind20

#endif
