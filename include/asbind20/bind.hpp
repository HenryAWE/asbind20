#ifndef ASBIND20_BIND_HPP
#define ASBIND20_BIND_HPP

#pragma once

#include <cassert>
#include <concepts>
#include <type_traits>
#include <bit>
#include <string>
#include <tuple>
#include <algorithm>
#include <functional>
#include <span>
#include "detail/include_as.hpp"
#include "utility.hpp"
#include "generic.hpp"

namespace asbind20
{
template <typename T>
class auxiliary_wrapper
{
public:
    auxiliary_wrapper() = delete;
    constexpr auxiliary_wrapper(const auxiliary_wrapper&) noexcept = default;

    explicit constexpr auxiliary_wrapper(T* aux) noexcept
        : m_aux(aux) {}

    [[nodiscard]]
    void* get_address() const noexcept
    {
        return (void*)m_aux;
    }

private:
    T* m_aux;
};

template <>
class auxiliary_wrapper<this_type_t>
{
public:
    auxiliary_wrapper() = delete;
    constexpr auxiliary_wrapper(const auxiliary_wrapper&) noexcept = default;

    explicit constexpr auxiliary_wrapper(this_type_t) noexcept {};
};

template <typename T>
[[nodiscard]]
constexpr auxiliary_wrapper<T> auxiliary(T& aux) noexcept
{
    return auxiliary_wrapper<T>(std::addressof(aux));
}

template <typename T>
[[nodiscard]]
constexpr auxiliary_wrapper<T> auxiliary(T* aux) noexcept
{
    return auxiliary_wrapper<T>(aux);
}

[[nodiscard]]
constexpr inline auxiliary_wrapper<void> auxiliary(std::nullptr_t) noexcept
{
    return auxiliary_wrapper<void>(nullptr);
}

[[nodiscard]]
constexpr inline auxiliary_wrapper<this_type_t> auxiliary(this_type_t) noexcept
{
    return auxiliary_wrapper<this_type_t>(this_type);
}

template <typename T>
constexpr auxiliary_wrapper<T> auxiliary(T&& aux) = delete;

/**
 * @brief Store a pointer-sized integer value as auxiliary object
 *
 * @note DO NOT use this unless you know what you are doing!
 *
 * @warning Only use this with the @b generic calling convention!
 *
 * @param val Integer value
 */
[[nodiscard]]
constexpr auxiliary_wrapper<void> aux_value(std::intptr_t val) noexcept
{
    return auxiliary_wrapper<void>(std::bit_cast<void*>(val));
}

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

struct use_generic_t
{};

constexpr inline use_generic_t use_generic{};

struct use_explicit_t
{};

constexpr inline use_explicit_t use_explicit{};

/**
 * @brief Policies for some special functions/behaviours
 */
namespace policies
{
    /**
     * @brief Apply each elements of the initialization list to constructor, similar to `std::apply`.
     *
     * @note Unlike other policies, this can only be used with list pattern with known type and limited size, e.g. `{int, int}`.
     *       DO NOT use this with patterns like `{ repeat_same int }`!
     */
    template <std::size_t Size>
    requires(Size >= 1)
    struct apply_to
    {
        using initialization_list_policy_tag = void;

        static constexpr std::size_t size() noexcept
        {
            return Size;
        }

        template <has_static_name ListElementType>
        requires(!std::is_void_v<ListElementType>)
        static std::string pattern()
        {
            auto type_name = name_of<ListElementType>();

            std::string result;
            result.reserve(2 + type_name.size() * Size + (Size - 1));
            result += '{';

            for(std::size_t i = 0; i < Size; ++i)
            {
                if(i != 0)
                    result += ',';
                result.append(type_name);
            }

            result += '}';

            return result;
        }
    };

    /**
     * @brief Convert script list to a proxy class
     *
     * @sa script_init_list_repeat
     */
    struct repeat_list_proxy
    {
        using initialization_list_policy_tag = void;
    };

    /**
     * @brief Convert the initialization list to an iterator pair of `[begin, end)`.
     */
    struct as_iterators
    {
        using initialization_list_policy_tag = void;

        template <typename T, typename Fn>
        static decltype(auto) apply(Fn&& fn, script_init_list_repeat list)
        {
            T* start = (T*)list.data();
            T* stop = start + list.size();

            return std::invoke(std::forward<Fn>(fn), start, stop);
        }
    };

    /**
     * @brief Convert the initialization list to a pointer and an asUINT indicating its size.
     */
    struct pointer_and_size
    {
        using initialization_list_policy_tag = void;
    };

    /**
     * @brief Convert the initialization list to an initializer list of C++
     *
     * @warning C++ doesn't provide a @b standard way to construct an initializer list from user.
     *          You should try other policies at first.
     */
    struct as_initializer_list
    {
        using initialization_list_policy_tag = void;

#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
        // Both libstdc++ and libc++ implement the initializer list as if a pair of {T*, size_t}.

#    define ASBIND20_HAS_AS_INITIALIZER_LIST "{T*, size_t}"

        template <typename T>
        static std::initializer_list<T> convert(script_init_list_repeat list) noexcept
        {
            struct tmp_type
            {
                const T* ptr;
                std::size_t size;
            };

            tmp_type tmp((const T*)list.data(), list.size());

            return std::bit_cast<std::initializer_list<T>>(tmp);
        }

#elif defined(_CPPLIB_VER) // MSVC STL
        // MSVC STL provides an extensional interface for constructing initializer list from user
        // See: https://github.com/microsoft/STL/blob/main/stl/inc/initializer_list

#    define ASBIND20_HAS_AS_INITIALIZER_LIST "MSVC STL"

        template <typename T>
        static std::initializer_list<T> convert(script_init_list_repeat list) noexcept
        {
            const T* start = (const T*)list.data();
            const T* sentinel = start + list.size();
            return std::initializer_list<T>(start, sentinel);
        }

#else // Unknown standard library. Not supported

        template <typename T>
        static std::initializer_list<T> convert(script_init_list_repeat list) = delete;

#endif
    };

    /**
     * @brief Convert the initialization list to a `span`.
     */
    struct as_span
    {
        using initialization_list_policy_tag = void;

        template <typename T>
        static std::span<T> convert(script_init_list_repeat list)
        {
            return std::span<T>((T*)list.data(), list.size());
        }
    };

    // TODO: Support `std::from_range` if C++23 is available (`__cpp_lib_containers_ranges`)

    template <typename T>
    concept initialization_list_policy =
        std::is_void_v<T> || // Default policy: directly pass the initialization list from AS to C++
        requires() {
            typename T::initialization_list_policy_tag;
        };

    /**
     * @brief Notify GC for newly created reference class
     */
    struct notify_gc
    {
        using factory_policy_tag = void;
    };

    template <typename T>
    concept factory_policy =
        std::is_void_v<T> || // Default policy: no-op
        requires() {
            typename T::factory_policy_tag;
        };
} // namespace policies

/**
 * @brief Wrapper generators for special functions like constructor
 *
 * @note DO NOT directly use anything in this namespace unless you have really special requirement!
 *       The interfaces in this namespace are not guaranteed to keep compatibility between versions.
 */
namespace wrappers
{
    template <typename Class, bool Template, typename... Args>
    class constructor
    {
    public:
        static constexpr bool is_acceptable_native_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST;
        }

        static constexpr bool is_acceptable_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC ||
                   is_acceptable_native_call_conv(conv);
        }

        using native_function_type = std::conditional_t<
            Template,
            void (*)(AS_NAMESPACE_QUALIFIER asITypeInfo*, Args..., void*),
            void (*)(Args..., void*)>;

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        using wrapper_type = std::conditional_t<
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC,
            AS_NAMESPACE_QUALIFIER asGENFUNC_t,
            native_function_type>;

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept -> wrapper_type<CallConv>
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                using args_tuple = std::tuple<Args...>;

