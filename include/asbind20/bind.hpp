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
class [[nodiscard]] access_mask : public engine_ref_holder
{
    using my_base = engine_ref_holder;

public:
    using mask_type = AS_NAMESPACE_QUALIFIER asDWORD;

    access_mask() = delete;
    access_mask(const access_mask&) = delete;

    access_mask& operator=(const access_mask&) = delete;

    access_mask(
        engine_pointer engine,
        mask_type mask
    )
        : my_base(engine)
    {
        set_impl(mask);
    }

    access_mask(
        engine_reference engine,
        mask_type mask
    )
        : my_base(engine)
    {
        set_impl(mask);
    }

    ~access_mask()
    {
        get_engine()->SetDefaultAccessMask(m_prev);
    }

    /**
     * @brief Get the previous mask
     */
    [[nodiscard]]
    mask_type previous() const noexcept
    {
        return m_prev;
    }

private:
    mask_type m_prev{};

    void set_impl(mask_type mask)
    {
        m_prev = get_engine()->SetDefaultAccessMask(mask);
    }
};

class [[nodiscard]] namespace_ : public engine_ref_holder
{
    using my_base = engine_ref_holder;

public:
    namespace_() = delete;
    namespace_(const namespace_&) = delete;

    namespace_& operator=(const namespace_&) = delete;

    explicit namespace_(engine_pointer engine)
        : my_base(engine)
    {
        set_as_global();
    }

    explicit namespace_(engine_reference engine)
        : my_base(engine)
    {
        set_as_global();
    }

    namespace_(
        engine_pointer engine,
        std::string_view ns,
        bool nested = true
    )
        : my_base(engine)
    {
        set_as(ns, nested);
    }

    namespace_(
        engine_reference engine,
        std::string_view ns,
        bool nested = true
    )
        : my_base(engine)
    {
        set_as(ns, nested);
    }

    ~namespace_()
    {
        set_ns_impl(m_prev.c_str());
    }

    [[nodiscard]]
    const std::string& previous() const noexcept
    {
        return m_prev;
    }

private:
    std::string m_prev;

    void set_as_global()
    {
        m_prev = get_engine()->GetDefaultNamespace();
        set_ns_impl("");
    }

    void set_as(std::string_view ns, bool nested)
    {
        m_prev = get_engine()->GetDefaultNamespace();
        if(nested && ns.empty()) [[unlikely]]
            return; // Same as the previous namespace

        // Synthesize "prev::ns" declaration when in nested mode, ns is non-empty,
        // and a previous namespace exists to nest under.
        // Otherwise (non-nested, or no previous namespace), pass ns directly.
        const bool synthesize_decl =
            nested ? (!ns.empty() && !m_prev.empty()) : false;
        if(synthesize_decl)
        {
            set_ns_impl(
                string_concat(m_prev, "::", ns).c_str()
            );
        }
        else
        {
            util::with_cstr(
                &namespace_::set_ns_impl,
                this,
                ns
            );
        }
    }

    void set_ns_impl(const char* ns) const
    {
        [[maybe_unused]]
        int r = get_engine()->SetDefaultNamespace(
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
    using flag_type = AS_NAMESPACE_QUALIFIER asQWORD;

    basic_interface() = delete;
    basic_interface(const basic_interface&) = default;

    basic_interface& operator=(const basic_interface&) = delete;

    basic_interface(engine_pointer engine, std::string name)
        : my_base(engine), m_name(std::move(name))
    {
        do_register();
    }

    basic_interface(engine_reference engine, std::string name)
        : basic_interface(std::addressof(engine), std::move(name)) {}

    template <bool AppendOnly>
    basic_interface(appending_t<AppendOnly>, engine_pointer engine, std::string name)
        : my_base(engine), m_name(std::move(name))
    {
        if(typeinfo_pointer ti = this->get_engine()->GetTypeInfoByName(m_name.c_str()); ti)
        {
#ifndef ASBIND20_CONFIG_NO_APPEND_CHECK
            [[maybe_unused]]
            flag_type flags = ti->GetFlags();
            ASBIND20_ASSERT(!(flags & AS_NAMESPACE_QUALIFIER asOBJ_VALUE));
#endif
        }
        else if constexpr(!AppendOnly)
        {
            do_register();
        }
    }

    template <bool AppendOnly>
    basic_interface(appending_t<AppendOnly>, engine_reference engine, std::string name)
        : basic_interface(
              appending_t<AppendOnly>{},
              std::addressof(engine),
              std::move(name)
          )
    {}

    template <string_like StringLike>
    basic_interface(
        engine_pointer engine,
        StringLike&& name
    )
        : basic_interface(
              engine,
              util::string_like_to_string(std::forward<StringLike>(name))
          )
    {}

    template <string_like StringLike>
    basic_interface(
        engine_reference engine,
        StringLike&& name
    )
        : basic_interface(
              std::addressof(engine),
              std::forward<StringLike>(name)
          )
    {}

    template <bool AppendOnly, string_like StringLike>
    basic_interface(
        appending_t<AppendOnly>,
        engine_pointer engine,
        StringLike&& name
    )
        : basic_interface(
              appending_t<AppendOnly>{},
              engine,
              util::string_like_to_string(std::forward<StringLike>(name))
          )
    {}

    template <bool AppendOnly, string_like StringLike>
    basic_interface(
        appending_t<AppendOnly>,
        engine_reference engine,
        StringLike&& name
    )
        : basic_interface(
              appending_t<AppendOnly>{},
              std::addressof(engine),
              std::forward<StringLike>(name)
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

    void do_register()
    {
        int r = this->get_engine()->RegisterInterface(m_name.c_str());
        listener_type_traits::on_interface(
            this->get_listener(), *this, r
        );
    }
};

using interface = basic_interface<>;

/**
 * @brief Generic calling convention for message callback is not supported.
 */
int set_message_callback(
    engine_pointer engine,
    generic_function gfn,
    void* obj = nullptr
) = delete;

/**
 * @brief Set the message callback.
 */
template <native_function Callback>
requires(!std::is_member_function_pointer_v<Callback>)
int set_message_callback(
    engine_pointer engine,
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
        to_asSFuncPtr(fn),
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
    engine_pointer engine,
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
        to_asSFuncPtr(fn),
        aux.get_address(),
        AS_NAMESPACE_QUALIFIER asCALL_THISCALL
    );
}

/**
 * @brief Generic calling convention for exception translator is not supported.
 */
int set_exception_translator(
    engine_pointer engine,
    generic_function gfn,
    void* obj = nullptr
) = delete;

/**
 * @brief Set the exception translator.
 */
template <native_function Callback>
requires(!std::is_member_function_pointer_v<Callback>)
int set_exception_translator(
    engine_pointer engine,
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
        to_asSFuncPtr(fn),
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
    engine_pointer engine,
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
        to_asSFuncPtr(fn),
        aux.get_address(),
        AS_NAMESPACE_QUALIFIER asCALL_THISCALL
    );
}
} // namespace asbind20

#endif
