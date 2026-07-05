/**
 * @file bind/common.hpp
 * @author HenryAWE
 * @brief Common code for binding generator
 */

#ifndef ASBIND20_BIND_COMMON_HPP
#define ASBIND20_BIND_COMMON_HPP

#pragma once

#include <cassert>
#include <string_view>
#include <algorithm>
#include <concepts>
#include "../meta.hpp"
#include "../utility.hpp"
#include "../detail/calling_convention.hpp"
#include "listener.hpp"

namespace asbind20
{
template <bool AppendOnly>
struct appending_t
{};

constexpr inline appending_t<true> appending{};
constexpr inline appending_t<false> try_appending{};

struct use_generic_t
{};

constexpr inline use_generic_t use_generic{};

struct use_explicit_t
{};

constexpr inline use_explicit_t use_explicit{};

template <bool ObjFirst>
struct obj_loc_t
{
    static constexpr bool is_obj_first = ObjFirst;
};

template <bool ObjFirst>
inline constexpr obj_loc_t<ObjFirst> obj_loc;

inline constexpr obj_loc_t<true> objfirst{};
inline constexpr obj_loc_t<false> objlast{};

namespace detail
{
    template <bool ObjFirst>
    consteval auto conv_of_loc(obj_loc_t<ObjFirst>, bool is_thiscall)
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        if constexpr(ObjFirst)
        {
            return is_thiscall ?
                       AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST :
                       AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST;
        }
        else
        {
            return is_thiscall ?
                       AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST :
                       AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST;
        }
    }

    template <typename Func>
    auto to_asSFuncPtr(Func f)
        -> AS_NAMESPACE_QUALIFIER asSFuncPtr
    {
        // Reference: asFUNCTION and asMETHOD from the AngelScript interface
        if constexpr(std::is_member_function_pointer_v<Func>)
            return AS_NAMESPACE_QUALIFIER asSMethodPtr<sizeof(f)>::Convert(f);
        else
            return AS_NAMESPACE_QUALIFIER asFunctionPtr(f);
    }

    // Helper for retrieving actual auxiliary type from the helper
    template <typename Auxiliary>
    struct auxiliary_traits
    {
        using value_type = Auxiliary;
    };

    template <>
    struct auxiliary_traits<this_type_t>
    {
        using value_type = AS_NAMESPACE_QUALIFIER asITypeInfo;
    };

    template <typename FuncSig>
    requires(!std::is_member_function_pointer_v<FuncSig>)
    constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes deduce_function_callconv()
    {
        static_assert(!std::convertible_to<FuncSig, generic_function>);

        /*
        On x64 and many platforms (like arm64), CDECL and STDCALL have the same effect.
        It's safe to treat all global functions as CDECL.
        See: https://www.gamedev.net/forums/topic/715839-question-about-calling-convention-when-registering-functions-on-x64-platform/

        Only some platforms like x86 need to treat the STDCALL separately.
        */

#ifdef ASBIND20_HAS_STANDALONE_STDCALL
        if constexpr(meta::is_stdcall_v<FuncSig>)
            return AS_NAMESPACE_QUALIFIER asCALL_STDCALL;
        else
            return AS_NAMESPACE_QUALIFIER asCALL_CDECL;

#else
        return AS_NAMESPACE_QUALIFIER asCALL_CDECL;
#endif
    }

    template <typename T, typename Class>
    consteval bool is_this_arg(bool try_void_ptr)
    {
        if(try_void_ptr)
        {
            // For user wrapper for placement new
            if constexpr(std::same_as<T, void*>)
                return true;
        }

        return std::same_as<T, Class*> ||
               std::same_as<T, const Class*> ||
               std::same_as<T, Class&> ||
               std::same_as<T, const Class&>;
    }