                if constexpr(Template)
                {
                    return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            void* mem = gen->GetObject();
                            new(mem) Class(
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1
                                )...
                            );
                        }(std::index_sequence_for<Args...>());
                    };
                }
                else
                {
                    return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            void* mem = gen->GetObject();
                            new(mem) Class(
                                get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is
                                )...
                            );
                        }(std::index_sequence_for<Args...>());
                    };
                }
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                if constexpr(Template)
                {
                    return +[](AS_NAMESPACE_QUALIFIER asITypeInfo* ti, Args... args, void* mem) -> void
                    {
                        new(mem) Class(ti, std::forward<Args>(args)...);
                    };
                }
                else
                {
                    return +[](Args... args, void* mem) -> void
                    {
                        new(mem) Class(std::forward<Args>(args)...);
                    };
                }
            }
        }
    };

    template <
        native_function auto ConstructorFunc,
        typename Class,
        bool Template,
        AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
    requires(
        OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    class constructor_function
    {
    public:
        using native_function_type = std::decay_t<decltype(ConstructorFunc)>;

        static auto generate(call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>) noexcept
            -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
        {
            using traits = function_traits<decltype(ConstructorFunc)>;
            using args_tuple = typename traits::args_tuple;

            if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    if constexpr(Template)
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc,
                                mem,
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is + 2, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1
                                )...
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 2>());
                    }
                    else
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc,
                                mem,
                                get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is
                                )...
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 1>());
                    }
                };
            }
            else // OriginalCallConv == asCALL_CDECL_OBJLAST
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    if constexpr(Template)
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc,
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1
                                )...,
                                mem
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 2>());
                    }
                    else
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc,
                                get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is
                                )...,
                                mem
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 1>());
                    }
                };
            }
        }
    };

    template <
        noncapturing_lambda ConstructorFunc,
        typename Class,
        bool Template,
        AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
    requires(
        OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    class constructor_lambda
    {
    public:
        using native_function_type = std::decay_t<decltype(+ConstructorFunc{})>;

        static auto generate(call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>) noexcept
            -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
        {
            using traits = function_traits<native_function_type>;
            using args_tuple = typename traits::args_tuple;

            if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    if constexpr(Template)
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc{},
                                mem,
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is + 2, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1
                                )...
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 2>());
                    }
                    else
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc{},
                                mem,
                                get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is
                                )...
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 1>());
                    }
                };
            }
            else // OriginalCallConv == asCALL_CDECL_OBJLAST
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    if constexpr(Template)
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc{},
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1
                                )...,
                                mem
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 2>());
                    }
                    else
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc{},
                                get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is
                                )...,
                                mem
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 1>());
                    }
                };
            }
        }
    };

    template <typename Class, bool Template, typename ListBufType>
    class list_constructor_base
    {
    public:
        static constexpr bool is_acceptable_native_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST;
        }

        static constexpr bool is_acceptable_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC ||
                   is_acceptable_native_call_conv(conv);
        }

        using native_function_type =
            std::conditional_t<
                Template,
                void (*)(AS_NAMESPACE_QUALIFIER asITypeInfo*, ListBufType, void*),
                void (*)(ListBufType, void*)>;

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        using wrapper_type = std::conditional_t<
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC,
            AS_NAMESPACE_QUALIFIER asGENFUNC_t,
            native_function_type>;
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType = void,
        policies::initialization_list_policy Policy = void>
    class list_constructor : public list_constructor_base<Class, Template, ListElementType*>
    {
        using my_base = list_constructor_base<Class, Template, ListElementType*>;

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
            -> my_base::template wrapper_type<CallConv>
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                if constexpr(Template)
                {
                    return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                    {
                        void* mem = gen->GetObject();
                        new(mem) Class(
                            *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                            *(ListElementType**)gen->GetAddressOfArg(1)
                        );
                    };
                }
                else
                {
                    return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                    {
                        void* mem = gen->GetObject();
                        new(mem) Class(*(ListElementType**)gen->GetAddressOfArg(0));
                    };
                }
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                if constexpr(Template)
                {
                    return +[](AS_NAMESPACE_QUALIFIER asITypeInfo* ti, ListElementType* list_buf, void* mem) -> void
                    {
                        new(mem) Class(ti, list_buf);
                    };
                }
                else
                {
                    return +[](ListElementType* list_buf, void* mem) -> void
                    {
                        new(mem) Class(list_buf);
                    };
                }
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType>
    class list_constructor<Class, Template, ListElementType, policies::repeat_list_proxy> :
        public list_constructor_base<Class, Template, void*>
    {
        using my_base = list_constructor_base<Class, Template, void*>;

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
            -> my_base::template wrapper_type<CallConv>
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                if constexpr(Template)
                {
                    return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                    {
                        void* mem = gen->GetObject();
                        new(mem) Class(
                            *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                            script_init_list_repeat(gen, 1)
                        );
                    };
                }
                else
                {
                    return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                    {
                        void* mem = gen->GetObject();
                        new(mem) Class(script_init_list_repeat(gen));
                    };
                }
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                if constexpr(Template)
                {
                    return +[](AS_NAMESPACE_QUALIFIER asITypeInfo* ti, void* list_buf, void* mem) -> void
                    {
                        new(mem) Class(ti, script_init_list_repeat(list_buf));
                    };
                }
                else
                {
                    return +[](void* list_buf, void* mem) -> void
                    {
                        new(mem) Class(script_init_list_repeat(list_buf));
                    };
                }
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType,
        std::size_t Size>
    class list_constructor<Class, Template, ListElementType, policies::apply_to<Size>> :
        public list_constructor_base<Class, Template, ListElementType*>
    {
        using my_base = list_constructor_base<Class, Template, ListElementType*>;

    public:
        static_assert(!std::is_void_v<ListElementType>, "Invalid list element type");
        static_assert(!Template, "This policy is invalid for a template class");

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
            -> my_base::template wrapper_type<CallConv>
        {
            static constexpr auto helper = [](void* mem, ListElementType* list_buf)
            {
                [&]<std::size_t... Is>(std::index_sequence<Is...>)
                {
                    new(mem) Class(list_buf[Is]...);
                }(std::make_index_sequence<Size>());
            };

            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    helper(
                        gen->GetObject(),
                        *(ListElementType**)gen->GetAddressOfArg(0)
                    );
                };
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                return +[](ListElementType* list_buf, void* mem) -> void
                {
                    helper(mem, list_buf);
                };
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType>
    class list_constructor<Class, Template, ListElementType, policies::as_iterators> :
        public list_constructor_base<Class, Template, void*>
    {
        using my_base = list_constructor_base<Class, Template, void*>;

    public:
        static_assert(!std::is_void_v<ListElementType>, "Invalid list element type");
        static_assert(!Template, "This policy is invalid for a template class");

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
            -> my_base::template wrapper_type<CallConv>
        {
            static constexpr auto helper = [](void* mem, script_init_list_repeat list)
            {
                policies::as_iterators::apply<ListElementType>(
                    [mem](auto start, auto stop)
                    {
                        new(mem) Class(start, stop);
                    },
                    list
                );
            };

            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    helper(
                        gen->GetObject(),
                        script_init_list_repeat(gen)
                    );
                };
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                return +[](void* list_buf, void* mem) -> void
                {
                    helper(mem, script_init_list_repeat(list_buf));
                };
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType>
    class list_constructor<Class, Template, ListElementType, policies::pointer_and_size> :
        public list_constructor_base<Class, Template, void*>
    {
        using my_base = list_constructor_base<Class, Template, void*>;

    public:
        static_assert(!std::is_void_v<ListElementType>, "Invalid list element type");
        static_assert(!Template, "This policy is invalid for a template class");

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
            -> my_base::template wrapper_type<CallConv>
        {
            static constexpr auto helper = [](void* mem, script_init_list_repeat list)
            {
                new(mem) Class((ListElementType*)list.data(), list.size());
            };

            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    helper(
                        gen->GetObject(),
                        script_init_list_repeat(gen)
                    );
                };
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                return +[](void* list_buf, void* mem) -> void
                {
                    helper(mem, script_init_list_repeat(list_buf));
                };
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType,
        policies::initialization_list_policy ConvertibleRangePolicy>
    requires(
        std::same_as<ConvertibleRangePolicy, policies::as_initializer_list> ||
        std::same_as<ConvertibleRangePolicy, policies::as_span>
    )
    class list_constructor<Class, Template, ListElementType, ConvertibleRangePolicy> :
        public list_constructor_base<Class, Template, void*>
    {
        using my_base = list_constructor_base<Class, Template, void*>;

    public:
        static_assert(!std::is_void_v<ListElementType>, "Invalid list element type");
        static_assert(!Template, "This policy is invalid for a template class");

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
            -> my_base::template wrapper_type<CallConv>
        {
            static constexpr auto helper = [](void* mem, script_init_list_repeat list)
            {
                new(mem) Class(ConvertibleRangePolicy::template convert<ListElementType>(list));
            };

            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    helper(
                        gen->GetObject(),
                        script_init_list_repeat(gen)
                    );
                };
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                return +[](void* list_buf, void* mem) -> void
                {
                    helper(mem, script_init_list_repeat(list_buf));
                };
            }
        }
    };

    template <
        typename Class,
        bool Template,
        policies::factory_policy Policy,
        typename... Args>
    class factory
    {
        static_assert(std::is_void_v<Policy>);

    public:
        static constexpr bool is_acceptable_native_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL;
        }

        static constexpr bool is_acceptable_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC ||
                   is_acceptable_native_call_conv(conv);
        }

        using native_function_type = std::conditional_t<
            Template,
            Class* (*)(AS_NAMESPACE_QUALIFIER asITypeInfo*, Args...),
            Class* (*)(Args...)>;

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        using wrapper_type = std::conditional_t<
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC,
            AS_NAMESPACE_QUALIFIER asGENFUNC_t,
            native_function_type>;

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept -> wrapper_type<CallConv>
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                if constexpr(Template)
                {
                    return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                    {
                        using args_tuple = std::tuple<Args...>;

                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* ptr = new Class(
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is + 1)...
                            );
                            gen->SetReturnAddress(ptr);
                        }(std::index_sequence_for<Args...>());
                    };
                }
                else
                {
                    return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                    {
                        using args_tuple = std::tuple<Args...>;

                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* ptr = new Class(get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is)...);
                            gen->SetReturnAddress(ptr);
                        }(std::index_sequence_for<Args...>());
                    };
                }
            }
            else // CallConv == asCALL_CDECL
            {
                if constexpr(Template)
                {
                    return +[](AS_NAMESPACE_QUALIFIER asITypeInfo* ti, Args... args) -> Class*
                    {
                        return new Class(ti, std::forward<Args>(args)...);
                    };
                }
                else
                {
                    return +[](Args... args) -> Class*
                    {
                        return new Class(std::forward<Args>(args)...);
                    };
                }
            }
        }
    };

    template <typename Class, typename... Args>
    class factory<Class, false, policies::notify_gc, Args...>
    {
    public:
        static constexpr bool is_acceptable_native_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST;
        }

        static constexpr bool is_acceptable_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC ||
                   is_acceptable_native_call_conv(conv);
        }

        using native_function_type =
            Class* (*)(Args..., AS_NAMESPACE_QUALIFIER asITypeInfo*);

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        using wrapper_type = std::conditional_t<
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC,
            AS_NAMESPACE_QUALIFIER asGENFUNC_t,
            native_function_type>;

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        static auto generate(call_conv_t<CallConv>) noexcept
            -> wrapper_type<CallConv>
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
                {
                    using args_tuple = std::tuple<Args...>;

                    [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                    {
                        auto* ti = (AS_NAMESPACE_QUALIFIER asITypeInfo*)gen->GetAuxiliary();
                        Class* ptr = new Class(
                            get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                                gen, (AS_NAMESPACE_QUALIFIER asUINT)Is
                            )...
                        );
                        assert(ti->GetEngine() == gen->GetEngine());
                        gen->GetEngine()->NotifyGarbageCollectorOfNewObject(ptr, ti);

                        gen->SetReturnAddress(ptr);
                    }(std::index_sequence_for<Args...>());
                };
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                return +[](Args... args, AS_NAMESPACE_QUALIFIER asITypeInfo* ti) -> Class*
                {
                    Class* ptr = new Class(std::forward<Args>(args)...);
                    ti->GetEngine()->NotifyGarbageCollectorOfNewObject(ptr, ti);

                    return ptr;
                };
            }
        }
    };

    template <typename Class, typename... Args>
    class factory<Class, true, policies::notify_gc, Args...>
    {
    public:
        static constexpr bool is_acceptable_native_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL;
        }

        static constexpr bool is_acceptable_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC ||
                   is_acceptable_native_call_conv(conv);
        }

        using native_function_type =
            Class* (*)(AS_NAMESPACE_QUALIFIER asITypeInfo*, Args...);

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        using wrapper_type = std::conditional_t<
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC,
            AS_NAMESPACE_QUALIFIER asGENFUNC_t,
            native_function_type>;

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        static auto generate(call_conv_t<CallConv>) noexcept
            -> wrapper_type<CallConv>
        {
            // Note: Template callback may remove the asOBJ_GC flag for some instantiations,
            // so this helper will check the flag at runtime.

            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
                {
                    using args_tuple = std::tuple<Args...>;

                    [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                    {
                        auto* ti = *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0);
                        Class* ptr = new Class(
                            ti,
                            get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                                gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1
                            )...
                        );

                        auto flags = ti->GetFlags();
                        if(flags & AS_NAMESPACE_QUALIFIER asOBJ_GC)
                        {
                            assert(ti->GetEngine() == gen->GetEngine());
                            gen->GetEngine()->NotifyGarbageCollectorOfNewObject(ptr, ti);
                        }

                        gen->SetReturnAddress(ptr);
                    }(std::index_sequence_for<Args...>());
                };
            }
            else // CallConv == asCALL_CDECL
            {
                return +[](AS_NAMESPACE_QUALIFIER asITypeInfo* ti, Args... args) -> Class*
                {
                    Class* ptr = new Class(ti, std::forward<Args>(args)...);

                    auto flags = ti->GetFlags();
                    if(flags & AS_NAMESPACE_QUALIFIER asOBJ_GC)
                    {
                        ti->GetEngine()->NotifyGarbageCollectorOfNewObject(ptr, ti);
                    }

                    return ptr;
                };
            }
        }
    };

    template <
        native_function auto FactoryFunc,
        bool Template,
        AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
    requires(
        OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL ||
        OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    class factory_function_auxiliary
    {
    public:
        static auto generate(call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>) noexcept
            -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
        {
            using traits = function_traits<decltype(FactoryFunc)>;
            using args_tuple = typename traits::args_tuple;
            using auxiliary_type = std::conditional_t<
                OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST,
                typename traits::first_arg_type,
                typename traits::last_arg_type>;

            if constexpr(Template)
            {
                constexpr std::size_t real_arg_count =
                    OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL ?
                        traits::arg_count_v - 1 :
                        traits::arg_count_v - 2;

                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    void* ptr = nullptr;

                    [&]<std::size_t... Is>(std::index_sequence<Is...>)
                    {
                        if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL)
                        {
                            ptr = std::invoke(
                                FactoryFunc,
                                get_generic_auxiliary<typename traits::class_type>(gen),
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(gen, (asUINT)Is + 1)...
                            );
                        }
                        else if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
                        {
                            ptr = std::invoke(
                                FactoryFunc,
                                get_generic_auxiliary<auxiliary_type>(gen),
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is + 2, args_tuple>>(gen, (asUINT)Is + 1)...
                            );
                        }
                        else // OriginalCallConv == asCALL_CDECL_OBJLAST
                        {
                            ptr = std::invoke(
                                FactoryFunc,
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(gen, (asUINT)Is + 1)...,
                                get_generic_auxiliary<auxiliary_type>(gen)
                            );
                        }
                    }(std::make_index_sequence<real_arg_count>());

                    gen->SetReturnAddress(ptr);
                };
            }
            else
            {
                constexpr std::size_t real_arg_count =
                    OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL ?
                        traits::arg_count_v :
                        traits::arg_count_v - 1;

                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    void* ptr = nullptr;

                    [&]<std::size_t... Is>(std::index_sequence<Is...>)
                    {
                        if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL)
                        {
                            ptr = std::invoke(
                                FactoryFunc,
                                get_generic_auxiliary<typename traits::class_type>(gen),
                                get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is)...
                            );
                        }
                        else if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
                        {
                            ptr = std::invoke(
                                FactoryFunc,
                                get_generic_auxiliary<auxiliary_type>(gen),
                                get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(gen, (asUINT)Is)...
                            );
                        }
                        else // OriginalCallConv == asCALL_CDECL_OBJLAST
                        {
                            ptr = std::invoke(
                                FactoryFunc,
                                get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (asUINT)Is)...,
                                get_generic_auxiliary<auxiliary_type>(gen)
                            );
                        }
                    }(std::make_index_sequence<real_arg_count>());

                    gen->SetReturnAddress(ptr);
                };
            }
        }
    };

    template <typename Class, bool Template, typename ListBufType, policies::factory_policy FactoryPolicy>
    class list_factory_base
    {
    public:
        static constexpr bool is_acceptable_native_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            if constexpr(std::same_as<FactoryPolicy, policies::notify_gc> && !Template)
                return conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST;
            else
                return conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL;
        }

        static constexpr bool is_acceptable_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC ||
                   is_acceptable_native_call_conv(conv);
        }

        using native_function_type = std::conditional_t<
            Template,
            Class* (*)(AS_NAMESPACE_QUALIFIER asITypeInfo*, ListBufType),
            std::conditional_t<
                std::same_as<FactoryPolicy, policies::notify_gc>,
                Class* (*)(ListBufType, AS_NAMESPACE_QUALIFIER asITypeInfo*),
                Class* (*)(ListBufType)>>;


        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        using wrapper_type = std::conditional_t<
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC,
            AS_NAMESPACE_QUALIFIER asGENFUNC_t,
            native_function_type>;

        static void notify_gc_helper(void* obj, AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                if constexpr(Template)
                {
                    auto flags = ti->GetFlags();
                    if(!(flags & AS_NAMESPACE_QUALIFIER asOBJ_GC))
                        return;
                }
                ti->GetEngine()->NotifyGarbageCollectorOfNewObject(obj, ti);
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType = void,
        policies::initialization_list_policy Policy = void,
        policies::factory_policy FactoryPolicy = void>
    class list_factory : public list_factory_base<Class, Template, ListElementType*, FactoryPolicy>
    {
        using my_base = list_factory_base<Class, Template, ListElementType*, FactoryPolicy>;

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
            -> my_base::template wrapper_type<CallConv>
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                if constexpr(Template)
                {
                    return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                    {
                        Class* ptr = new Class(
                            *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                            *(ListElementType**)gen->GetAddressOfArg(1)
                        );
                        gen->SetReturnAddress(ptr);
                    };
                }
                else
                {
                    return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                    {
                        Class* ptr = new Class(
                            *(ListElementType**)gen->GetAddressOfArg(0)
                        );
                        gen->SetReturnAddress(ptr);
                    };
                }
            }
            else // CallConv == asCALL_CDECL
            {
                if constexpr(Template)
                {
                    return +[](AS_NAMESPACE_QUALIFIER asITypeInfo* ti, ListElementType* list_buf) -> Class*
                    {
                        return new Class(ti, list_buf);
                    };
                }
                else
                {
                    return +[](ListElementType* list_buf) -> Class*
                    {
                        return new Class(list_buf);
                    };
                }
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType,
        std::size_t Size,
        policies::factory_policy FactoryPolicy>
    class list_factory<Class, Template, ListElementType, policies::apply_to<Size>, FactoryPolicy> :
        public list_factory_base<Class, Template, ListElementType*, FactoryPolicy>
    {
        using my_base = list_factory_base<Class, Template, ListElementType*, FactoryPolicy>;

    public:
        static_assert(!std::is_void_v<ListElementType>, "Invalid list element type");
        static_assert(!Template, "This policy is invalid for a template class");

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
            -> my_base::template wrapper_type<CallConv>
        {
            static constexpr auto helper = [](ListElementType* list_buf) -> Class*
            {
                return [&]<std::size_t... Is>(std::index_sequence<Is...>)
                {
                    return new Class(list_buf[Is]...);
                }(std::make_index_sequence<Size>());
            };

            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    Class* ptr = helper(*(ListElementType**)gen->GetAddressOfArg(0));
                    if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
                    {
                        auto* ti = (AS_NAMESPACE_QUALIFIER asITypeInfo*)gen->GetAuxiliary();
                        assert(ti != nullptr);
                        my_base::notify_gc_helper(ptr, ti);
                    }
                    gen->SetReturnAddress(ptr);
                };
            }
            else if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                return +[](ListElementType* list_buf, AS_NAMESPACE_QUALIFIER asITypeInfo* ti) -> Class*
                {
                    Class* ptr = helper(list_buf);
                    my_base::notify_gc_helper(ptr, ti);
                    return ptr;
                };
            }
            else // CallConv == asCALL_CDECL
            {
                return +[](ListElementType* list_buf) -> Class*
                {
                    return helper(list_buf);
                };
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType,
        policies::factory_policy FactoryPolicy>
    class list_factory<Class, Template, ListElementType, policies::repeat_list_proxy, FactoryPolicy> :
        public list_factory_base<Class, Template, void*, FactoryPolicy>
    {
        using my_base = list_factory_base<Class, Template, void*, FactoryPolicy>;

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
            -> my_base::template wrapper_type<CallConv>
        {
            if constexpr(Template)
            {
                if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                {
                    return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                    {
                        auto* ti = *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0);
                        Class* ptr = new Class(
                            ti,
                            script_init_list_repeat(gen, 1)
                        );
                        my_base::notify_gc_helper(ptr, ti);
                        gen->SetReturnAddress(ptr);
                    };
                }
                else // CallConv == asCALL_CDECL
                {
                    return +[](AS_NAMESPACE_QUALIFIER asITypeInfo* ti, void* list_buf) -> Class*
                    {
                        Class* ptr = new Class(ti, script_init_list_repeat(list_buf));
                        my_base::notify_gc_helper(ptr, ti);
                        return ptr;
                    };
                }
            }
            else
            {
                if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                {
                    return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                    {
                        if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
                        {
                            Class* ptr = new Class(script_init_list_repeat(gen));
                            auto* ti = (AS_NAMESPACE_QUALIFIER asITypeInfo*)gen->GetAuxiliary();
                            my_base::notify_gc_helper(ptr, ti);
                            gen->SetReturnAddress(ptr);
                        }
                        else
                        {
                            gen->SetReturnAddress(
                                new Class(script_init_list_repeat(gen))
                            );
                        }
                    };
                }
                else if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
                {
                    return +[](void* list_buf, AS_NAMESPACE_QUALIFIER asITypeInfo* ti) -> Class*
                    {
                        Class* ptr = new Class(script_init_list_repeat(list_buf));
                        my_base::notify_gc_helper(ptr, ti);
                        return ptr;
                    };
                }
                else // CallConv == asCALL_CDECL
                {
                    return +[](void* list_buf) -> Class*
                    {
                        return new Class(script_init_list_repeat(list_buf));
                    };
                }
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType,
        policies::factory_policy FactoryPolicy>
    class list_factory<Class, Template, ListElementType, policies::as_iterators, FactoryPolicy> :
        public list_factory_base<Class, Template, void*, FactoryPolicy>
    {
        using my_base = list_factory_base<Class, Template, void*, FactoryPolicy>;

    public:
        static_assert(!std::is_void_v<ListElementType>, "Invalid list element type");
        static_assert(!Template, "This policy is invalid for a template class");

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
            -> my_base::template wrapper_type<CallConv>
        {
            static constexpr auto helper = [](script_init_list_repeat list) -> Class*
            {
                return policies::as_iterators::apply<ListElementType>(
                    [](auto start, auto stop) -> Class*
                    {
                        return new Class(start, stop);
                    },
                    list
                );
            };

            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    Class* ptr = helper(script_init_list_repeat(gen));
                    if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
                    {
                        auto* ti = (AS_NAMESPACE_QUALIFIER asITypeInfo*)gen->GetAuxiliary();
                        my_base::notify_gc_helper(ptr, ti);
                    }
                    gen->SetReturnAddress(ptr);
                };
            }
            else if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                return +[](void* list_buf, AS_NAMESPACE_QUALIFIER asITypeInfo* ti) -> Class*
                {
                    Class* ptr = helper(script_init_list_repeat(list_buf));
                    my_base::notify_gc_helper(ptr, ti);
                    return ptr;
                };
            }
            else // CallConv == asCALL_CDECL
            {
                return +[](void* list_buf) -> Class*
                {
                    return helper(script_init_list_repeat(list_buf));
                };
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType,
        policies::factory_policy FactoryPolicy>
    class list_factory<Class, Template, ListElementType, policies::pointer_and_size, FactoryPolicy> :
        public list_factory_base<Class, Template, void*, FactoryPolicy>
    {
        using my_base = list_factory_base<Class, Template, void*, FactoryPolicy>;

    public:
        static_assert(!std::is_void_v<ListElementType>, "Invalid list element type");
        static_assert(!Template, "This policy is invalid for a template class");

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
            -> my_base::template wrapper_type<CallConv>
        {
            static constexpr auto helper = [](script_init_list_repeat list) -> Class*
            {
                return new Class((ListElementType*)list.data(), list.size());
            };

            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    Class* ptr = helper(script_init_list_repeat(gen));
                    if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
                    {
                        auto* ti = (AS_NAMESPACE_QUALIFIER asITypeInfo*)gen->GetAuxiliary();
                        my_base::notify_gc_helper(ptr, ti);
                    }
                    gen->SetReturnAddress(ptr);
                };
            }
            else if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                return +[](void* list_buf, AS_NAMESPACE_QUALIFIER asITypeInfo* ti) -> Class*
                {
                    Class* ptr = helper(script_init_list_repeat(list_buf));
                    my_base::notify_gc_helper(ptr, ti);
                    return ptr;
                };
            }
            else // CallConv == asCALL_CDECL
            {
                return +[](void* list_buf) -> Class*
                {
                    return helper(script_init_list_repeat(list_buf));
                };
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType,
        policies::initialization_list_policy ConvertibleRangePolicy,
        policies::factory_policy FactoryPolicy>
    requires(
        std::same_as<ConvertibleRangePolicy, policies::as_initializer_list> ||
        std::same_as<ConvertibleRangePolicy, policies::as_span>
    )
    class list_factory<Class, Template, ListElementType, ConvertibleRangePolicy, FactoryPolicy> :
        public list_factory_base<Class, Template, void*, FactoryPolicy>
    {
        using my_base = list_factory_base<Class, Template, void*, FactoryPolicy>;

    public:
        static_assert(!std::is_void_v<ListElementType>, "Invalid list element type");
        static_assert(!Template, "This policy is invalid for a template class");

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
            -> my_base::template wrapper_type<CallConv>
        {
            static constexpr auto helper = [](script_init_list_repeat list) -> Class*
            {
                return new Class(ConvertibleRangePolicy::template convert<ListElementType>(list));
            };

            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    Class* ptr = helper(script_init_list_repeat(gen));
                    if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
                    {
                        auto* ti = (AS_NAMESPACE_QUALIFIER asITypeInfo*)gen->GetAuxiliary();
                        my_base::notify_gc_helper(ptr, ti);
                    }
                    gen->SetReturnAddress(ptr);
                };
            }
            else if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                return +[](void* list_buf, AS_NAMESPACE_QUALIFIER asITypeInfo* ti) -> Class*
                {
                    Class* ptr = helper(script_init_list_repeat(list_buf));
                    my_base::notify_gc_helper(ptr, ti);
                    return ptr;
                };
            }
            else // CallConv == asCALL_CDECL
            {
                return +[](void* list_buf) -> Class*
                {
                    return helper(script_init_list_repeat(list_buf));
                };
            }
        }
    };

    template <typename Class, typename To>
    class opConv
    {
    public:
        static constexpr bool is_acceptable_native_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
                   conv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST;
        }

        static constexpr bool is_acceptable_call_conv(
            AS_NAMESPACE_QUALIFIER asECallConvTypes conv
        ) noexcept
        {
            return conv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC ||
                   is_acceptable_native_call_conv(conv);
        }

        using native_function_type = To (*)(Class&);

        template <asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        using wrapper_type = std::conditional_t<
            CallConv == asCALL_GENERIC,
            AS_NAMESPACE_QUALIFIER asGENFUNC_t,
            native_function_type>;

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        requires(is_acceptable_call_conv(CallConv))
        static auto generate(call_conv_t<CallConv>) noexcept -> wrapper_type<CallConv>
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    set_generic_return<To>(
                        gen,
                        static_cast<To>(get_generic_object<Class&>(gen))
                    );
                };
            }
            else // CallConv == asCALL_CDECL_OBJFIRST/LAST
            {
                return +[](Class& obj) -> To
                {
                    return static_cast<To>(obj);
                };
            }
        }
    };
} // namespace wrappers

