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

namespace asbind20
{
struct use_generic_t
{};

constexpr inline use_generic_t use_generic{};

struct use_explicit_t
{};

constexpr inline use_explicit_t use_explicit{};

namespace detail
{
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

    template <typename Class, typename FuncSig, bool TryVoidPtr = false>
    consteval auto deduce_method_callconv() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        if constexpr(std::is_member_function_pointer_v<FuncSig>)
            return AS_NAMESPACE_QUALIFIER asCALL_THISCALL;
        else if constexpr(std::convertible_to<FuncSig, AS_NAMESPACE_QUALIFIER asGENFUNC_t>)
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
                if(obj_first)
                    return traits::arg_count_v == 1 ?
                               AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST :
                               AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST;
                else
                    return AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST;
            }
            else
            {
                if constexpr(!TryVoidPtr)
                    static_assert(obj_last || obj_first, "Missing object parameter");

                constexpr bool void_obj_first = is_this_arg<first_arg_t, Class>(true);
                constexpr bool void_obj_last = is_this_arg<last_arg_t, Class>(true);

                static_assert(void_obj_last || void_obj_first, "Missing object/void* parameter");

                if(void_obj_first)
                    return traits::arg_count_v == 1 ?
                               AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST :
                               AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST;
                else
                    return AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST;
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

        if(obj_first)
            return traits::arg_count_v == 1 ?
                       AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST :
                       AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST;
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
        if constexpr(
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

                if(obj_first)
                    return traits::arg_count_v == 1 ?
                               AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST :
                               AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST;
                else
                    return AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST;
            }
        }

        // unreachable
    }

    template <typename Class, noncapturing_lambda Lambda>
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
        // Firstly, find the begin of parameters
        auto param_being = std::find(funcdef.rbegin(), funcdef.rend(), '(');
        if(param_being != funcdef.rend())
        {
            ++param_being; // Skip '('

            // Skip possible whitespaces between the function name and the parameters
            param_being = std::find_if_not(
                param_being,
                funcdef.rend(),
                [](char ch) -> bool
                { return ch == ' '; }
            );
        }
        auto name_begin = std::find_if_not(
            param_being,
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
            "::"sv,
            std::string_view(name_begin.base(), funcdef.end())
        );
    }

    template <typename T, typename RegisterHelper>
    concept auto_register = requires(T&& ar, RegisterHelper& c) {
        ar(c);
    };
} // namespace detail

template <bool ForceGeneric>
class register_helper_base
{
public:
    register_helper_base() = delete;
    register_helper_base(const register_helper_base&) noexcept = default;

    register_helper_base(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
        : m_engine(engine)
    {
        assert(engine != nullptr);
    }

    [[nodiscard]]
    auto get_engine() const noexcept
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine;
    }

    static constexpr bool force_generic() noexcept
    {
        return ForceGeneric;
    }

protected:
    AS_NAMESPACE_QUALIFIER asIScriptEngine* const m_engine;
};
} // namespace asbind20

#endif