    consteval auto cdecl_method_callconv(std::size_t arg_count, bool obj_first)
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        // The AngelScript itself seems to prefer OBJLAST
        // if both calling conventions are available.
        // We are following this convention here.
        if(obj_first)
        {
            return arg_count == 1 ?
                       AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST :
                       AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST;
        }
        else
            return AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST;
    }

    template <typename Class, typename FuncSig, bool TryVoidPtr = false>
    consteval auto deduce_method_callconv() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        if constexpr(std::is_member_function_pointer_v<FuncSig>)
            return AS_NAMESPACE_QUALIFIER asCALL_THISCALL;
        else if constexpr(std::convertible_to<FuncSig, generic_function>)
            return AS_NAMESPACE_QUALIFIER asCALL_GENERIC;
        else
        {
            using traits = function_traits<FuncSig>;
            using first_arg_t = std::remove_cv_t<typename traits::first_arg_type>;
            using last_arg_t = std::remove_cv_t<typename traits::last_arg_type>;

            constexpr bool obj_first = is_this_arg<first_arg_t, Class>(false);
            constexpr bool obj_last = is_this_arg<last_arg_t, Class>(false);

            if constexpr(obj_last || obj_first)
            {
                return cdecl_method_callconv(
                    traits::arg_count_v, obj_first
                );
            }
            else
            {
                if constexpr(!TryVoidPtr)
                    static_assert(obj_last || obj_first, "Missing object parameter");

                constexpr bool void_obj_first = is_this_arg<first_arg_t, Class>(true);
                constexpr bool void_obj_last = is_this_arg<last_arg_t, Class>(true);

                static_assert(void_obj_last || void_obj_first, "Missing object/void* parameter");

                return cdecl_method_callconv(
                    traits::arg_count_v, void_obj_first
                );
            }
        }
    }

    template <typename Class, typename FuncSig, typename Auxiliary>
    consteval auto deduce_method_callconv_aux() noexcept
    {
        static_assert(std::is_member_function_pointer_v<FuncSig>);

        using traits = function_traits<FuncSig>;
        using first_arg_t = std::remove_cv_t<typename traits::first_arg_type>;
        using last_arg_t = std::remove_cv_t<typename traits::last_arg_type>;

        constexpr bool obj_first = is_this_arg<first_arg_t, Class>(false);
        constexpr bool obj_last = is_this_arg<last_arg_t, Class>(false);

        static_assert(obj_last || obj_first, "Missing object parameter");

        // The AngelScript itself seems to prefer OBJLAST
        // if both calling conventions are available.
        // We are following this convention here.
        if(obj_first)
        {
            return traits::arg_count_v == 1 ?
                       AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST :
                       AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST;
        }
        else
            return AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST;
    }

    template <AS_NAMESPACE_QUALIFIER asEBehaviours Beh, typename Class, typename FuncSig>
    consteval auto deduce_beh_callconv() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        if constexpr(
            Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_TEMPLATE_CALLBACK ||
            Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY ||
            Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY
        )
            return deduce_function_callconv<FuncSig>();
        else if constexpr(std::is_member_function_pointer_v<FuncSig>)
            return AS_NAMESPACE_QUALIFIER asCALL_THISCALL;
        else
        {
            constexpr bool try_void_ptr =
                Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT ||
                Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_CONSTRUCT;

            return deduce_method_callconv<Class, FuncSig, try_void_ptr>();
        }
    }

    template <AS_NAMESPACE_QUALIFIER asEBehaviours Beh, typename Class, typename FuncSig, typename Auxiliary>
    consteval auto deduce_beh_callconv_aux() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        if constexpr(Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_TEMPLATE_CALLBACK)
        {
            if constexpr(std::is_member_function_pointer_v<FuncSig>)
                return AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL;
            else
                return deduce_function_callconv<FuncSig>();
        }
        else if constexpr(
            Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY ||
            Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY
        )
        {
            if constexpr(std::is_member_function_pointer_v<FuncSig>)
            {
                return AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL;
            }
            else
            {
                // According to the AngelScript documentation, the factory function with auxiliary object uses
                // asCALL_CDECL_OBJFIRST/LAST for native calling convention.
                // See: https://www.angelcode.com/angelscript/sdk/docs/manual/doc_reg_basicref.html#doc_reg_basicref_1_1

                using traits = function_traits<FuncSig>;
                using first_arg_t = std::remove_cv_t<typename traits::first_arg_type>;
                using last_arg_t = std::remove_cv_t<typename traits::last_arg_type>;
                using aux_t = typename auxiliary_traits<Auxiliary>::value_type;

                constexpr bool obj_first = is_this_arg<first_arg_t, aux_t>(false);
                constexpr bool obj_last = is_this_arg<last_arg_t, aux_t>(false);

                static_assert(obj_last || obj_first, "Missing auxiliary object parameter");

                return cdecl_method_callconv(
                    traits::arg_count_v, obj_first
                );
            }
        }
        else
        {
            return deduce_method_callconv_aux<Class, FuncSig, Auxiliary>();
        }
    }

    template <typename Class, noncapturing_native_lambda Lambda>
    consteval auto deduce_lambda_callconv()
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        using function_type = decltype(+std::declval<const Lambda>());

        return deduce_method_callconv<Class, function_type>();
    }

    inline std::string generate_member_funcdef(
        std::string_view type, std::string_view funcdef
    )
    {
        // Firstly, find the start of parameters
        auto param_begin = std::find(funcdef.rbegin(), funcdef.rend(), '(');
        if(param_begin != funcdef.rend())
        {
            ++param_begin; // Skip '('

            // Skip possible whitespaces between the function name and the parameters
            param_begin = std::find_if_not(
                param_begin,
                funcdef.rend(),
                [](char ch) -> bool
                { return ch == ' '; }
            );
        }
        const auto name_begin = std::find_if_not(
            param_begin,
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
            "::",
            std::string_view(name_begin.base(), funcdef.end())
        );
    }
} // namespace detail