template <typename FirstPolicy = void, typename... Policies>
struct use_policy_t
{
    using first_policy = FirstPolicy;
    using policies_tuple = std::tuple<FirstPolicy, Policies...>;
};

template <typename FirstPolicy = void, typename... Policies>
constexpr inline use_policy_t<FirstPolicy, Policies...> use_policy{};

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

namespace detail
{
    template <typename Auxiliary>
    struct real_aux_type
    {
        using type = Auxiliary;
    };

    template <>
    struct real_aux_type<this_type_t>
    {
        using type = AS_NAMESPACE_QUALIFIER asITypeInfo;
    };

    template <typename FuncSig>
    requires(!std::is_member_function_pointer_v<FuncSig>)
    constexpr asECallConvTypes deduce_function_callconv()
    {
        // TODO: Check stdcall

        /*
        On x64 and many platform, CDECL and STDCALL have the same effect.
        It's safe to treat all global functions as CDECL.
        See: https://www.gamedev.net/forums/topic/715839-question-about-calling-convention-when-registering-functions-on-x64-platform/

        We'll support STDCALL in future version if anyone need it.
        */
        return AS_NAMESPACE_QUALIFIER asCALL_CDECL;
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
        else if constexpr(std::convertible_to<FuncSig, asGENFUNC_t>)
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
                using aux_t = typename real_aux_type<Auxiliary>::type;

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
} // namespace detail

namespace wrappers
{
    template <typename T, typename RegisterHelper>
    concept auto_register = requires(T&& ar, RegisterHelper& c) {
        ar(c);
    };
} // namespace wrappers

template <bool ForceGeneric>
class global final : public register_helper_base<ForceGeneric>
{
    using my_base = register_helper_base<ForceGeneric>;

    using my_base::m_engine;

public:
    global() = delete;
    global(const global&) = default;

    global(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
        : my_base(engine) {}

    template <typename Auxiliary>
    void* get_auxiliary_address(auxiliary_wrapper<Auxiliary> aux) const
    {
        if constexpr(std::same_as<Auxiliary, this_type_t>)
        {
            static_assert(!sizeof(Auxiliary), "auxiliary(this_type) is invalid for a global function!");
        }
        else
        {
            return aux.get_address();
        }
    }

    template <
        native_function Fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    global& function(
        std::string_view decl,
        Fn&& fn,
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        static_assert(
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL || CallConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL,
            "Invalid calling convention for a global function"
        );

        [[maybe_unused]]
        int r = with_cstr(
            [this, &fn](const char* decl)
            {
                return m_engine->RegisterGlobalFunction(
                    decl,
                    to_asSFuncPtr(fn),
                    CallConv
                );
            },
            decl
        );
        assert(r >= 0);

        return *this;
    }

    template <native_function Fn>
    global& function(
        std::string_view decl,
        Fn&& fn
    ) requires(!ForceGeneric)
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_function_callconv<std::decay_t<Fn>>();

        this->function(decl, fn, call_conv<conv>);

        return *this;
    }

    global& function(
        std::string_view decl,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        [[maybe_unused]]
        int r = with_cstr(
            [this, &gfn](const char* decl)
            {
                return m_engine->RegisterGlobalFunction(
                    decl,
                    to_asSFuncPtr(gfn),
                    AS_NAMESPACE_QUALIFIER asCALL_GENERIC
                );
            },
            decl
        );
        assert(r >= 0);

        return *this;
    }

    template <
        auto Function,
        asECallConvTypes CallConv>
    global& function(
        use_generic_t,
        std::string_view decl,
        fp_wrapper_t<Function>,
        call_conv_t<CallConv>
    )
    {
        static_assert(
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL || CallConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL,
            "Invalid calling convention for a global function"
        );

        this->function(
            decl,
            to_asGENFUNC_t(fp<Function>, call_conv<CallConv>),
            generic_call_conv
        );

        return *this;
    }

    template <
        auto Function,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    global& function(
        std::string_view decl,
        fp_wrapper_t<Function>,
        call_conv_t<CallConv>
    )
    {
        static_assert(
            CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL || CallConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL,
            "Invalid calling convention for a global function"
        );

        if constexpr(ForceGeneric)
        {
            function(use_generic, decl, fp<Function>, call_conv<CallConv>);
        }
        else
        {
            this->function(decl, Function, call_conv<CallConv>);
        }

        return *this;
    }

    template <auto Function>
    global& function(
        use_generic_t,
        std::string_view decl,
        fp_wrapper_t<Function>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_function_callconv<std::decay_t<decltype(Function)>>();

        function(use_generic, decl, fp<Function>, call_conv<conv>);

        return *this;
    }

    template <auto Function>
    global& function(
        std::string_view decl,
        fp_wrapper_t<Function>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_function_callconv<std::decay_t<decltype(Function)>>();

        if constexpr(ForceGeneric)
        {
            function(use_generic, decl, fp<Function>, call_conv<conv>);
        }
        else
        {
            this->function(decl, Function, call_conv<conv>);
        }

        return *this;
    }

    template <noncapturing_lambda Lambda>
    global& function(
        use_generic_t,
        std::string_view decl,
        const Lambda&
    )
    {
        this->function(
            decl,
            to_asGENFUNC_t(Lambda{}, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>),
            generic_call_conv
        );

        return *this;
    }

    template <noncapturing_lambda Lambda>
    global& function(
        std::string_view decl,
        const Lambda&
    )
    {
        if constexpr(ForceGeneric)
            this->function(use_generic, decl, Lambda{});
        else
            this->function(decl, +Lambda{}, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);

        return *this;
    }

    template <typename Auxiliary>
    global& function(
        std::string_view decl,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        [[maybe_unused]]
        int r = with_cstr(
            [this, &gfn, &aux](const char* decl)
            {
                return m_engine->RegisterGlobalFunction(
                    decl,
                    to_asSFuncPtr(gfn),
                    AS_NAMESPACE_QUALIFIER asCALL_GENERIC,
                    get_auxiliary_address(aux)
                );
            },
            decl
        );
        assert(r >= 0);

        return *this;
    }

    template <typename Fn, typename Auxiliary>
    requires(std::is_member_function_pointer_v<Fn>)
    global& function(
        std::string_view decl,
        Fn&& fn,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL> = {}
    ) requires(!ForceGeneric)
    {
        [[maybe_unused]]
        int r = with_cstr(
            [this, &fn, &aux](const char* decl)
            {
                return m_engine->RegisterGlobalFunction(
                    decl,
                    to_asSFuncPtr(fn),
                    AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL,
                    get_auxiliary_address(aux)
                );
            },
            decl
        );
        assert(r >= 0);

        return *this;
    }

    template <
        auto Function,
        typename Auxiliary>
    global& function(
        use_generic_t,
        std::string_view decl,
        fp_wrapper_t<Function>,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL> = {}
    )
    {
        static_assert(
            std::is_member_function_pointer_v<std::decay_t<decltype(Function)>>,
            "Function for asCALL_THISCALL_ASGLOBAL must be a member function"
        );

        function(
            decl,
            to_asGENFUNC_t(fp<Function>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL>),
            aux,
            generic_call_conv
        );

        return *this;
    }

    template <
        auto Function,
        typename Auxiliary>
    global& function(
        std::string_view decl,
        fp_wrapper_t<Function>,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL> = {}
    )
    {
        static_assert(
            std::is_member_function_pointer_v<std::decay_t<decltype(Function)>>,
            "Function for asCALL_THISCALL_ASGLOBAL must be a member function"
        );

        if constexpr(ForceGeneric)
            function(use_generic, decl, fp<Function>, aux);
        else
            function(decl, Function, aux);

        return *this;
    }

    /**
     * @brief Register a global property
     */
    template <typename T>
    global& property(
        std::string_view decl,
        T& val
    )
    {
        [[maybe_unused]]
        int r = with_cstr(
            &AS_NAMESPACE_QUALIFIER asIScriptEngine::RegisterGlobalProperty,
            m_engine,
            decl,
            (void*)std::addressof(val)
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Register a funcdef
     *
     * @param decl Function declaration
     */
    global& funcdef(
        std::string_view decl
    )
    {
        [[maybe_unused]]
        int r = with_cstr(
            &AS_NAMESPACE_QUALIFIER asIScriptEngine::RegisterFuncdef,
            m_engine,
            decl
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Register a typedef
     *
     * @param type_decl Type declaration
     * @param new_name Aliased type name
     */
    global& typedef_(
        std::string_view type_decl,
        std::string_view new_name
    )
    {
        [[maybe_unused]]
        int r = with_cstr(
            &AS_NAMESPACE_QUALIFIER asIScriptEngine::RegisterTypedef,
            m_engine,
            new_name,
            type_decl
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Register a typedef in C++11 style
     *
     * @param new_name Aliased type name
     * @param type_decl Type declaration
     */
    global& using_(
        std::string_view new_name,
        std::string_view type_decl
    )
    {
        typedef_(type_decl, new_name);

        return *this;
    }

    /**
     * @brief Generic calling convention for message callback is not supported.
     */
    global& message_callback(
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        void* obj = nullptr
    ) = delete;

    /**
     * @brief Set the message callback.
     */
    template <native_function Callback>
    requires(!std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& message_callback(Callback fn, void* obj = nullptr)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->SetMessageCallback(
            to_asSFuncPtr(fn),
            obj,
            AS_NAMESPACE_QUALIFIER asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Set a member function as the message callback.
     */
    template <native_function Callback, typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& message_callback(Callback fn, T& obj)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->SetMessageCallback(
            to_asSFuncPtr(fn),
            (void*)std::addressof(obj),
            AS_NAMESPACE_QUALIFIER asCALL_THISCALL
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Generic calling convention for exception translator is not supported.
     */
    global& exception_translator(
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        void* obj = nullptr
    ) = delete;

    /**
     * @brief Set the exception translator.
     */
    template <native_function Callback>
    requires(!std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& exception_translator(Callback fn, void* obj = nullptr)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->SetTranslateAppExceptionCallback(
            to_asSFuncPtr(fn),
            obj,
            AS_NAMESPACE_QUALIFIER asCALL_CDECL
        );
        assert(r >= 0);

        return *this;
    }

    /**
     * @brief Set a member function as the exception translator.
     */
    template <native_function Callback, typename T>
    requires(std::is_member_function_pointer_v<std::decay_t<Callback>>)
    global& exception_translator(Callback fn, T& obj)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->SetTranslateAppExceptionCallback(
            to_asSFuncPtr(fn),
            (void*)std::addressof(obj),
            AS_NAMESPACE_QUALIFIER asCALL_THISCALL
        );
        assert(r >= 0);

        return *this;
    }
};

global(AS_NAMESPACE_QUALIFIER asIScriptEngine*) -> global<false>;

global(const script_engine&) -> global<false>;

namespace detail
{
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
} // namespace detail

template <bool ForceGeneric>
class class_register_helper_base : public register_helper_base<ForceGeneric>
{
    using my_base = register_helper_base<ForceGeneric>;

public:
    [[nodiscard]]
    int get_type_id() const noexcept
    {
        assert(m_this_type_id > 0);
        return m_this_type_id;
    }

    template <typename Auxiliary>
    void* get_auxiliary_address(auxiliary_wrapper<Auxiliary> aux) const
    {
        if constexpr(std::same_as<Auxiliary, this_type_t>)
        {
            return m_engine->GetTypeInfoById(get_type_id());
        }
        else
        {
            return aux.get_address();
        }
    }

    [[nodiscard]]
    const std::string& get_name() const noexcept
    {
        return m_name;
    }

protected:
    using my_base::m_engine;
    std::string m_name;
    int m_this_type_id = 0; // asTYPEID_VOID

    class_register_helper_base() = delete;
    class_register_helper_base(const class_register_helper_base&) = default;

    class_register_helper_base(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, std::string name)
        : my_base(engine), m_name(std::move(name)) {}

    template <typename Class>
    void register_object_type(AS_NAMESPACE_QUALIFIER asQWORD flags)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterObjectType(
            m_name.c_str(),
            static_cast<int>(sizeof(Class)),
            flags
        );
        assert(r >= 0);

        if(r > 0)
            m_this_type_id = r;
    }

    template <typename Fn, AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    void method_impl(
        std::string_view decl, Fn&& fn, call_conv_t<CallConv>, void* aux = nullptr
    ) requires(!ForceGeneric)
    {
        [[maybe_unused]]
        int r = with_cstr(
            [this, &fn, &aux](const char* decl)
            {
                return m_engine->RegisterObjectMethod(
                    m_name.c_str(),
                    decl,
                    to_asSFuncPtr(fn),
                    CallConv,
                    aux
                );
            },
            decl
        );
        assert(r >= 0);
    }

    // Implementation Note: DO NOT DELETE this specialization!
    // MSVC needs this to produce correct result.
    // 2025-2-21: Tested with MSVC 19.42.34436
    void method_impl(
        std::string_view decl,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>,
        void* aux = nullptr
    )
    {
        [[maybe_unused]]
        int r = with_cstr(
            [this, &gfn, &aux](const char* decl)
            {
                return m_engine->RegisterObjectMethod(
                    m_name.c_str(),
                    decl,
                    to_asSFuncPtr(gfn),
                    AS_NAMESPACE_QUALIFIER asCALL_GENERIC,
                    aux
                );
            },
            decl
        );
        assert(r >= 0);
    }

    template <
        typename Fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    void behaviour_impl(
        AS_NAMESPACE_QUALIFIER asEBehaviours beh,
        const char* decl,
        Fn&& fn,
        call_conv_t<CallConv>,
        void* aux = nullptr
    )
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterObjectBehaviour(
            m_name.c_str(),
            beh,
            decl,
            to_asSFuncPtr(fn),
            CallConv,
            aux
        );
        assert(r >= 0);
    }

    void property_impl(std::string_view decl, std::size_t off)
    {
        [[maybe_unused]]
        int r = with_cstr(
            [this, off](const char* decl)
            {
                return m_engine->RegisterObjectProperty(
                    m_name.c_str(),
                    decl,
                    static_cast<int>(off)
                );
            },
            decl
        );
        assert(r >= 0);
    }

    template <typename MemberPointer>
    requires(std::is_member_object_pointer_v<MemberPointer>)
    void property_impl(std::string_view decl, MemberPointer mp)
    {
        property_impl(decl, member_offset(mp));
    }

#define ASBIND20_CLASS_UNARY_PREFIX_OP(as_op_sig, cpp_op, decl_arg_list, return_type, const_)      \
    std::string as_op_sig##_decl() const                                                           \
    {                                                                                              \
        return string_concat decl_arg_list;                                                        \
    }                                                                                              \
    template <typename Class>                                                                      \
    void as_op_sig##_impl_generic()                                                                \
    {                                                                                              \
        method_impl(                                                                               \
            as_op_sig##_decl().c_str(),                                                            \
            +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void                              \
            {                                                                                      \
                using this_arg_t = std::conditional_t<(#const_[0] != '\0'), const Class&, Class&>; \
                set_generic_return<return_type>(                                                   \
                    gen,                                                                           \
                    cpp_op get_generic_object<this_arg_t>(gen)                                     \
                );                                                                                 \
            },                                                                                     \
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>                                       \
        );                                                                                         \
    }                                                                                              \
    template <typename Class>                                                                      \
    void as_op_sig##_impl_native()                                                                 \
    {                                                                                              \
        static constexpr bool has_member_func = requires() {                                       \
            static_cast<return_type (Class::*)() const_>(&Class::operator cpp_op);                 \
        };                                                                                         \
        if constexpr(has_member_func)                                                              \
        {                                                                                          \
            method_impl(                                                                           \
                as_op_sig##_decl().c_str(),                                                        \
                static_cast<return_type (Class::*)() const_>(&Class::operator cpp_op),             \
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>                                  \
            );                                                                                     \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            using this_arg_t = std::conditional_t<(#const_[0] != '\0'), const Class&, Class&>;     \
            this->method_impl(                                                                     \
                as_op_sig##_decl().c_str(),                                                        \
                +[](this_arg_t this_) -> return_type                                               \
                {                                                                                  \
                    return cpp_op this_;                                                           \
                },                                                                                 \
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>                            \
            );                                                                                     \
        }                                                                                          \
    }

    ASBIND20_CLASS_UNARY_PREFIX_OP(
        opNeg, -, (m_name, " opNeg() const"), Class, const
    )

    ASBIND20_CLASS_UNARY_PREFIX_OP(
        opPreInc, ++, (m_name, "& opPreInc()"), Class&,
    )
    ASBIND20_CLASS_UNARY_PREFIX_OP(
        opPreDec, --, (m_name, "& opPreDec()"), Class&,
    )

#undef ASBIND20_CLASS_REGISTER_UNARY_PREFIX_OP

    // Only for operator++/--(int)
#define ASBIND20_CLASS_UNARY_SUFFIX_OP(as_op_sig, cpp_op)                   \
    std::string as_op_sig##_decl() const                                    \
    {                                                                       \
        return string_concat(m_name, " " #as_op_sig "()");                  \
    }                                                                       \
    template <typename Class>                                               \
    void as_op_sig##_impl_generic()                                         \
    {                                                                       \
        method_impl(                                                        \
            as_op_sig##_decl().c_str(),                                     \
            +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void       \
            {                                                               \
                set_generic_return<Class>(                                  \
                    gen,                                                    \
                    get_generic_object<Class&>(gen) cpp_op                  \
                );                                                          \
            },                                                              \
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>                \
        );                                                                  \
    }                                                                       \
    template <typename Class>                                               \
    void as_op_sig##_impl_native()                                          \
    {                                                                       \
        /* Use a wrapper to deal with the hidden int of postfix operator */ \
        method_impl(                                                        \
            as_op_sig##_decl().c_str(),                                     \
            +[](Class& this_) -> Class                                      \
            { return this_ cpp_op; },                                       \
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>          \
        );                                                                  \
    }

    ASBIND20_CLASS_UNARY_SUFFIX_OP(opPostInc, ++)
    ASBIND20_CLASS_UNARY_SUFFIX_OP(opPostDec, --)

#undef ASBIND20_CLASS_UNARY_SUFFIX_OP

#define ASBIND20_CLASS_BINARY_OP_GENERIC(as_decl, cpp_op, return_type, const_, rhs_type)           \
    do {                                                                                           \
        this->method_impl(                                                                         \
            as_decl,                                                                               \
            +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void                              \
            {                                                                                      \
                using this_arg_t = std::conditional_t<(#const_[0] != '\0'), const Class&, Class&>; \
                set_generic_return<return_type>(                                                   \
                    gen,                                                                           \
                    get_generic_object<this_arg_t>(gen) cpp_op get_generic_arg<rhs_type>(gen, 0)   \
                );                                                                                 \
            },                                                                                     \
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>                                       \
        );                                                                                         \
    } while(0)

#define ASBIND20_CLASS_BINARY_OP_NATIVE(as_decl, cpp_op, return_type, const_, rhs_type)        \
    do {                                                                                       \
        static constexpr bool has_member_func = requires() {                                   \
            static_cast<return_type (Class::*)(rhs_type) const_>(&Class::operator cpp_op);     \
        };                                                                                     \
        if constexpr(has_member_func)                                                          \
        {                                                                                      \
            this->method_impl(                                                                 \
                as_decl,                                                                       \
                static_cast<return_type (Class::*)(rhs_type) const_>(&Class::operator cpp_op), \
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>                              \
            );                                                                                 \
        }                                                                                      \
        else                                                                                   \
        {                                                                                      \
            using this_arg_t = std::conditional_t<(#const_[0] != '\0'), const Class&, Class&>; \
            this->method_impl(                                                                 \
                as_decl,                                                                       \
                +[](this_arg_t lhs, rhs_type rhs) -> return_type                               \
                {                                                                              \
                    return lhs cpp_op rhs;                                                     \
                },                                                                             \
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>                        \
            );                                                                                 \
        }                                                                                      \
    } while(0)

#define ASBIND20_CLASS_BINARY_OP_IMPL(as_op_sig, cpp_op, decl_arg_list, return_type, const_, rhs_type) \
    std::string as_op_sig##_decl() const                                                               \
    {                                                                                                  \
        return string_concat decl_arg_list;                                                            \
    }                                                                                                  \
    template <typename Class>                                                                          \
    void as_op_sig##_impl_generic()                                                                    \
    {                                                                                                  \
        ASBIND20_CLASS_BINARY_OP_GENERIC(                                                              \
            as_op_sig##_decl().c_str(), cpp_op, return_type, const_, rhs_type                          \
        );                                                                                             \
    }                                                                                                  \
    template <typename Class>                                                                          \
    void as_op_sig##_impl_native()                                                                     \
    {                                                                                                  \
        ASBIND20_CLASS_BINARY_OP_NATIVE(                                                               \
            as_op_sig##_decl().c_str(), cpp_op, return_type, const_, rhs_type                          \
        );                                                                                             \
    }

    // Predefined method names:
    // https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_class_ops.html

    // Assignment operators

    ASBIND20_CLASS_BINARY_OP_IMPL(
        opAssign, =, (m_name, "& opAssign(const ", m_name, " &in)"), Class&, , const Class&
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opAddAssign, +=, (m_name, "& opAddAssign(const ", m_name, " &in)"), Class&, , const Class&
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opSubAssign, -=, (m_name, "& opSubAssign(const ", m_name, " &in)"), Class&, , const Class&
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opMulAssign, *=, (m_name, "& opMulAssign(const ", m_name, " &in)"), Class&, , const Class&
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opDivAssign, /=, (m_name, "& opDivAssign(const ", m_name, " &in)"), Class&, , const Class&
    )

    // Comparison operators

    ASBIND20_CLASS_BINARY_OP_IMPL(
        opEquals, ==, ("bool opEquals(const ", m_name, " &in) const"), bool, const, const Class&
    )

    // opCmp needs special logic to translate the result of operator<=> from C++

    std::string opCmp_decl() const
    {
        return string_concat("int opCmp(const ", m_name, "&in) const");
    }

    template <typename Class>
    void opCmp_impl_generic()
    {
        method_impl(
            opCmp_decl().c_str(),
            +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
            {
                set_generic_return<int>(
                    gen,
                    translate_three_way(
                        get_generic_object<const Class&>(gen) <=> get_generic_arg<const Class&>(gen, 0)
                    )
                );
            },
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>
        );
    }

    template <typename Class>
    void opCmp_impl_native()
    {
        method_impl(
            opCmp_decl().c_str(),
            +[](const Class& lhs, const Class& rhs) -> int
            {
                return translate_three_way(lhs <=> rhs);
            },
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
        );
    }

    ASBIND20_CLASS_BINARY_OP_IMPL(
        opAdd, +, (m_name, " opAdd(const ", m_name, " &in) const"), Class, const, const Class&
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opSub, -, (m_name, " opSub(const ", m_name, " &in) const"), Class, const, const Class&
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opMul, *, (m_name, " opMul(const ", m_name, " &in) const"), Class, const, const Class&
    )
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opDiv, /, (m_name, " opDiv(const ", m_name, " &in) const"), Class, const, const Class&
    )

#undef ASBIND20_CLASS_BINARY_OP_GENERIC
#undef ASBIND20_CLASS_BINARY_OP_NATIVE
#undef ASBIND20_CLASS_BINARY_OP_IMPL

    void member_funcdef_impl(std::string_view decl)
    {
        full_funcdef(
            detail::generate_member_funcdef(m_name, decl).c_str()
        );
    }

    static std::string decl_opConv(std::string_view ret, bool implicit)
    {
        if(implicit)
            return string_concat(ret, " opImplConv() const");
        else
            return string_concat(ret, " opConv() const");
    }

    template <typename Class, typename To>
    void opConv_impl_native(std::string_view ret, bool implicit)
    {
        auto wrapper = wrappers::opConv<std::add_const_t<Class>, To>::generate(
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
        );

        method_impl(
            decl_opConv(ret, implicit).c_str(),
            wrapper,
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
        );
    }

    template <typename Class, typename To>
    void opConv_impl_generic(std::string_view ret, bool implicit)
    {
        auto wrapper = wrappers::opConv<std::add_const_t<Class>, To>::generate(generic_call_conv);

        method_impl(
            decl_opConv(ret, implicit).c_str(),
            wrapper,
            generic_call_conv
        );
    }

    void as_string_impl(
        const char* name,
        AS_NAMESPACE_QUALIFIER asIStringFactory* factory
    )
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterStringFactory(name, factory);
        assert(r >= 0);
    }

private:
    void full_funcdef(const char* decl)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterFuncdef(decl);
        assert(r >= 0);
    }
};

#define ASBIND20_CLASS_TEMPLATE_CALLBACK(register_type)                                        \
    register_type& template_callback(                                                          \
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn                                                 \
    ) requires(Template)                                                                       \
    {                                                                                          \
        this->behaviour_impl(                                                                  \
            AS_NAMESPACE_QUALIFIER asBEHAVE_TEMPLATE_CALLBACK,                                 \
            "bool f(int&in,bool&out)",                                                         \
            gfn,                                                                               \
            generic_call_conv                                                                  \
        );                                                                                     \
        return *this;                                                                          \
    }                                                                                          \
    template <native_function Fn>                                                              \
    register_type& template_callback(Fn&& fn) requires(Template && !ForceGeneric)              \
    {                                                                                          \
        this->behaviour_impl(                                                                  \
            AS_NAMESPACE_QUALIFIER asBEHAVE_TEMPLATE_CALLBACK,                                 \
            "bool f(int&in,bool&out)",                                                         \
            fn,                                                                                \
            call_conv<detail::deduce_function_callconv<std::decay_t<Fn>>()>                    \
        );                                                                                     \
        return *this;                                                                          \
    }                                                                                          \
    template <auto Callback>                                                                   \
    register_type& template_callback(use_generic_t, fp_wrapper_t<Callback>) requires(Template) \
    {                                                                                          \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                               \
            detail::deduce_beh_callconv<                                                       \
                AS_NAMESPACE_QUALIFIER asBEHAVE_TEMPLATE_CALLBACK,                             \
                Class,                                                                         \
                std::decay_t<decltype(Callback)>>();                                           \
        template_callback(                                                                     \
            to_asGENFUNC_t(fp<Callback>, call_conv<conv>)                                      \
        );                                                                                     \
        return *this;                                                                          \
    }                                                                                          \
    template <auto Callback>                                                                   \
    register_type& template_callback(fp_wrapper_t<Callback>) requires(Template)                \
    {                                                                                          \
        if constexpr(ForceGeneric)                                                             \
            template_callback(use_generic, fp<Callback>);                                      \
        else                                                                                   \
            template_callback(Callback);                                                       \
        return *this;                                                                          \
    }

#define ASBIND20_CLASS_METHOD(register_type)                                \
    template <                                                              \
        native_function Fn,                                                 \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                   \
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)             \
    register_type& method(                                                  \
        std::string_view decl,                                              \
        Fn&& fn,                                                            \
        call_conv_t<CallConv>                                               \
    ) requires(!ForceGeneric)                                               \
    {                                                                       \
        this->method_impl(decl, std::forward<Fn>(fn), call_conv<CallConv>); \
        return *this;                                                       \
    }                                                                       \
    template <native_function Fn>                                           \
    register_type& method(                                                  \
        const char* decl,                                                   \
        Fn&& fn                                                             \
    ) requires(!ForceGeneric)                                               \
    {                                                                       \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =            \
            method_callconv<Fn>();                                          \
        this->method_impl(                                                  \
            decl,                                                           \
            fn,                                                             \
            call_conv<conv>                                                 \
        );                                                                  \
        return *this;                                                       \
    }                                                                       \
    register_type& method(                                                  \
        std::string_view decl,                                              \
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,                             \
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}             \
    )                                                                       \
    {                                                                       \
        this->method_impl(                                                  \
            decl,                                                           \
            gfn,                                                            \
            generic_call_conv                                               \
        );                                                                  \
        return *this;                                                       \
    }

#define ASBIND20_CLASS_METHOD_AUXILIARY(register_type)           \
    template <                                                   \
        native_function Fn,                                      \
        typename Auxiliary,                                      \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>        \
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)  \
    register_type& method(                                       \
        std::string_view decl,                                   \
        Fn&& fn,                                                 \
        auxiliary_wrapper<Auxiliary> aux,                        \
        call_conv_t<CallConv>                                    \
    ) requires(!ForceGeneric)                                    \
    {                                                            \
        this->method_impl(                                       \
            decl,                                                \
            std::forward<Fn>(fn),                                \
            call_conv<CallConv>,                                 \
            my_base::get_auxiliary_address(aux)                  \
        );                                                       \
        return *this;                                            \
    }                                                            \
    template <                                                   \
        native_function Fn,                                      \
        typename Auxiliary>                                      \
    register_type& method(                                       \
        std::string_view decl,                                   \
        Fn&& fn,                                                 \
        auxiliary_wrapper<Auxiliary> aux                         \
    ) requires(!ForceGeneric)                                    \
    {                                                            \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv = \
            method_callconv_aux<Fn, Auxiliary>();                \
        this->method(                                            \
            decl,                                                \
            std::forward<Fn>(fn),                                \
            aux,                                                 \
            call_conv<conv>                                      \
        );                                                       \
        return *this;                                            \
    }                                                            \
    template <typename Auxiliary>                                \
    register_type& method(                                       \
        std::string_view decl,                                   \
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,                  \
        auxiliary_wrapper<Auxiliary> aux,                        \
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}  \
    )                                                            \
    {                                                            \
        this->method_impl(                                       \
            decl,                                                \
            gfn,                                                 \
            generic_call_conv,                                   \
            my_base::get_auxiliary_address(aux)                  \
        );                                                       \
        return *this;                                            \
    }

#define ASBIND20_CLASS_WRAPPED_METHOD(register_type)                          \
    template <                                                                \
        auto Method,                                                          \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                     \
    register_type& method(                                                    \
        use_generic_t,                                                        \
        std::string_view decl,                                                \
        fp_wrapper_t<Method>,                                                 \
        call_conv_t<CallConv>                                                 \
    )                                                                         \
    {                                                                         \
        this->method_impl(                                                    \
            decl,                                                             \
            to_asGENFUNC_t(fp<Method>, call_conv<CallConv>),                  \
            generic_call_conv                                                 \
        );                                                                    \
        return *this;                                                         \
    }                                                                         \
    template <auto Method>                                                    \
    register_type& method(                                                    \
        use_generic_t,                                                        \
        std::string_view decl,                                                \
        fp_wrapper_t<Method>                                                  \
    )                                                                         \
    {                                                                         \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =              \
            method_callconv<Method>();                                        \
        this->method_impl(                                                    \
            decl,                                                             \
            to_asGENFUNC_t(fp<Method>, call_conv<conv>),                      \
            generic_call_conv                                                 \
        );                                                                    \
        return *this;                                                         \
    }                                                                         \
    template <                                                                \
        auto Method,                                                          \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                     \
    register_type& method(                                                    \
        std::string_view decl,                                                \
        fp_wrapper_t<Method>,                                                 \
        call_conv_t<CallConv>                                                 \
    )                                                                         \
    {                                                                         \
        if constexpr(ForceGeneric)                                            \
            this->method(use_generic, decl, fp<Method>, call_conv<CallConv>); \
        else                                                                  \
            this->method(decl, Method, call_conv<CallConv>);                  \
        return *this;                                                         \
    }                                                                         \
    template <auto Method>                                                    \
    register_type& method(                                                    \
        std::string_view decl,                                                \
        fp_wrapper_t<Method>                                                  \
    )                                                                         \
    {                                                                         \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =              \
            method_callconv<Method>();                                        \
        if constexpr(ForceGeneric)                                            \
            this->method(use_generic, decl, fp<Method>, call_conv<conv>);     \
        else                                                                  \
            this->method(decl, Method, call_conv<conv>);                      \
        return *this;                                                         \
    }

#define ASBIND20_CLASS_WRAPPED_METHOD_AUXILIARY(register_type)                     \
    template <                                                                     \
        auto Method,                                                               \
        typename Auxiliary,                                                        \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                          \
    register_type& method(                                                         \
        use_generic_t,                                                             \
        std::string_view decl,                                                     \
        fp_wrapper_t<Method>,                                                      \
        auxiliary_wrapper<Auxiliary> aux,                                          \
        call_conv_t<CallConv>                                                      \
    )                                                                              \
    {                                                                              \
        this->method_impl(                                                         \
            decl,                                                                  \
            to_asGENFUNC_t(fp<Method>, call_conv<CallConv>),                       \
            generic_call_conv,                                                     \
            my_base::get_auxiliary_address(aux)                                    \
        );                                                                         \
        return *this;                                                              \
    }                                                                              \
    template <auto Method, typename Auxiliary>                                     \
    register_type& method(                                                         \
        use_generic_t,                                                             \
        std::string_view decl,                                                     \
        fp_wrapper_t<Method>,                                                      \
        auxiliary_wrapper<Auxiliary> aux                                           \
    )                                                                              \
    {                                                                              \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                   \
            method_callconv_aux<Method, Auxiliary>();                              \
        this->method_impl(                                                         \
            decl,                                                                  \
            to_asGENFUNC_t(fp<Method>, call_conv<conv>),                           \
            generic_call_conv,                                                     \
            my_base::get_auxiliary_address(aux)                                    \
        );                                                                         \
        return *this;                                                              \
    }                                                                              \
    template <                                                                     \
        auto Method,                                                               \
        typename Auxiliary,                                                        \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                          \
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)                    \
    register_type& method(                                                         \
        std::string_view decl,                                                     \
        fp_wrapper_t<Method>,                                                      \
        auxiliary_wrapper<Auxiliary> aux,                                          \
        call_conv_t<CallConv>                                                      \
    )                                                                              \
    {                                                                              \
        if constexpr(ForceGeneric)                                                 \
            this->method(use_generic, decl, fp<Method>, aux, call_conv<CallConv>); \
        else                                                                       \
            this->method(decl, Method, call_conv<CallConv>, aux);                  \
        return *this;                                                              \
    }                                                                              \
    template <auto Method, typename Auxiliary>                                     \
    register_type& method(                                                         \
        std::string_view decl,                                                     \
        fp_wrapper_t<Method>,                                                      \
        auxiliary_wrapper<Auxiliary> aux                                           \
    )                                                                              \
    {                                                                              \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                   \
            method_callconv_aux<Method, Auxiliary>();                              \
        if constexpr(ForceGeneric)                                                 \
            this->method(use_generic, decl, fp<Method>, aux, call_conv<conv>);     \
        else                                                                       \
            this->method(decl, Method, aux, call_conv<conv>);                      \
        return *this;                                                              \
    }

#define ASBIND20_CLASS_WRAPPED_LAMBDA_METHOD(register_type)                 \
    template <                                                              \
        noncapturing_lambda Lambda,                                         \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                   \
    register_type& method(                                                  \
        use_generic_t,                                                      \
        std::string_view decl,                                              \
        const Lambda&,                                                      \
        call_conv_t<CallConv>                                               \
    )                                                                       \
    {                                                                       \
        this->method_impl(                                                  \
            decl,                                                           \
            to_asGENFUNC_t(Lambda{}, call_conv<CallConv>),                  \
            generic_call_conv                                               \
        );                                                                  \
        return *this;                                                       \
    }                                                                       \
    template <noncapturing_lambda Lambda>                                   \
    register_type& method(                                                  \
        use_generic_t,                                                      \
        std::string_view decl,                                              \
        const Lambda&                                                       \
    )                                                                       \
    {                                                                       \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =            \
            method_callconv<Lambda>();                                      \
        this->method(use_generic, decl, Lambda{}, call_conv<conv>);         \
        return *this;                                                       \
    }                                                                       \
    template <                                                              \
        noncapturing_lambda Lambda,                                         \
        asECallConvTypes CallConv>                                          \
    register_type& method(                                                  \
        std::string_view decl,                                              \
        const Lambda&,                                                      \
        call_conv_t<CallConv>                                               \
    )                                                                       \
    {                                                                       \
        if constexpr(ForceGeneric)                                          \
            this->method(use_generic, decl, Lambda{}, call_conv<CallConv>); \
        else                                                                \
            this->method_impl(decl, +Lambda{}, call_conv<CallConv>);        \
        return *this;                                                       \
    }                                                                       \
    template <noncapturing_lambda Lambda>                                   \
    register_type& method(                                                  \
        std::string_view decl,                                              \
        const Lambda&                                                       \
    )                                                                       \
    {                                                                       \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =            \
            method_callconv<Lambda>();                                      \
        this->method(decl, Lambda{}, call_conv<conv>);                      \
        return *this;                                                       \
    }

#define ASBIND20_CLASS_WRAPPED_VAR_TYPE_METHOD(register_type)                                    \
    template <                                                                                   \
        auto Function,                                                                           \
        std::size_t... Is,                                                                       \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                                        \
    register_type& method(                                                                       \
        use_generic_t,                                                                           \
        std::string_view decl,                                                                   \
        fp_wrapper_t<Function>,                                                                  \
        var_type_t<Is...>,                                                                       \
        call_conv_t<CallConv>                                                                    \
    )                                                                                            \
    {                                                                                            \
        this->method_impl(                                                                       \
            decl,                                                                                \
            to_asGENFUNC_t(fp<Function>, call_conv<CallConv>, var_type<Is...>),                  \
            generic_call_conv                                                                    \
        );                                                                                       \
        return *this;                                                                            \
    }                                                                                            \
    template <                                                                                   \
        auto Function,                                                                           \
        std::size_t... Is,                                                                       \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                                        \
    register_type& method(                                                                       \
        std::string_view decl,                                                                   \
        fp_wrapper_t<Function>,                                                                  \
        var_type_t<Is...>,                                                                       \
        call_conv_t<CallConv>                                                                    \
    )                                                                                            \
    {                                                                                            \
        if constexpr(ForceGeneric)                                                               \
            this->method(use_generic, decl, fp<Function>, var_type<Is...>, call_conv<CallConv>); \
        else                                                                                     \
        {                                                                                        \
            this->method_impl(                                                                   \
                decl,                                                                            \
                Function,                                                                        \
                call_conv<CallConv>                                                              \
            );                                                                                   \
        }                                                                                        \
        return *this;                                                                            \
    }                                                                                            \
    template <                                                                                   \
        auto Function,                                                                           \
        std::size_t... Is>                                                                       \
    register_type& method(                                                                       \
        use_generic_t,                                                                           \
        std::string_view decl,                                                                   \
        fp_wrapper_t<Function>,                                                                  \
        var_type_t<Is...>                                                                        \
    )                                                                                            \
    {                                                                                            \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                                 \
            method_callconv<Function>();                                                         \
        this->method(                                                                            \
            use_generic,                                                                         \
            decl,                                                                                \
            fp<Function>,                                                                        \
            var_type<Is...>,                                                                     \
            call_conv<conv>                                                                      \
        );                                                                                       \
        return *this;                                                                            \
    }                                                                                            \
    template <                                                                                   \
        auto Function,                                                                           \
        std::size_t... Is>                                                                       \
    register_type& method(                                                                       \
        std::string_view decl,                                                                   \
        fp_wrapper_t<Function>,                                                                  \
        var_type_t<Is...>                                                                        \
    )                                                                                            \
    {                                                                                            \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                                 \
            method_callconv<Function>();                                                         \
        if constexpr(ForceGeneric)                                                               \
            this->method(use_generic, decl, fp<Function>, var_type<Is...>, call_conv<conv>);     \
        else                                                                                     \
        {                                                                                        \
            this->method_impl(                                                                   \
                decl,                                                                            \
                Function,                                                                        \
                call_conv<conv>                                                                  \
            );                                                                                   \
        }                                                                                        \
        return *this;                                                                            \
    }

#define ASBIND20_CLASS_WRAPPED_VAR_TYPE_METHOD_AUXILIARY(register_type)                               \
    template <                                                                                        \
        auto Function,                                                                                \
        std::size_t... Is,                                                                            \
        typename Auxiliary,                                                                           \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                                             \
    register_type& method(                                                                            \
        use_generic_t,                                                                                \
        std::string_view decl,                                                                        \
        fp_wrapper_t<Function>,                                                                       \
        var_type_t<Is...>,                                                                            \
        auxiliary_wrapper<Auxiliary> aux,                                                             \
        call_conv_t<CallConv>                                                                         \
    )                                                                                                 \
    {                                                                                                 \
        this->method_impl(                                                                            \
            decl,                                                                                     \
            to_asGENFUNC_t(fp<Function>, call_conv<CallConv>, var_type<Is...>),                       \
            generic_call_conv,                                                                        \
            my_base::get_auxiliary_address(aux)                                                       \
        );                                                                                            \
        return *this;                                                                                 \
    }                                                                                                 \
    template <                                                                                        \
        auto Function,                                                                                \
        std::size_t... Is,                                                                            \
        typename Auxiliary,                                                                           \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                                             \
    register_type& method(                                                                            \
        std::string_view decl,                                                                        \
        fp_wrapper_t<Function>,                                                                       \
        var_type_t<Is...>,                                                                            \
        auxiliary_wrapper<Auxiliary> aux,                                                             \
        call_conv_t<CallConv>                                                                         \
    )                                                                                                 \
    {                                                                                                 \
        if constexpr(ForceGeneric)                                                                    \
            this->method(use_generic, decl, fp<Function>, var_type<Is...>, aux, call_conv<CallConv>); \
        else                                                                                          \
        {                                                                                             \
            this->method_impl(                                                                        \
                decl,                                                                                 \
                Function,                                                                             \
                call_conv<CallConv>,                                                                  \
                my_base::get_auxiliary_address(aux)                                                   \
            );                                                                                        \
        }                                                                                             \
        return *this;                                                                                 \
    }                                                                                                 \
    template <                                                                                        \
        auto Function,                                                                                \
        std::size_t... Is,                                                                            \
        typename Auxiliary>                                                                           \
    register_type& method(                                                                            \
        use_generic_t,                                                                                \
        std::string_view decl,                                                                        \
        fp_wrapper_t<Function>,                                                                       \
        var_type_t<Is...>,                                                                            \
        auxiliary_wrapper<Auxiliary> aux                                                              \
    )                                                                                                 \
    {                                                                                                 \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                                      \
            method_callconv_aux<Function, Auxiliary>();                                               \
        this->method(                                                                                 \
            use_generic,                                                                              \
            decl,                                                                                     \
            fp<Function>,                                                                             \
            var_type<Is...>,                                                                          \
            aux,                                                                                      \
            call_conv<conv>                                                                           \
        );                                                                                            \
        return *this;                                                                                 \
    }                                                                                                 \
    template <                                                                                        \
        auto Function,                                                                                \
        std::size_t... Is,                                                                            \
        typename Auxiliary>                                                                           \
    register_type& method(                                                                            \
        std::string_view decl,                                                                        \
        fp_wrapper_t<Function>,                                                                       \
        var_type_t<Is...>,                                                                            \
        auxiliary_wrapper<Auxiliary> aux                                                              \
    )                                                                                                 \
    {                                                                                                 \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                                      \
            method_callconv_aux<Function, Auxiliary>();                                               \
        if constexpr(ForceGeneric)                                                                    \
            this->method(use_generic, decl, fp<Function>, var_type<Is...>, aux, call_conv<conv>);     \
        else                                                                                          \
        {                                                                                             \
            this->method_impl(                                                                        \
                decl,                                                                                 \
                Function,                                                                             \
                call_conv<conv>,                                                                      \
                my_base::get_auxiliary_address(aux)                                                   \
            );                                                                                        \
        }                                                                                             \
        return *this;                                                                                 \
    }

#define ASBIND20_CLASS_WRAPPED_LAMBDA_VAR_TYPE_METHOD(register_type)                         \
    template <                                                                               \
        noncapturing_lambda Lambda,                                                          \
        std::size_t... Is,                                                                   \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                                    \
    register_type& method(                                                                   \
        use_generic_t,                                                                       \
        std::string_view decl,                                                               \
        const Lambda&,                                                                       \
        var_type_t<Is...>,                                                                   \
        call_conv_t<CallConv>                                                                \
    )                                                                                        \
    {                                                                                        \
        this->method_impl(                                                                   \
            decl,                                                                            \
            to_asGENFUNC_t(Lambda{}, call_conv<CallConv>, var_type<Is...>),                  \
            generic_call_conv                                                                \
        );                                                                                   \
        return *this;                                                                        \
    }                                                                                        \
    template <                                                                               \
        noncapturing_lambda Lambda,                                                          \
        std::size_t... Is,                                                                   \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                                    \
    register_type& method(                                                                   \
        std::string_view decl,                                                               \
        const Lambda&,                                                                       \
        var_type_t<Is...>,                                                                   \
        call_conv_t<CallConv>                                                                \
    )                                                                                        \
    {                                                                                        \
        if constexpr(ForceGeneric)                                                           \
            this->method(use_generic, decl, Lambda{}, var_type<Is...>, call_conv<CallConv>); \
        else                                                                                 \
        {                                                                                    \
            this->method_impl(                                                               \
                decl,                                                                        \
                +Lambda{},                                                                   \
                call_conv<CallConv>                                                          \
            );                                                                               \
        }                                                                                    \
        return *this;                                                                        \
    }                                                                                        \
    template <                                                                               \
        noncapturing_lambda Lambda,                                                          \
        std::size_t... Is>                                                                   \
    register_type& method(                                                                   \
        use_generic_t,                                                                       \
        std::string_view decl,                                                               \
        const Lambda&,                                                                       \
        var_type_t<Is...>                                                                    \
    )                                                                                        \
    {                                                                                        \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                             \
            method_callconv<Lambda>();                                                       \
        this->method(                                                                        \
            use_generic,                                                                     \
            decl,                                                                            \
            Lambda{},                                                                        \
            var_type<Is...>,                                                                 \
            call_conv<conv>                                                                  \
        );                                                                                   \
        return *this;                                                                        \
    }                                                                                        \
    template <                                                                               \
        noncapturing_lambda Lambda,                                                          \
        std::size_t... Is>                                                                   \
    register_type& method(                                                                   \
        std::string_view decl,                                                               \
        const Lambda&,                                                                       \
        var_type_t<Is...>                                                                    \
    )                                                                                        \
    {                                                                                        \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                             \
            method_callconv<Lambda>();                                                       \
        if constexpr(ForceGeneric)                                                           \
            this->method(use_generic, decl, Lambda{}, var_type<Is...>, call_conv<conv>);     \
        else                                                                                 \
        {                                                                                    \
            this->method_impl(                                                               \
                decl,                                                                        \
                +Lambda{},                                                                   \
                call_conv<conv>                                                              \
            );                                                                               \
        }                                                                                    \
        return *this;                                                                        \
    }

/**
 * @brief Register helper for value class
 *
 * @tparam Class Class to be registered
 * @tparam Template True if the class is a templated type
 * @tparam ForceGeneric Force all registered methods and behaviors to use the generic calling convention
 */
template <typename Class, bool Template = false, bool ForceGeneric = false>
class basic_value_class final : public class_register_helper_base<ForceGeneric>
{
    using my_base = class_register_helper_base<ForceGeneric>;

    using my_base::m_engine;
    using my_base::m_name;

public:
    using class_type = Class;

    basic_value_class(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        std::string name,
        AS_NAMESPACE_QUALIFIER asQWORD flags = 0
    )
        : my_base(engine, std::move(name))
    {
        flags |= AS_NAMESPACE_QUALIFIER asOBJ_VALUE;

        assert(!(flags & (AS_NAMESPACE_QUALIFIER asOBJ_REF)));

        if constexpr(!Template)
        {
            assert(!(flags & (AS_NAMESPACE_QUALIFIER asOBJ_TEMPLATE)));
            flags |= AS_NAMESPACE_QUALIFIER asGetTypeTraits<Class>();
        }
        else
        {
            flags |= AS_NAMESPACE_QUALIFIER asOBJ_TEMPLATE;
        }

        this->template register_object_type<Class>(flags);
    }

private:
    template <typename Method>
    static consteval auto method_callconv() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        if constexpr(noncapturing_lambda<Method>)
            return detail::deduce_lambda_callconv<Class, Method>();
        else
            return detail::deduce_method_callconv<Class, Method>();
    }

    template <auto Method>
    static consteval auto method_callconv() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        return method_callconv<std::decay_t<decltype(Method)>>();
    }

    template <typename Method, typename Auxiliary>
    static consteval auto method_callconv_aux() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        return detail::deduce_method_callconv_aux<Class, Method, Auxiliary>();
    }