template <typename T, typename BindingGenerator>
concept usable_by_generator = requires(T&& t, BindingGenerator& gen) {
    t(gen);
};

class engine_ref_holder
{
public:
    engine_ref_holder() = delete;
    engine_ref_holder(const engine_ref_holder&) noexcept = default;

    engine_ref_holder& operator=(const engine_ref_holder&) noexcept = default;

    [[nodiscard]]
    engine_pointer get_engine() const noexcept
    {
        return m_engine;
    }

protected:
    explicit engine_ref_holder(engine_pointer engine) noexcept
        : m_engine(engine)
    {
        ASBIND20_ASSERT(engine != nullptr);
    }

private:
    engine_pointer m_engine;
};

template <typename Listener = default_listener>
class binding_generator_base : public engine_ref_holder
{
public:
    binding_generator_base(const binding_generator_base&) noexcept(std::is_nothrow_copy_constructible_v<Listener>) = default;

protected:
    binding_generator_base(engine_pointer engine) noexcept(std::is_nothrow_default_constructible_v<Listener>)
        : engine_ref_holder(engine), m_listener() {}

    binding_generator_base(engine_pointer engine, const Listener& listener)
        : engine_ref_holder(engine), m_listener(listener) {}

    binding_generator_base(engine_pointer engine, Listener&& listener)
        : engine_ref_holder(engine), m_listener(std::move(listener)) {}

public:
    [[nodiscard]]
    Listener& get_listener() noexcept
    {
        return m_listener;
    }

    [[nodiscard]]
    const Listener& get_listener() const noexcept
    {
        return m_listener;
    }

private:
    Listener m_listener;
};

template <bool ForceGeneric, typename Listener = default_listener>
class binding_generator_interface : public binding_generator_base<Listener>
{
    using my_base = binding_generator_base<Listener>;

public:
    binding_generator_interface(
        const binding_generator_interface&
    ) noexcept(std::is_nothrow_copy_constructible_v<my_base>) = default;

protected:
    using my_base::my_base;

public:
    [[nodiscard]]
    static constexpr bool force_generic() noexcept
    {
        return ForceGeneric;
    }
};

template <typename T>
concept binding_generator = std::derived_from<T, engine_ref_holder>;
} // namespace asbind20

#endif