    template <auto Method, typename Auxiliary>
    static consteval auto method_callconv_aux() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        return method_callconv_aux<std::decay_t<decltype(Method)>, Auxiliary>();
    }

    std::string decl_constructor_impl(std::string_view params, bool explicit_) const
    {
        if constexpr(Template)
        {
            if(explicit_)
            {
                return params.empty() ?
                           "void f(int&in)explicit" :
                           string_concat("void f(int&in,", params, ")explicit");
            }
            else
            {
                return params.empty() ?
                           "void f(int&in)" :
                           string_concat("void f(int&in,", params, ')');
            }
        }
        else
        {
            if(explicit_)
            {
                return params.empty() ?
                           "void f()explicit" :
                           string_concat("void f(", params, ")explicit");
            }
            else
            {
                return params.empty() ?
                           "void f()" :
                           string_concat("void f(", params, ')');
            }
        }
    }

public:
    basic_value_class& constructor_function(
        std::string_view params,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            decl_constructor_impl(params, false).c_str(),
            gfn,
            generic_call_conv
        );

        return *this;
    }

    basic_value_class& constructor_function(
        std::string_view params,
        use_explicit_t,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            decl_constructor_impl(params, true).c_str(),
            gfn,
            generic_call_conv
        );

        return *this;
    }

    template <
        native_function Constructor,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    basic_value_class& constructor_function(
        std::string_view params,
        Constructor ctor,
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            decl_constructor_impl(params, false).c_str(),
            ctor,
            call_conv<CallConv>
        );

        return *this;
    }

    template <
        native_function Constructor,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    basic_value_class& constructor_function(
        std::string_view params,
        use_explicit_t,
        Constructor ctor,
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            decl_constructor_impl(params, true).c_str(),
            ctor,
            call_conv<CallConv>
        );

        return *this;
    }

    template <native_function Constructor>
    basic_value_class& constructor_function(
        std::string_view params,
        Constructor ctor
    ) requires(!ForceGeneric)
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT, Class, std::decay_t<Constructor>>();
        this->constructor_function(
            params,
            ctor,
            call_conv<conv>
        );

        return *this;
    }

    template <native_function Constructor>
    basic_value_class& constructor_function(
        std::string_view params,
        use_explicit_t,
        Constructor ctor
    ) requires(!ForceGeneric)
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT, Class, std::decay_t<Constructor>>();
        this->constructor_function(
            params,
            use_explicit,
            ctor,
            call_conv<conv>
        );

        return *this;
    }

    template <
        auto Constructor,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        fp_wrapper_t<Constructor>,
        call_conv_t<CallConv>
    )
    {
        this->constructor_function(
            params,
            wrappers::constructor_function<Constructor, Class, Template, CallConv>::generate(generic_call_conv),
            generic_call_conv
        );

        return *this;
    }

    template <
        auto Constructor,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        fp_wrapper_t<Constructor>,
        call_conv_t<CallConv>
    )
    {
        this->constructor_function(
            params,
            use_explicit,
            wrappers::constructor_function<Constructor, Class, Template, CallConv>::generate(generic_call_conv),
            generic_call_conv
        );

        return *this;
    }

    template <auto Constructor>
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        fp_wrapper_t<Constructor>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT, Class, std::decay_t<decltype(Constructor)>>();
        this->constructor_function(
            use_generic,
            params,
            fp<Constructor>,
            call_conv<conv>
        );

        return *this;
    }

    template <auto Constructor>
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        fp_wrapper_t<Constructor>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT, Class, std::decay_t<decltype(Constructor)>>();
        this->constructor_function(
            use_generic,
            params,
            use_explicit,
            fp<Constructor>,
            call_conv<conv>
        );

        return *this;
    }

    template <auto Constructor>
    basic_value_class& constructor_function(
        std::string_view params,
        fp_wrapper_t<Constructor>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT, Class, std::decay_t<decltype(Constructor)>>();
        if constexpr(ForceGeneric)
        {
            this->constructor_function(
                use_generic,
                params,
                fp<Constructor>,
                call_conv<conv>
            );
        }
        else
        {
            this->constructor_function(
                params,
                Constructor,
                call_conv<conv>
            );
        }

        return *this;
    }

    template <auto Constructor>
    basic_value_class& constructor_function(
        std::string_view params,
        use_explicit_t,
        fp_wrapper_t<Constructor>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT, Class, std::decay_t<decltype(Constructor)>>();
        if constexpr(ForceGeneric)
        {
            this->constructor_function(
                use_generic,
                params,
                use_explicit,
                fp<Constructor>,
                call_conv<conv>
            );
        }
        else
        {
            this->constructor_function(
                params,
                use_explicit,
                Constructor,
                call_conv<conv>
            );
        }

        return *this;
    }

    template <
        noncapturing_lambda Constructor,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        const Constructor&,
        call_conv_t<CallConv>
    )
    {
        this->constructor_function(
            params,
            wrappers::constructor_lambda<Constructor, Class, Template, CallConv>::generate(generic_call_conv),
            generic_call_conv
        );

        return *this;
    }

    template <
        noncapturing_lambda Constructor,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        const Constructor&,
        call_conv_t<CallConv>
    )
    {
        this->constructor_function(
            params,
            use_explicit,
            wrappers::constructor_lambda<Constructor, Class, Template, CallConv>::generate(generic_call_conv),
            generic_call_conv
        );

        return *this;
    }

    template <
        noncapturing_lambda Constructor,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    basic_value_class& constructor_function(
        std::string_view params,
        const Constructor&,
        call_conv_t<CallConv>
    )
    {
        if constexpr(ForceGeneric)
        {
            this->constructor_function(
                use_generic,
                params,
                Constructor{},
                call_conv<CallConv>
            );
        }
        else
        {
            this->constructor_function(
                params,
                +Constructor{},
                call_conv<CallConv>
            );
        }

        return *this;
    }

    template <
        noncapturing_lambda Constructor,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    basic_value_class& constructor_function(
        std::string_view params,
        use_explicit_t,
        const Constructor&,
        call_conv_t<CallConv>
    )
    {
        if constexpr(ForceGeneric)
        {
            this->constructor_function(
                use_generic,
                params,
                use_explicit,
                Constructor{},
                call_conv<CallConv>
            );
        }
        else
        {
            this->constructor_function(
                params,
                use_explicit,
                +Constructor{},
                call_conv<CallConv>
            );
        }

        return *this;
    }

    template <noncapturing_lambda Constructor>
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        const Constructor&
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT, Class, decltype(+Constructor{})>();
        this->constructor_function(
            use_generic,
            params,
            Constructor{},
            call_conv<conv>
        );

        return *this;
    }

    template <noncapturing_lambda Constructor>
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        const Constructor&
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT, Class, decltype(+Constructor{})>();
        this->constructor_function(
            use_generic,
            params,
            use_explicit,
            Constructor{},
            call_conv<conv>
        );

        return *this;
    }

    template <noncapturing_lambda Constructor>
    basic_value_class& constructor_function(
        std::string_view params,
        const Constructor&
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT, Class, decltype(+Constructor{})>();
        this->constructor_function(
            params,
            Constructor{},
            call_conv<conv>
        );

        return *this;
    }

    template <noncapturing_lambda Constructor>
    basic_value_class& constructor_function(
        std::string_view params,
        use_explicit_t,
        const Constructor&
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT, Class, decltype(+Constructor{})>();
        this->constructor_function(
            params,
            use_explicit,
            Constructor{},
            call_conv<conv>
        );

        return *this;
    }

private:
    template <typename... Args>
    void constructor_impl_generic(std::string_view params, bool explicit_)
    {
        wrappers::constructor<Class, Template, Args...> wrapper;
        if(explicit_)
        {
            constructor_function(
                params,
                use_explicit,
                wrapper.generate(generic_call_conv),
                generic_call_conv
            );
        }
        else
        {
            constructor_function(
                params,
                wrapper.generate(generic_call_conv),
                generic_call_conv
            );
        }
    }

    template <typename... Args>
    void constructor_impl_native(std::string_view params, bool explicit_)
    {
        wrappers::constructor<Class, Template, Args...> wrapper;
        if(explicit_)
        {
            constructor_function(
                params,
                use_explicit,
                wrapper.generate(call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>),
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
            );
        }
        else
        {
            constructor_function(
                params,
                wrapper.generate(call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>),
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
            );
        }
    }

    template <typename... Args>
    static consteval bool check_constructible()
    {
        if constexpr(Template)
            return is_only_constructible_v<Class, AS_NAMESPACE_QUALIFIER asITypeInfo*, Args...>;
        else
            return is_only_constructible_v<Class, Args...>;
    }

public:
    template <typename... Args>
    basic_value_class& constructor(
        use_generic_t,
        std::string_view params
    ) requires(check_constructible<Args...>())
    {
        constructor_impl_generic<Args...>(params, false);

        return *this;
    }

    template <typename... Args>
    basic_value_class& constructor(
        use_generic_t,
        std::string_view params,
        use_explicit_t
    ) requires(check_constructible<Args...>())
    {
        constructor_impl_generic<Args...>(params, true);

        return *this;
    }

    /**
     * @warning Remember to set `asOBJ_APP_CLASS_MORE_CONSTRUCTORS` if necessary!
     */
    template <typename... Args>
    basic_value_class& constructor(
        std::string_view params
    ) requires(check_constructible<Args...>())
    {
        if constexpr(ForceGeneric)
            constructor<Args...>(use_generic, params);
        else
            constructor_impl_native<Args...>(params, false);

        return *this;
    }

    /**
     * @warning Remember to set `asOBJ_APP_CLASS_MORE_CONSTRUCTORS` if necessary!
     */
    template <typename... Args>
    basic_value_class& constructor(
        std::string_view params,
        use_explicit_t
    ) requires(check_constructible<Args...>())
    {
        if constexpr(ForceGeneric)
            constructor<Args...>(use_generic, params, use_explicit);
        else
            constructor_impl_native<Args...>(params, true);

        return *this;
    }

    basic_value_class& default_constructor(use_generic_t)
    {
        constructor<>(use_generic, "");

        return *this;
    }

    basic_value_class& default_constructor()
    {
        constructor<>("");

        return *this;
    }

    basic_value_class& copy_constructor(use_generic_t)
    {
        constructor<const Class&>(
            use_generic,
            string_concat("const ", m_name, " &in")
        );

        return *this;
    }

    basic_value_class& copy_constructor()
    {
        constructor<const Class&>(
            string_concat("const ", m_name, "&in")
        );

        return *this;
    }

    basic_value_class& destructor(use_generic_t)
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_DESTRUCT,
            "void f()",
            +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
            {
                std::destroy_at(get_generic_object<Class*>(gen));
            },
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>
        );

        return *this;
    }

    basic_value_class& destructor()
    {
        if constexpr(ForceGeneric)
            destructor(use_generic);
        else
        {
            this->behaviour_impl(
                AS_NAMESPACE_QUALIFIER asBEHAVE_DESTRUCT,
                "void f()",
                +[](Class* ptr) -> void
                {
                    std::destroy_at(ptr);
                },
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
            );
        }

        return *this;
    }

    /**
     * @brief Automatically register functions based on traits using generic wrapper.
     *
     * @param traits Type traits
     */
    basic_value_class& behaviours_by_traits(
        use_generic_t,
        AS_NAMESPACE_QUALIFIER asQWORD traits = AS_NAMESPACE_QUALIFIER asGetTypeTraits<Class>()
    ) requires(!Template)
    {
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_C))
        {
            if constexpr(is_only_constructible_v<Class>)
                default_constructor(use_generic);
            else
                assert(false && "missing default constructor");
        }
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_D))
        {
            if constexpr(std::is_destructible_v<Class>)
                destructor(use_generic);
            else
                assert(false && "missing destructor");
        }
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_A))
        {
            if constexpr(std::is_copy_assignable_v<Class>)
                opAssign(use_generic);
            else
                assert(false && "missing assignment operator");
        }
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_K))
        {
            if constexpr(is_only_constructible_v<Class, const Class&>)
                copy_constructor(use_generic);
            else
                assert(false && "missing copy constructor");
        }

        return *this;
    }

    /**
     * @brief Automatically register functions based on traits.
     *
     * @param traits Type traits
     */
    basic_value_class& behaviours_by_traits(
        AS_NAMESPACE_QUALIFIER asQWORD traits = AS_NAMESPACE_QUALIFIER asGetTypeTraits<Class>()
    ) requires(!Template)
    {
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_C))
        {
            if constexpr(is_only_constructible_v<Class>)
                default_constructor();
            else
                assert(false && "missing default constructor");
        }
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_D))
        {
            if constexpr(std::is_destructible_v<Class>)
                destructor();
            else
                assert(false && "missing destructor");
        }
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_A))
        {
            if constexpr(std::is_copy_assignable_v<Class>)
                opAssign();
            else
                assert(false && "missing assignment operator");
        }
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_K))
        {
            if constexpr(is_only_constructible_v<Class, const Class&>)
                copy_constructor();
            else
                assert(false && "missing copy constructor");
        }

        return *this;
    }

private:
    constexpr std::string decl_list_constructor(std::string_view pattern) const
    {
        if constexpr(Template)
            return string_concat("void f(int&in,int&in){", pattern, '}');
        else
            return string_concat("void f(int&in){", pattern, '}');
    }

public:
    basic_value_class& list_constructor_function(
        std::string_view pattern,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_CONSTRUCT,
            decl_list_constructor(pattern).c_str(),
            gfn,
            generic_call_conv
        );

        return *this;
    }

    template <
        native_function ListConstructor,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST || CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
    basic_value_class& list_constructor_function(
        std::string_view pattern,
        ListConstructor&& ctor,
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_CONSTRUCT,
            decl_list_constructor(pattern).c_str(),
            ctor,
            call_conv<CallConv>
        );

        return *this;
    }

    template <native_function ListConstructor>
    basic_value_class& list_constructor_function(
        std::string_view pattern,
        ListConstructor&& ctor
    ) requires(!ForceGeneric)
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_CONSTRUCT, Class, std::decay_t<ListConstructor>>();
        this->list_constructor_function(
            pattern,
            ctor,
            call_conv<conv>
        );

        return *this;
    }

    template <
        auto ListConstructor,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST || CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
    basic_value_class& list_constructor_function(
        use_generic_t,
        std::string_view pattern,
        fp_wrapper_t<ListConstructor>,
        call_conv_t<CallConv>
    )
    {
        using traits = function_traits<std::decay_t<decltype(ListConstructor)>>;
        static_assert(traits::arg_count_v == 2);
        static_assert(std::is_void_v<typename traits::return_type>);

        if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
        {
            using list_buf_t = typename traits::last_arg_type;
            static_assert(std::is_pointer_v<list_buf_t>);
            list_constructor_function(
                pattern,
                +[](asIScriptGeneric* gen) -> void
                {
                    ListConstructor(
                        get_generic_object<Class*>(gen),
                        *(list_buf_t*)gen->GetAddressOfArg(0)
                    );
                },
                generic_call_conv
            );
        }
        else // CallConv == asCALL_CDECL_OBJLAST
        {
            using list_buf_t = typename traits::first_arg_type;
            static_assert(std::is_pointer_v<list_buf_t>);
            list_constructor_function(
                pattern,
                +[](asIScriptGeneric* gen) -> void
                {
                    ListConstructor(
                        *(list_buf_t*)gen->GetAddressOfArg(0),
                        get_generic_object<Class*>(gen)
                    );
                },
                generic_call_conv
            );
        }

        return *this;
    }

    template <
        auto ListConstructor,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST || CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
    basic_value_class& list_constructor_function(
        std::string_view pattern,
        fp_wrapper_t<ListConstructor>,
        call_conv_t<CallConv>
    )
    {
        if constexpr(ForceGeneric)
        {
            list_constructor_function(use_generic, pattern, fp<ListConstructor>, call_conv<CallConv>);
        }
        else
        {
            list_constructor_function(pattern, ListConstructor, call_conv<CallConv>);
        }

        return *this;
    }

    template <auto ListConstructor>
    basic_value_class& list_constructor_function(
        use_generic_t,
        std::string_view pattern,
        fp_wrapper_t<ListConstructor>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_CONSTRUCT, Class, std::decay_t<decltype(ListConstructor)>>();

        this->list_constructor_function(use_generic, pattern, fp<ListConstructor>, call_conv<conv>);

        return *this;
    }

    template <auto ListConstructor>
    basic_value_class& list_constructor_function(
        std::string_view pattern,
        fp_wrapper_t<ListConstructor>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_CONSTRUCT, Class, std::decay_t<decltype(ListConstructor)>>();
        if constexpr(ForceGeneric)
        {
            this->list_constructor_function(use_generic, pattern, fp<ListConstructor>, call_conv<conv>);
        }
        else
        {
            this->list_constructor_function(pattern, ListConstructor, call_conv<conv>);
        }

        return *this;
    }

    /**
     * @brief Register a list constructor
     *
     * @tparam ListElementType Element type
     * @tparam Policy Policy for converting initialization list from AngelScript
     * @param pattern List pattern
     */
    template <
        typename ListElementType = void,
        policies::initialization_list_policy Policy = void>
    basic_value_class& list_constructor(
        use_generic_t, std::string_view pattern, use_policy_t<Policy> = {}
    )
    {
        list_constructor_function(
            pattern,
            wrappers::list_constructor<Class, Template, ListElementType, Policy>::generate(generic_call_conv),
            generic_call_conv
        );

        return *this;
    }

    /**
     * @brief Register a list constructor
     *
     * @tparam ListElementType Element type
     * @tparam Policy Policy for converting initialization list from AngelScript
     * @param pattern List pattern
     */
    template <
        typename ListElementType = void,
        policies::initialization_list_policy Policy = void>
    basic_value_class& list_constructor(
        std::string_view pattern, use_policy_t<Policy> = {}
    )
    {
        if constexpr(ForceGeneric)
            list_constructor<ListElementType>(use_generic, pattern, use_policy<Policy>);
        else
        {
            list_constructor_function(
                pattern,
                wrappers::list_constructor<Class, Template, ListElementType, Policy>::generate(
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
                ),
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
            );
        }

        return *this;
    }

#define ASBIND20_VALUE_CLASS_OP(op_name)                   \
    basic_value_class& op_name(use_generic_t)              \
    {                                                      \
        this->template op_name##_impl_generic<Class>();    \
        return *this;                                      \
    }                                                      \
    basic_value_class& op_name()                           \
    {                                                      \
        if constexpr(ForceGeneric)                         \
            this->op_name(use_generic);                    \
        else                                               \
            this->template op_name##_impl_native<Class>(); \
        return *this;                                      \
    }

    ASBIND20_VALUE_CLASS_OP(opNeg);

    ASBIND20_VALUE_CLASS_OP(opPreInc);
    ASBIND20_VALUE_CLASS_OP(opPreDec);
    ASBIND20_VALUE_CLASS_OP(opPostInc);
    ASBIND20_VALUE_CLASS_OP(opPostDec);

    ASBIND20_VALUE_CLASS_OP(opAssign);
    ASBIND20_VALUE_CLASS_OP(opAddAssign);
    ASBIND20_VALUE_CLASS_OP(opSubAssign);
    ASBIND20_VALUE_CLASS_OP(opMulAssign);
    ASBIND20_VALUE_CLASS_OP(opDivAssign);

    ASBIND20_VALUE_CLASS_OP(opEquals);
    ASBIND20_VALUE_CLASS_OP(opCmp);

    ASBIND20_VALUE_CLASS_OP(opAdd);
    ASBIND20_VALUE_CLASS_OP(opSub);
    ASBIND20_VALUE_CLASS_OP(opMul);
    ASBIND20_VALUE_CLASS_OP(opDiv);

#undef ASBIND20_VALUE_CLASS_OP

    template <typename To>
    basic_value_class& opConv(use_generic_t, std::string_view to_decl)
    {
        this->template opConv_impl_generic<Class, To>(to_decl, false);

        return *this;
    }

    template <typename To>
    basic_value_class& opConv(std::string_view to_decl)
    {
        if constexpr(ForceGeneric)
            opConv<To>(use_generic, to_decl);
        else
        {
            this->template opConv_impl_native<Class, To>(to_decl, false);
        }

        return *this;
    }

    template <typename To>
    basic_value_class& opImplConv(use_generic_t, std::string_view to_decl)
    {
        this->template opConv_impl_generic<Class, To>(to_decl, true);

        return *this;
    }

    template <typename To>
    basic_value_class& opImplConv(std::string_view to_decl)
    {
        if constexpr(ForceGeneric)
            opImplConv<To>(use_generic, to_decl);
        else
        {
            this->template opConv_impl_native<Class, To>(to_decl, true);
        }

        return *this;
    }

    template <has_static_name To>
    basic_value_class& opConv(use_generic_t)
    {
        opConv<To>(use_generic, name_of<To>());

        return *this;
    }

    template <has_static_name To>
    basic_value_class& opConv()
    {
        opConv<To>(name_of<To>());

        return *this;
    }

    template <has_static_name To>
    basic_value_class& opImplConv(use_generic_t)
    {
        opImplConv<To>(use_generic, name_of<To>());

        return *this;
    }

    template <has_static_name To>
    basic_value_class& opImplConv()
    {
        opImplConv<To>(name_of<To>());

        return *this;
    }

#define ASBIND20_VALUE_CLASS_BEH(func_name, as_beh, as_decl)                             \
    template <native_function Fn>                                                        \
    basic_value_class& func_name(Fn&& fn) requires(!ForceGeneric)                        \
    {                                                                                    \
        using func_t = std::decay_t<Fn>;                                                 \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                         \
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER as_beh, Class, func_t>(); \
        this->behaviour_impl(                                                            \
            AS_NAMESPACE_QUALIFIER as_beh,                                               \
            as_decl,                                                                     \
            fn,                                                                          \
            call_conv<conv>                                                              \
        );                                                                               \
        return *this;                                                                    \
    }                                                                                    \
    basic_value_class& func_name(AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn)                 \
    {                                                                                    \
        this->behaviour_impl(                                                            \
            AS_NAMESPACE_QUALIFIER as_beh,                                               \
            as_decl,                                                                     \
            gfn,                                                                         \
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>                             \
        );                                                                               \
        return *this;                                                                    \
    }                                                                                    \
    template <auto Function>                                                             \
    basic_value_class& func_name(use_generic_t, fp_wrapper_t<Function>)                  \
    {                                                                                    \
        using func_t = std::decay_t<decltype(Function)>;                                 \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                         \
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER as_beh, Class, func_t>(); \
        this->func_name(to_asGENFUNC_t(fp<Function>, call_conv<conv>));                  \
        return *this;                                                                    \
    }                                                                                    \
    template <auto Function>                                                             \
    basic_value_class& func_name(fp_wrapper_t<Function>)                                 \
    {                                                                                    \
        if constexpr(ForceGeneric)                                                       \
            this->func_name(use_generic, fp<Function>);                                  \
        else                                                                             \
            this->func_name(Function);                                                   \
        return *this;                                                                    \
    }

    // For garbage collected value type
    // See: https://www.angelcode.com/angelscript/sdk/docs/manual/doc_gc_object.html#doc_reg_gcref_value

    ASBIND20_VALUE_CLASS_BEH(enum_refs, asBEHAVE_ENUMREFS, "void f(int&in)")
    ASBIND20_VALUE_CLASS_BEH(release_refs, asBEHAVE_RELEASEREFS, "void f(int&in)")

#undef ASBIND20_VALUE_CLASS_BEH

    ASBIND20_CLASS_TEMPLATE_CALLBACK(basic_value_class)

    ASBIND20_CLASS_METHOD(basic_value_class)
    ASBIND20_CLASS_METHOD_AUXILIARY(basic_value_class)
    ASBIND20_CLASS_WRAPPED_METHOD(basic_value_class)
    ASBIND20_CLASS_WRAPPED_METHOD_AUXILIARY(basic_value_class)
    ASBIND20_CLASS_WRAPPED_LAMBDA_METHOD(basic_value_class)
    ASBIND20_CLASS_WRAPPED_VAR_TYPE_METHOD(basic_value_class)
    ASBIND20_CLASS_WRAPPED_VAR_TYPE_METHOD_AUXILIARY(basic_value_class)
    ASBIND20_CLASS_WRAPPED_LAMBDA_VAR_TYPE_METHOD(basic_value_class)

    template <wrappers::auto_register<basic_value_class> AutoRegister>
    basic_value_class& use(AutoRegister&& ar)
    {
        ar(*this);

        return *this;
    }

    basic_value_class& property(std::string_view decl, std::size_t off)
    {
        this->property_impl(decl, off);

        return *this;
    }

    template <typename MemberPointer>
    requires(std::is_member_object_pointer_v<MemberPointer>)
    basic_value_class& property(std::string_view decl, MemberPointer mp)
    {
        this->template property_impl<MemberPointer>(decl, mp);

        return *this;
    }

    basic_value_class& funcdef(std::string_view decl)
    {
        this->member_funcdef_impl(decl);

        return *this;
    }

    basic_value_class& as_string(
        AS_NAMESPACE_QUALIFIER asIStringFactory* str_factory
    )
    {
        this->as_string_impl(m_name.c_str(), str_factory);
        return *this;
    }
};

template <typename Class, bool ForceGeneric = false>
using value_class = basic_value_class<Class, false, ForceGeneric>;

template <typename Class, bool ForceGeneric = false>
using template_value_class = basic_value_class<Class, true, ForceGeneric>;

template <typename Class, bool Template = false, bool ForceGeneric = false>
class basic_ref_class : public class_register_helper_base<ForceGeneric>
{
    using my_base = class_register_helper_base<ForceGeneric>;

    using my_base::m_engine;
    using my_base::m_name;

public:
    using class_type = Class;

    basic_ref_class(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        std::string name,
        AS_NAMESPACE_QUALIFIER asQWORD flags = 0
    )
        : my_base(engine, std::move(name))
    {
        flags |= AS_NAMESPACE_QUALIFIER asOBJ_REF;
        assert(!(flags & (AS_NAMESPACE_QUALIFIER asOBJ_VALUE)));

        if constexpr(!Template)
        {
            assert(!(flags & (AS_NAMESPACE_QUALIFIER asOBJ_TEMPLATE)));
        }
        else
        {
            flags |= AS_NAMESPACE_QUALIFIER asOBJ_TEMPLATE;
        }

        this->template register_object_type<Class>(flags);
    }

private:
    template <typename Method>
    static constexpr auto method_callconv() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        if constexpr(noncapturing_lambda<Method>)
            return detail::deduce_lambda_callconv<Class, Method>();
        else
            return detail::deduce_method_callconv<Class, Method>();
    }

    template <auto Method>
    static constexpr auto method_callconv() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        return method_callconv<std::decay_t<decltype(Method)>>();
    }

    template <typename Method, typename Auxiliary>
    static consteval auto method_callconv_aux() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        return detail::deduce_method_callconv_aux<Class, Method, Auxiliary>();
    }

    template <auto Method, typename Auxiliary>
    static consteval auto method_callconv_aux() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        return method_callconv_aux<std::decay_t<decltype(Method)>, Auxiliary>();
    }

public:
    ASBIND20_CLASS_TEMPLATE_CALLBACK(basic_ref_class)

    ASBIND20_CLASS_METHOD(basic_ref_class)
    ASBIND20_CLASS_METHOD_AUXILIARY(basic_ref_class)
    ASBIND20_CLASS_WRAPPED_METHOD(basic_ref_class)
    ASBIND20_CLASS_WRAPPED_METHOD_AUXILIARY(basic_ref_class)
    ASBIND20_CLASS_WRAPPED_LAMBDA_METHOD(basic_ref_class)
    ASBIND20_CLASS_WRAPPED_VAR_TYPE_METHOD(basic_ref_class)
    ASBIND20_CLASS_WRAPPED_VAR_TYPE_METHOD_AUXILIARY(basic_ref_class)
    ASBIND20_CLASS_WRAPPED_LAMBDA_VAR_TYPE_METHOD(basic_ref_class)

    template <wrappers::auto_register<basic_ref_class> AutoRegister>
    basic_ref_class& use(AutoRegister&& ar)
    {
        ar(*this);

        return *this;
    }

private:
    std::string decl_factory(std::string_view params) const
    {
        if constexpr(Template)
        {
            return params.empty() ?
                       string_concat(m_name, "@f(int&in)") :
                       string_concat(m_name, "@f(int&in,", params, ')');
        }
        else
        {
            return params.empty() ?
                       string_concat(m_name, "@f()") :
                       string_concat(m_name, "@f(", params, ')');
        }
    }

    std::string decl_factory(std::string_view params, use_explicit_t) const
    {
        if constexpr(Template)
        {
            return params.empty() ?
                       string_concat(m_name, "@f(int&in)explicit") :
                       string_concat(m_name, "@f(int&in,", params, ")explicit");
        }
        else
        {
            return params.empty() ?
                       string_concat(m_name, "@f()explicit") :
                       string_concat(m_name, "@f(", params, ")explicit");
        }
    }

    template <typename... Args>
    static consteval bool check_factory_args()
    {
        if constexpr(Template)
        {
            using args_tuple = std::tuple<Args...>;
            if constexpr(std::tuple_size_v<args_tuple> >= 1)
            {
                return std::convertible_to<
                    asITypeInfo*,
                    std::tuple_element_t<0, args_tuple>>;
            }
            else
                return false;
        }
        else
            return true;
    }

public:
    template <typename Factory>
    basic_ref_class& factory_function(
        std::string_view params,
        Factory fn
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            decl_factory(params).c_str(),
            fn,
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
        );

        return *this;
    }

    template <typename Factory>
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        Factory fn
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            decl_factory(params, use_explicit).c_str(),
            fn,
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
        );

        return *this;
    }

    template <
        typename Factory,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL
    )
    basic_ref_class& factory_function(
        std::string_view params,
        Factory fn,
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            decl_factory(params).c_str(),
            fn,
            call_conv<CallConv>
        );

        return *this;
    }

    template <
        typename Factory,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL
    )
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        Factory fn,
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            decl_factory(params, use_explicit).c_str(),
            fn,
            call_conv<CallConv>
        );

        return *this;
    }

    template <
        typename Factory,
        typename Auxiliary,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    basic_ref_class& factory_function(
        std::string_view params,
        Factory fn,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            decl_factory(params).c_str(),
            fn,
            call_conv<CallConv>,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        typename Factory,
        typename Auxiliary,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        Factory fn,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            decl_factory(params, use_explicit).c_str(),
            fn,
            call_conv<CallConv>,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        typename Factory,
        typename Auxiliary>
    basic_ref_class& factory_function(
        std::string_view params,
        Factory fn,
        auxiliary_wrapper<Auxiliary> aux
    ) requires(!ForceGeneric)
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv_aux<AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY, Class, std::decay_t<Factory>, Auxiliary>();

        this->factory_function(
            params,
            fn,
            aux,
            call_conv<conv>
        );

        return *this;
    }

    template <
        typename Factory,
        typename Auxiliary>
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        Factory fn,
        auxiliary_wrapper<Auxiliary> aux
    ) requires(!ForceGeneric)
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv_aux<AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY, Class, std::decay_t<Factory>, Auxiliary>();

        this->factory_function(
            params,
            use_explicit,
            fn,
            aux,
            call_conv<conv>
        );

        return *this;
    }

    basic_ref_class& factory_function(
        std::string_view params,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            decl_factory(params).c_str(),
            gfn,
            generic_call_conv
        );

        return *this;
    }

    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            decl_factory(params, use_explicit).c_str(),
            gfn,
            generic_call_conv
        );

        return *this;
    }

    template <typename Auxiliary>
    basic_ref_class& factory_function(
        std::string_view params,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            decl_factory(params).c_str(),
            gfn,
            generic_call_conv,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <typename Auxiliary>
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            decl_factory(params, use_explicit).c_str(),
            gfn,
            generic_call_conv,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <auto Factory>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        fp_wrapper_t<Factory>,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_CDECL> = {}
    )
    {
        factory_function(
            params,
            to_asGENFUNC_t(fp<Factory>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>),
            generic_call_conv
        );

        return *this;
    }

    template <auto Factory>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        fp_wrapper_t<Factory>,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_CDECL> = {}
    )
    {
        this->factory_function(
            params,
            use_explicit,
            to_asGENFUNC_t(fp<Factory>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>),
            generic_call_conv
        );

        return *this;
    }

    template <
        auto Factory,
        typename Auxiliary,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        fp_wrapper_t<Factory>,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<CallConv>
    )
    {
        factory_function(
            params,
            wrappers::factory_function_auxiliary<Factory, Template, CallConv>::generate(generic_call_conv),
            aux,
            generic_call_conv
        );

        return *this;
    }

    template <
        auto Factory,
        typename Auxiliary,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        fp_wrapper_t<Factory>,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<CallConv>
    )
    {
        factory_function(
            params,
            use_explicit,
            wrappers::factory_function_auxiliary<Factory, Template, CallConv>::generate(generic_call_conv),
            aux,
            generic_call_conv
        );

        return *this;
    }

    template <auto Factory>
    basic_ref_class& factory_function(
        std::string_view params,
        fp_wrapper_t<Factory>
    )
    {
        if constexpr(ForceGeneric)
            factory_function(use_generic, params, fp<Factory>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);
        else
            factory_function(params, Factory, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);

        return *this;
    }

    template <auto Factory>
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        fp_wrapper_t<Factory>
    )
    {
        if constexpr(ForceGeneric)
            factory_function(use_generic, params, use_explicit, fp<Factory>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);
        else
            factory_function(params, use_explicit, Factory, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);

        return *this;
    }

    template <auto Factory, asECallConvTypes CallConv>
    requires(CallConv == asCALL_CDECL)
    basic_ref_class& factory_function(
        std::string_view params,
        fp_wrapper_t<Factory>,
        call_conv_t<CallConv>
    )
    {
        if constexpr(ForceGeneric)
            factory_function(use_generic, params, fp<Factory>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);
        else
            factory_function(params, Factory, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);

        return *this;
    }

    template <auto Factory, asECallConvTypes CallConv>
    requires(CallConv == asCALL_CDECL)
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        fp_wrapper_t<Factory>,
        call_conv_t<CallConv>
    )
    {
        if constexpr(ForceGeneric)
            factory_function(use_generic, params, use_explicit, fp<Factory>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);
        else
            factory_function(params, use_explicit, Factory, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);

        return *this;
    }

    template <
        auto Factory,
        typename Auxiliary,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    basic_ref_class& factory_function(
        std::string_view params,
        fp_wrapper_t<Factory>,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<CallConv>
    )
    {
        if constexpr(ForceGeneric)
            factory_function(use_generic, params, fp<Factory>, aux, call_conv<CallConv>);
        else
            factory_function(params, Factory, aux, call_conv<CallConv>);

        return *this;
    }

    template <
        auto Factory,
        typename Auxiliary,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        fp_wrapper_t<Factory>,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<CallConv>
    )
    {
        if constexpr(ForceGeneric)
            factory_function(use_generic, params, use_explicit, fp<Factory>, aux, call_conv<CallConv>);
        else
            factory_function(params, use_explicit, Factory, aux, call_conv<CallConv>);

        return *this;
    }

    template <
        auto Factory,
        typename Auxiliary>
    basic_ref_class& factory_function(
        std::string_view params,
        fp_wrapper_t<Factory>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv_aux<AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY, Class, std::decay_t<decltype(Factory)>, Auxiliary>();

        if constexpr(ForceGeneric)
            factory_function(use_generic, params, fp<Factory>, aux, call_conv<conv>);
        else
            factory_function(params, Factory, aux, call_conv<conv>);

        return *this;
    }

    template <
        auto Factory,
        typename Auxiliary>
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        fp_wrapper_t<Factory>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv_aux<AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY, Class, std::decay_t<decltype(Factory)>, Auxiliary>();

        if constexpr(ForceGeneric)
            factory_function(use_generic, params, use_explicit, fp<Factory>, aux, call_conv<conv>);
        else
            factory_function(params, use_explicit, Factory, aux, call_conv<conv>);

        return *this;
    }

    template <policies::factory_policy Policy = void>
    basic_ref_class& default_factory(use_generic_t, use_policy_t<Policy> = {})
    {
        this->factory<>(use_generic, "", use_policy<Policy>);

        return *this;
    }

    template <policies::factory_policy Policy = void>
    basic_ref_class& default_factory(use_policy_t<Policy> = {})
    {
        if constexpr(ForceGeneric)
        {
            default_factory(use_generic, use_policy<Policy>);
        }
        else
        {
            this->factory<>("", use_policy<Policy>);
        }

        return *this;
    }

private:
    template <typename... Args, typename Policy>
    void factory_impl_generic(
        std::string_view params, use_policy_t<Policy>, bool explicit_
    )
    {
        AS_NAMESPACE_QUALIFIER asGENFUNC_t wrapper =
            wrappers::factory<Class, Template, Policy, Args...>::generate(generic_call_conv);

        void* aux = nullptr;
        if constexpr(std::same_as<Policy, policies::notify_gc> && !Template)
            aux = m_engine->GetTypeInfoById(this->get_type_id());

        if(explicit_)
        {
            factory_function(
                params,
                use_explicit,
                wrapper,
                auxiliary(aux),
                generic_call_conv
            );
        }
        else
        {
            factory_function(
                params,
                wrapper,
                auxiliary(aux),
                generic_call_conv
            );
        }
    }

    template <typename... Args, typename Policy>
    void factory_impl_native(
        std::string_view params, use_policy_t<Policy>, bool explicit_
    ) requires(!ForceGeneric)
    {
        if constexpr(std::same_as<Policy, policies::notify_gc> && !Template)
        {
            auto wrapper =
                wrappers::factory<Class, Template, Policy, Args...>::generate(call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>);

            AS_NAMESPACE_QUALIFIER asITypeInfo* ti = m_engine->GetTypeInfoById(this->get_type_id());

            if(explicit_)
            {
                factory_function(
                    params,
                    use_explicit,
                    wrapper,
                    auxiliary(ti),
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
                );
            }
            else
            {
                factory_function(
                    params,
                    wrapper,
                    auxiliary(ti),
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
                );
            }
        }
        else
        {
            auto wrapper =
                wrappers::factory<Class, Template, Policy, Args...>::generate(call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);

            if(explicit_)
            {
                factory_function(
                    params,
                    use_explicit,
                    wrapper,
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                );
            }
            else
            {
                factory_function(
                    params,
                    wrapper,
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                );
            }
        }
    }

public:
    template <typename... Args, policies::factory_policy Policy = void>
    basic_ref_class& factory(
        use_generic_t, std::string_view params, use_policy_t<Policy> = {}
    )
    {
        this->factory_impl_generic<Args...>(params, use_policy<Policy>, false);

        return *this;
    }

    template <typename... Args, policies::factory_policy Policy = void>
    basic_ref_class& factory(
        use_generic_t, std::string_view params, use_explicit_t, use_policy_t<Policy> = {}
    )
    {
        this->factory_impl_generic<Args...>(params, use_policy<Policy>, true);

        return *this;
    }

    template <typename... Args, policies::factory_policy Policy = void>
    basic_ref_class& factory(
        std::string_view params, use_policy_t<Policy> = {}
    )
    {
        if constexpr(ForceGeneric)
        {
            factory<Args...>(use_generic, params, use_policy<Policy>);
        }
        else
        {
            this->factory_impl_native<Args...>(params, use_policy<Policy>, false);
        }

        return *this;
    }

    template <typename... Args, policies::factory_policy Policy = void>
    basic_ref_class& factory(
        std::string_view params, use_explicit_t, use_policy_t<Policy> = {}
    )
    {
        if constexpr(ForceGeneric)
        {
            factory<Args...>(use_generic, params, use_explicit, use_policy<Policy>);
        }
        else
        {
            this->factory_impl_native<Args...>(params, use_policy<Policy>, true);
        }

        return *this;
    }

private:
    std::string decl_list_factory(std::string_view pattern) const
    {
        if constexpr(Template)
            return string_concat(m_name, "@f(int&in,int&in){", pattern, "}");
        else
            return string_concat(m_name, "@f(int&in){", pattern, "}");
    }

public:
    template <
        native_function ListFactory,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL
    )
    basic_ref_class& list_factory_function(
        std::string_view pattern,
        ListFactory ctor,
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY,
            decl_list_factory(pattern).c_str(),
            ctor,
            call_conv<CallConv>
        );

        return *this;
    }

    template <native_function ListFactory>
    basic_ref_class& list_factory_function(
        std::string_view pattern,
        ListFactory ctor
    ) requires(!ForceGeneric)
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY, Class, std::decay_t<ListFactory>>();
        this->list_factory_function(
            pattern,
            ctor,
            call_conv<conv>
        );

        return *this;
    }

    basic_ref_class& list_factory_function(
        std::string_view pattern,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY,
            decl_list_factory(pattern).c_str(),
            gfn,
            generic_call_conv
        );

        return *this;
    }

    template <
        native_function ListFactory,
        typename Auxiliary,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    basic_ref_class& list_factory_function(
        std::string_view pattern,
        ListFactory ctor,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<CallConv>
    ) requires(!ForceGeneric)
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY,
            decl_list_factory(pattern).c_str(),
            ctor,
            call_conv<CallConv>,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <typename Auxiliary>
    basic_ref_class& list_factory_function(
        std::string_view pattern,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t ctor,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> = {}
    )
    {
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY,
            decl_list_factory(pattern).c_str(),
            ctor,
            generic_call_conv,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        native_function ListFactory,
        typename Auxiliary>
    basic_ref_class& list_factory_function(
        std::string_view pattern,
        ListFactory ctor,
        auxiliary_wrapper<Auxiliary> aux
    ) requires(!ForceGeneric)
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv_aux<AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY, Class, std::decay_t<ListFactory>, Auxiliary>();

        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY,
            decl_list_factory(pattern).c_str(),
            ctor,
            call_conv<conv>,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        auto ListFactory,
        typename Auxiliary,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    basic_ref_class& list_factory_function(
        use_generic_t,
        std::string_view pattern,
        fp_wrapper_t<ListFactory>,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<CallConv>
    )
    {
        AS_NAMESPACE_QUALIFIER asGENFUNC_t wrapper = nullptr;

        using traits = function_traits<std::decay_t<decltype(ListFactory)>>;
        if constexpr(Template)
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL)
            {
                wrapper = +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    Class* ptr = std::invoke(
                        ListFactory,
                        get_generic_auxiliary<Auxiliary>(gen),
                        *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                        *(void**)gen->GetAddressOfArg(1)
                    );
                    gen->SetReturnAddress(ptr);
                };
            }
            else if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            {
                using first_arg_type = typename traits::first_arg_type;
                wrapper = +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    Class* ptr = std::invoke(
                        ListFactory,
                        get_generic_auxiliary<first_arg_type>(gen),
                        *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                        *(void**)gen->GetAddressOfArg(1)
                    );
                    gen->SetReturnAddress(ptr);
                };
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                using last_arg_type = typename traits::last_arg_type;
                wrapper = +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    Class* ptr = std::invoke(
                        ListFactory,
                        *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                        *(void**)gen->GetAddressOfArg(1),
                        get_generic_auxiliary<last_arg_type>(gen)
                    );
                    gen->SetReturnAddress(ptr);
                };
            }
        }
        else // !Template
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL)
            {
                wrapper = +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    Class* ptr = std::invoke(
                        ListFactory,
                        get_generic_auxiliary<Auxiliary>(gen),
                        *(void**)gen->GetAddressOfArg(0)
                    );
                    gen->SetReturnAddress(ptr);
                };
            }
            else if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            {
                using first_arg_type = typename traits::first_arg_type;
                wrapper = +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    Class* ptr = std::invoke(
                        ListFactory,
                        get_generic_auxiliary<first_arg_type>(gen),
                        *(void**)gen->GetAddressOfArg(0)
                    );
                    gen->SetReturnAddress(ptr);
                };
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                using last_arg_type = typename traits::last_arg_type;
                wrapper = +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    Class* ptr = std::invoke(
                        ListFactory,
                        *(void**)gen->GetAddressOfArg(0),
                        get_generic_auxiliary<last_arg_type>(gen)
                    );
                    gen->SetReturnAddress(ptr);
                };
            }
        }

        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY,
            decl_list_factory(pattern).c_str(),
            wrapper,
            generic_call_conv,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        auto ListFactory,
        typename Auxiliary,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    basic_ref_class& list_factory_function(
        std::string_view pattern,
        fp_wrapper_t<ListFactory>,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<CallConv>
    )
    {
        if(ForceGeneric)
        {
            this->list_factory_function(
                use_generic,
                pattern,
                fp<ListFactory>,
                aux,
                call_conv<CallConv>
            );
        }
        else
        {
            this->list_factory_function(
                pattern,
                ListFactory,
                aux,
                call_conv<CallConv>
            );
        }

        return *this;
    }

    template <
        auto ListFactory,
        typename Auxiliary>
    basic_ref_class& list_factory_function(
        use_generic_t,
        std::string_view pattern,
        fp_wrapper_t<ListFactory>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv_aux<AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY, Class, std::decay_t<decltype(ListFactory)>, Auxiliary>();

        this->list_factory_function(
            use_generic,
            pattern,
            fp<ListFactory>,
            aux,
            call_conv<conv>
        );

        return *this;
    }

    template <
        auto ListFactory,
        typename Auxiliary>
    basic_ref_class& list_factory_function(
        std::string_view pattern,
        fp_wrapper_t<ListFactory>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv_aux<AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY, Class, std::decay_t<decltype(ListFactory)>, Auxiliary>();

        if constexpr(ForceGeneric)
        {
            this->list_factory_function(
                use_generic,
                pattern,
                fp<ListFactory>,
                aux,
                call_conv<conv>
            );
        }
        else
        {
            this->list_factory_function(
                pattern,
                fp<ListFactory>,
                aux,
                call_conv<conv>
            );
        }

        return *this;
    }

    template <
        typename ListElementType = void,
        policies::initialization_list_policy ListPolicy,
        policies::factory_policy FactoryPolicy>
    basic_ref_class& list_factory(
        use_generic_t,
        std::string_view pattern,
        use_policy_t<ListPolicy, FactoryPolicy>
    )
    {
        AS_NAMESPACE_QUALIFIER asGENFUNC_t wrapper =
            wrappers::list_factory<Class, Template, ListElementType, ListPolicy, FactoryPolicy>::generate(generic_call_conv);

        void* aux = nullptr;
        if constexpr(std::same_as<FactoryPolicy, policies::notify_gc> && !Template)
            aux = m_engine->GetTypeInfoById(this->get_type_id());

        this->list_factory_function(
            pattern,
            wrapper,
            auxiliary(aux),
            generic_call_conv
        );

        return *this;
    }

    template <
        typename ListElementType = void,
        policies::initialization_list_policy ListPolicy,
        policies::factory_policy FactoryPolicy>
    basic_ref_class& list_factory(
        std::string_view pattern,
        use_policy_t<ListPolicy, FactoryPolicy>
    )
    {
        if constexpr(ForceGeneric)
        {
            list_factory<ListElementType>(use_generic, pattern, use_policy<ListPolicy, FactoryPolicy>);
        }
        else
        {
            if constexpr(std::same_as<FactoryPolicy, policies::notify_gc> && !Template)
            {
                auto wrapper =
                    wrappers::list_factory<Class, Template, ListElementType, ListPolicy, FactoryPolicy>::generate(
                        call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
                    );

                AS_NAMESPACE_QUALIFIER asITypeInfo* ti = m_engine->GetTypeInfoById(this->get_type_id());

                this->list_factory_function(
                    pattern,
                    wrapper,
                    auxiliary(ti),
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
                );
            }
            else
            {
                auto wrapper =
                    wrappers::list_factory<Class, Template, ListElementType, ListPolicy, FactoryPolicy>::generate(
                        call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                    );

                list_factory_function(
                    pattern,
                    wrapper,
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                );
            }
        }

        return *this;
    }

    template <
        typename ListElementType = void,
        policies::initialization_list_policy Policy = void>
    basic_ref_class& list_factory(
        use_generic_t,
        std::string_view pattern,
        use_policy_t<Policy> = {}
    )
    {
        AS_NAMESPACE_QUALIFIER asGENFUNC_t wrapper =
            wrappers::list_factory<Class, Template, ListElementType, Policy>::generate(generic_call_conv);

        list_factory_function(
            pattern,
            wrapper,
            generic_call_conv
        );

        return *this;
    }

    template <
        typename ListElementType = void,
        policies::initialization_list_policy Policy = void>
    basic_ref_class& list_factory(
        std::string_view pattern,
        use_policy_t<Policy> = {}
    )
    {
        if constexpr(ForceGeneric)
        {
            list_factory<ListElementType>(use_generic, pattern, use_policy<Policy>);
        }
        else
        {
            auto wrapper =
                wrappers::list_factory<Class, Template, ListElementType, Policy>::generate(
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                );

            list_factory_function(
                pattern,
                wrapper,
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
            );
        }

        return *this;
    }

#define ASBIND20_REFERENCE_CLASS_OP(op_name)               \
    basic_ref_class& op_name(use_generic_t)                \
    {                                                      \
        this->template op_name##_impl_generic<Class>();    \
        return *this;                                      \
    }                                                      \
    basic_ref_class& op_name()                             \
    {                                                      \
        if constexpr(ForceGeneric)                         \
            this->op_name(use_generic);                    \
        else                                               \
            this->template op_name##_impl_native<Class>(); \
        return *this;                                      \
    }

    ASBIND20_REFERENCE_CLASS_OP(opAssign);
    ASBIND20_REFERENCE_CLASS_OP(opAddAssign);
    ASBIND20_REFERENCE_CLASS_OP(opSubAssign);
    ASBIND20_REFERENCE_CLASS_OP(opMulAssign);
    ASBIND20_REFERENCE_CLASS_OP(opDivAssign);

    ASBIND20_REFERENCE_CLASS_OP(opEquals);
    ASBIND20_REFERENCE_CLASS_OP(opCmp);

    ASBIND20_REFERENCE_CLASS_OP(opPreInc);
    ASBIND20_REFERENCE_CLASS_OP(opPreDec);

#undef ASBIND20_REFERENCE_CLASS_OP

    template <typename To>
    basic_ref_class& opConv(use_generic_t, std::string_view to_decl)
    {
        this->template opConv_impl_generic<Class, To>(to_decl, false);

        return *this;
    }

    template <typename To>
    basic_ref_class& opConv(std::string_view to_decl)
    {
        if constexpr(ForceGeneric)
            opConv<To>(use_generic, to_decl);
        else
        {
            this->template opConv_impl_native<Class, To>(to_decl, false);
        }

        return *this;
    }

    template <typename To>
    basic_ref_class& opImplConv(use_generic_t, std::string_view to_decl)
    {
        this->template opConv_impl_generic<Class, To>(to_decl, true);

        return *this;
    }

    template <typename To>
    basic_ref_class& opImplConv(std::string_view to_decl)
    {
        if constexpr(ForceGeneric)
            opImplConv<To>(use_generic, to_decl);
        else
        {
            this->template opConv_impl_native<Class, To>(to_decl, true);
        }

        return *this;
    }

    template <has_static_name To>
    basic_ref_class& opConv(use_generic_t)
    {
        opConv<To>(use_generic, name_of<To>());

        return *this;
    }

    template <has_static_name To>
    basic_ref_class& opConv()
    {
        opConv<To>(name_of<To>());

        return *this;
    }

    template <has_static_name To>
    basic_ref_class& opImplConv(use_generic_t)
    {
        opImplConv<To>(use_generic, name_of<To>());

        return *this;
    }

    template <has_static_name To>
    basic_ref_class& opImplConv()
    {
        opImplConv<To>(name_of<To>());

        return *this;
    }

#define ASBIND20_REFERENCE_CLASS_BEH(func_name, as_beh, as_decl)                         \
    template <native_function Fn>                                                        \
    basic_ref_class& func_name(Fn&& fn) requires(!ForceGeneric)                          \
    {                                                                                    \
        using func_t = std::decay_t<Fn>;                                                 \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                         \
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER as_beh, Class, func_t>(); \
        this->behaviour_impl(                                                            \
            AS_NAMESPACE_QUALIFIER as_beh,                                               \
            as_decl,                                                                     \
            fn,                                                                          \
            call_conv<conv>                                                              \
        );                                                                               \
        return *this;                                                                    \
    }                                                                                    \
    basic_ref_class& func_name(AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn)                   \
    {                                                                                    \
        this->behaviour_impl(                                                            \
            AS_NAMESPACE_QUALIFIER as_beh,                                               \
            as_decl,                                                                     \
            gfn,                                                                         \
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>                             \
        );                                                                               \
        return *this;                                                                    \
    }                                                                                    \
    template <auto Function>                                                             \
    basic_ref_class& func_name(use_generic_t, fp_wrapper_t<Function>)                    \
    {                                                                                    \
        using func_t = std::decay_t<decltype(Function)>;                                 \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                         \
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER as_beh, Class, func_t>(); \
        this->func_name(to_asGENFUNC_t(fp<Function>, call_conv<conv>));                  \
        return *this;                                                                    \
    }                                                                                    \
    template <auto Function>                                                             \
    basic_ref_class& func_name(fp_wrapper_t<Function>)                                   \
    {                                                                                    \
        if constexpr(ForceGeneric)                                                       \
            this->func_name(use_generic, fp<Function>);                                  \
        else                                                                             \
            this->func_name(Function);                                                   \
        return *this;                                                                    \
    }

    ASBIND20_REFERENCE_CLASS_BEH(get_weakref_flag, asBEHAVE_GET_WEAKREF_FLAG, "int&f()")
    ASBIND20_REFERENCE_CLASS_BEH(addref, asBEHAVE_ADDREF, "void f()")
    ASBIND20_REFERENCE_CLASS_BEH(release, asBEHAVE_RELEASE, "void f()")
    ASBIND20_REFERENCE_CLASS_BEH(get_refcount, asBEHAVE_GETREFCOUNT, "int f()")
    ASBIND20_REFERENCE_CLASS_BEH(set_gc_flag, asBEHAVE_SETGCFLAG, "void f()")
    ASBIND20_REFERENCE_CLASS_BEH(get_gc_flag, asBEHAVE_GETGCFLAG, "bool f()")
    ASBIND20_REFERENCE_CLASS_BEH(enum_refs, asBEHAVE_ENUMREFS, "void f(int&in)")
    ASBIND20_REFERENCE_CLASS_BEH(release_refs, asBEHAVE_RELEASEREFS, "void f(int&in)")

#undef ASBIND20_REFERENCE_CLASS_BEH

    basic_ref_class& property(std::string_view decl, std::size_t off)
    {
        this->property_impl(decl, off);

        return *this;
    }

    template <typename MemberPointer>
    requires(std::is_member_object_pointer_v<MemberPointer>)
    basic_ref_class& property(std::string_view decl, MemberPointer mp)
    {
        this->template property_impl<MemberPointer>(decl, mp);

        return *this;
    }

    basic_ref_class& funcdef(std::string_view decl)
    {
        this->member_funcdef_impl(decl);

        return *this;
    }

    basic_ref_class& as_string(
        AS_NAMESPACE_QUALIFIER asIStringFactory* str_factory
    )
    {
        this->as_string_impl(m_name.c_str(), str_factory);
        return *this;
    }

    basic_ref_class& as_array() requires(Template)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterDefaultArrayType(m_name.c_str());
        assert(r >= 0);

        return *this;
    }
};

#undef ASBIND20_CLASS_TEMPLATE_CALLBACK

#undef ASBIND20_CLASS_METHOD
#undef ASBIND20_CLASS_METHOD_AUXILIARY
#undef ASBIND20_CLASS_WRAPPED_METHOD
#undef ASBIND20_CLASS_WRAPPED_METHOD_AUXILIARY
#undef ASBIND20_CLASS_WRAPPED_LAMBDA_METHOD
#undef ASBIND20_CLASS_WRAPPED_VAR_TYPE_METHOD
#undef ASBIND20_CLASS_WRAPPED_VAR_TYPE_METHOD_AUXILIARY
#undef ASBIND20_CLASS_WRAPPED_LAMBDA_VAR_TYPE_METHOD

template <typename Class, bool UseGeneric = false>
using ref_class = basic_ref_class<Class, false, UseGeneric>;

template <typename Class, bool ForceGeneric = false>
using template_ref_class = basic_ref_class<Class, true, ForceGeneric>;

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
