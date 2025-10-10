/**
 * @file bind/class.hpp
 * @author HenryAWE
 * @brief Binding generator for classes
 */

#ifndef ASBIND20_BIND_CLASS_HPP
#define ASBIND20_BIND_CLASS_HPP

#pragma once

#include <memory>
#include <concepts>
#include "common.hpp"
#include "wrappers.hpp"
#include "../policies.hpp"
#include "../decl.hpp"

namespace asbind20
{
namespace detail
{
    // Wrapper generators for special functions like constructor

    /**
     * @brief Destroy the constructed object if there is any script exception,
     *        because AngelScript won't call the destructor under this situation.
     *
     * @tparam Class Class type
     * @tparam ScriptNoexcept True if the user can guarantee that the constructor won't cause any script exception
     */
    template <typename Class, bool ScriptNoexcept = false>
    struct ctor_ex_guard
    {
        // Destroy the constructed object if there is any script exception,
        // because AngelScript won't call the destructor under this situation.
        static void destroy_if_ex(Class* obj)
        {
            constexpr bool no_guard =
                ScriptNoexcept ||
                std::is_trivially_destructible_v<Class>;

            if constexpr(!no_guard)
            {
                if(has_script_exception()) [[unlikely]]
                    std::destroy_at(obj);
            }
        }
    };

    // Specialization for C-array
    template <typename ElemType, std::size_t Size, bool ScriptNoexcept>
    struct ctor_ex_guard<ElemType[Size], ScriptNoexcept>
    {
        static void destroy_if_ex(ElemType* arr)
        {
            constexpr bool no_guard =
                ScriptNoexcept ||
                std::is_trivially_destructible_v<ElemType>;

            if constexpr(!no_guard)
            {
                if(has_script_exception()) [[unlikely]]
                    std::destroy_n(arr, Size);
            }
        }
    };

    template <typename Class, bool Template, typename... Args>
    class constructor;

    // Constructor for non-templated classes
    template <typename Class, typename... Args>
    requires(!std::is_array_v<Class>)
    class constructor<Class, false, Args...>
    {
        using ex_guard = ctor_ex_guard<Class>;

        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            using args_tuple = std::tuple<Args...>;

            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                void* mem = gen->GetObject();
                new(mem) Class(
                    get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                        gen, (AS_NAMESPACE_QUALIFIER asUINT)Is
                    )...
                );

                ex_guard::destroy_if_ex(static_cast<Class*>(mem));
            }(std::index_sequence_for<Args...>());
        }

        static void impl_objlast(Args... args, void* mem)
        {
            new(mem) Class(std::forward<Args>(args)...);

            ex_guard::destroy_if_ex(static_cast<Class*>(mem));
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL_OBJLAST
                return &impl_objlast;
        }
    };

    // Constructor wrapper for templated classes
    template <typename Class, typename... Args>
    requires(!std::is_array_v<Class>)
    class constructor<Class, true, Args...>
    {
        using ex_guard = ctor_ex_guard<Class>;

        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                using args_tuple = std::tuple<Args...>;

                void* mem = gen->GetObject();
                new(mem) Class(
                    *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                    get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                        gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1
                    )...
                );

                ex_guard::destroy_if_ex(static_cast<Class*>(mem));
            }(std::index_sequence_for<Args...>());
        }

        static void impl_objlast(
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti, Args... args, void* mem
        )
        {
            new(mem) Class(ti, std::forward<Args>(args)...);

            ex_guard::destroy_if_ex(static_cast<Class*>(mem));
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL_OBJLAST
                return &impl_objlast;
        }
    };

    template <typename ElemType, std::size_t Size, bool Template>
    class constructor<ElemType[Size], Template>
    {
        using ex_guard = ctor_ex_guard<ElemType[Size]>;

        static_assert(!Template, "Default constructor of C-array is invalid for template");

        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            auto* ptr = static_cast<ElemType*>(gen->GetObject());

            std::uninitialized_default_construct_n(
                ptr, Size
            );
            ex_guard::destroy_if_ex(ptr);
        }

        static void impl_objlast(void* mem)
        {
            auto* ptr = static_cast<ElemType*>(mem);

            std::uninitialized_default_construct_n(
                ptr, Size
            );
            ex_guard::destroy_if_ex(ptr);
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL_OBJLAST
                return &impl_objlast;
        }
    };

    template <typename Class, typename Arg>
    class arr_copy_constructor;

    template <typename ElemType, std::size_t Size, typename Arg>
    class arr_copy_constructor<ElemType[Size], Arg>
    {
        using ex_guard = ctor_ex_guard<ElemType[Size]>;

    public:
        static_assert(std::is_reference_v<Arg>);

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    auto* ptr = static_cast<ElemType*>(gen->GetObject());

                    // Decay to pointer
                    std::decay_t<Arg> src = get_generic_arg<Arg>(gen, 0);

                    std::uninitialized_copy_n(src, Size, ptr);
                    ex_guard::destroy_if_ex(ptr);
                };
            }
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                return +[](Arg arg, void* mem) -> void
                {
                    auto* ptr = static_cast<ElemType*>(mem);
                    // Decay to pointer
                    std::decay_t<Arg> src = arg;

                    std::uninitialized_copy_n(src, Size, ptr);
                    ex_guard::destroy_if_ex(ptr);
                };
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType = void,
        policies::initialization_list_policy Policy = void>
    class list_constructor;

    template <
        typename Class,
        bool Template,
        typename ListElementType>
    class list_constructor<Class, Template, ListElementType, void> // Implementation for default policy
    {
        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            if constexpr(Template)
            {
                void* mem = gen->GetObject();
                new(mem) Class(
                    *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                    *(ListElementType**)gen->GetAddressOfArg(1)
                );
            }
            else
            {
                void* mem = gen->GetObject();
                new(mem) Class(*(ListElementType**)gen->GetAddressOfArg(0));
            }
        }

        static void impl_objlast_template(
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti, ListElementType* list_buf, void* mem
        )
        {
            new(mem) Class(ti, list_buf);
        }

        static void impl_objlast(
            ListElementType* list_buf, void* mem
        )
        {
            new(mem) Class(list_buf);
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                if constexpr(Template)
                    return &impl_objlast_template;
                else
                    return &impl_objlast;
            }
        }
    };

    // Construct from a proxy of script initialization list
    template <
        typename Class,
        bool Template,
        typename ListElementType>
    class list_constructor<Class, Template, ListElementType, policies::repeat_list_proxy>
    {
        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            if constexpr(Template)
            {
                void* mem = gen->GetObject();
                new(mem) Class(
                    *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                    script_init_list_repeat(gen, 1)
                );
            }
            else
            {
                void* mem = gen->GetObject();
                new(mem) Class(script_init_list_repeat(gen));
            }
        }

        static void impl_objlast_template(
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti, void* list_buf, void* mem
        )
        {
            new(mem) Class(ti, script_init_list_repeat(list_buf));
        }

        static void impl_objlast(
            void* list_buf, void* mem
        )
        {
            new(mem) Class(script_init_list_repeat(list_buf));
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL_OBJLAST
            {
                if constexpr(Template)
                    return &impl_objlast_template;
                else
                    return &impl_objlast;
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType,
        std::size_t Size>
    class list_constructor<Class, Template, ListElementType, policies::apply_to<Size>>
    {
        static void apply_helper(void* mem, ListElementType* list_buf)
        {
            [&]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                new(mem) Class(list_buf[Is]...);
            }(std::make_index_sequence<Size>());
        }

        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            apply_helper(
                gen->GetObject(),
                *(ListElementType**)gen->GetAddressOfArg(0)
            );
        }

        static void impl_objlast(ListElementType* list_buf, void* mem)
        {
            apply_helper(mem, list_buf);
        }

    public:
        static_assert(!std::is_void_v<ListElementType>, "Invalid list element type");
        static_assert(!Template, "This policy is invalid for a template class");

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL_OBJLAST
                return &impl_objlast;
        }
    };

    template <typename T>
    concept repeat_list_based_policy =
        policies::initialization_list_policy<T> &&
        (std::same_as<T, policies::as_iterators> ||
         std::same_as<T, policies::pointer_and_size> ||
         std::same_as<T, policies::as_initializer_list> ||
         std::same_as<T, policies::as_span>
#ifdef ASBIND20_HAS_CONTAINERS_RANGES
         || std::same_as<T, policies::as_from_range>
#endif
        );

    template <
        typename Class,
        bool Template,
        typename ListElementType,
        repeat_list_based_policy IListPolicy>
    class list_constructor<Class, Template, ListElementType, IListPolicy>
    {
        static void from_list_helper(void* mem, script_init_list_repeat list)
        {
            if constexpr(std::same_as<IListPolicy, policies::as_iterators>)
            {
                policies::as_iterators::apply<ListElementType>(
                    [mem](auto start, auto stop)
                    {
                        new(mem) Class(start, stop);
                    },
                    list
                );
            }
            else if constexpr(std::same_as<IListPolicy, policies::pointer_and_size>)
            {
                new(mem) Class((ListElementType*)list.data(), list.size());
            }
#ifdef ASBIND20_HAS_CONTAINERS_RANGES
            else if constexpr(std::same_as<IListPolicy, policies::as_from_range>)
            {
                std::span<ListElementType> rng((ListElementType*)list.data(), list.size());
                new(mem) Class(std::from_range, rng);
            }
#endif
            else if constexpr(
                std::same_as<IListPolicy, policies::as_initializer_list> ||
                std::same_as<IListPolicy, policies::as_span>
            )
            {
                new(mem) Class(IListPolicy::template convert<ListElementType>(list));
            }
        }

        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            from_list_helper(
                gen->GetObject(),
                script_init_list_repeat(gen)
            );
        }

        static void impl_objlast(ListElementType* list_buf, void* mem)
        {
            from_list_helper(mem, script_init_list_repeat(list_buf));
        }

    public:
        static_assert(!std::is_void_v<ListElementType>, "Invalid list element type");
        static_assert(!Template, "This policy is invalid for a template class");

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL_OBJLAST
                return &impl_objlast;
        }
    };

    template <
        typename Class,
        bool Template,
        policies::factory_policy Policy,
        typename... Args>
    class factory;

    // Implementation for default no-op policy (Policy = void)
    template <
        typename Class,
        bool Template,
        typename... Args>
    class factory<Class, Template, void, Args...>
    {
        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            using args_tuple = std::tuple<Args...>;
            if constexpr(Template)
            {
                [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                {
                    Class* ptr = new Class(
                        *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                        get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1)...
                    );
                    gen->SetReturnAddress(ptr);
                }(std::index_sequence_for<Args...>());
            }
            else
            {
                [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                {
                    Class* ptr = new Class(
                        get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (AS_NAMESPACE_QUALIFIER asUINT)Is)...
                    );
                    gen->SetReturnAddress(ptr);
                }(std::index_sequence_for<Args...>());
            }
        }

        static Class* impl_cdecl_template(
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti, Args... args
        )
        {
            return new Class(ti, std::forward<Args>(args)...);
        }

        static Class* impl_cdecl(Args... args)
        {
            return new Class(std::forward<Args>(args)...);
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL
            {
                if constexpr(Template)
                    return &impl_cdecl_template;
                else
                    return &impl_cdecl;
            }
        }
    };

    // GC notifier for non-templated classes
    template <typename Class, typename... Args>
    class factory<Class, false, policies::notify_gc, Args...>
    {
        // Note: GC notifier for non-templated class expects the typeinfo is passed by auxiliary pointer,
        // a.k.a the "auxiliary(this_type)" helper

        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
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
                if(has_script_exception()) [[unlikely]]
                {
                    delete ptr;
                    return;
                }

                gen->GetEngine()->NotifyGarbageCollectorOfNewObject(ptr, ti);

                gen->SetReturnAddress(ptr);
            }(std::index_sequence_for<Args...>());
        }

        static Class* impl_objlast(Args... args, AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            Class* ptr = new Class(std::forward<Args>(args)...);
            if(has_script_exception()) [[unlikely]]
            {
                delete ptr;
                return nullptr; // placeholder
            }

            ti->GetEngine()->NotifyGarbageCollectorOfNewObject(ptr, ti);

            return ptr;
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL_OBJLAST
                return &impl_objlast;
        }
    };

    // GC notifier for templated classes
    template <typename Class, typename... Args>
    class factory<Class, true, policies::notify_gc, Args...>
    {
        // Note: Template callback may remove the asOBJ_GC flag for some instantiations,
        // so the following wrapper implementations will check the flag at runtime.

        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
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

                if(has_script_exception()) [[unlikely]]
                {
                    delete ptr;
                    return;
                }

                auto flags = ti->GetFlags();
                if(flags & AS_NAMESPACE_QUALIFIER asOBJ_GC)
                {
                    assert(ti->GetEngine() == gen->GetEngine());
                    gen->GetEngine()->NotifyGarbageCollectorOfNewObject(ptr, ti);
                }

                gen->SetReturnAddress(ptr);
            }(std::index_sequence_for<Args...>());
        }

        static Class* impl_cdecl(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, Args... args)
        {
            Class* ptr = new Class(ti, std::forward<Args>(args)...);

            if(has_script_exception()) [[unlikely]]
            {
                delete ptr;
                return nullptr;
            }

            auto flags = ti->GetFlags();
            if(flags & AS_NAMESPACE_QUALIFIER asOBJ_GC)
            {
                ti->GetEngine()->NotifyGarbageCollectorOfNewObject(ptr, ti);
            }

            return ptr;
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL
                return &impl_cdecl;
        }
    };

    template <
        policies::factory_policy FactoryPolicy,
        bool Template>
    struct notify_gc_helper
    {
        // Placeholder
        static void notify_gc_if_necessary(void* obj, AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            (void)obj;
            (void)ti;
        }
    };

    template <bool Template>
    struct notify_gc_helper<policies::notify_gc, Template>
    {
        static void notify_gc_if_necessary(void* obj, AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            if(!ti) [[unlikely]]
                return;

            // The template callback may remove the asOBJ_GC flag to optimize for certain subtypes,
            // so we need to check it again at runtime
            if constexpr(Template)
            {
                auto flags = ti->GetFlags();
                if(!(flags & AS_NAMESPACE_QUALIFIER asOBJ_GC))
                    return;
            }
            ti->GetEngine()->NotifyGarbageCollectorOfNewObject(obj, ti);
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType,
        policies::initialization_list_policy IListPolicy,
        policies::factory_policy FactoryPolicy>
    class list_factory;

    // Implementation for default initlist policy & factory policy
    template <
        typename Class,
        bool Template,
        typename ListElementType>
    class list_factory<Class, Template, ListElementType, void, void>
    {
        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            if constexpr(Template)
            {
                Class* ptr = new Class(
                    *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                    *(ListElementType**)gen->GetAddressOfArg(1)
                );
                gen->SetReturnAddress(ptr);
            }
            else
            {
                Class* ptr = new Class(
                    *(ListElementType**)gen->GetAddressOfArg(0)
                );
                gen->SetReturnAddress(ptr);
            }
        }

        static Class* impl_cdecl_template(
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti, ListElementType* list_buf
        )
        {
            return new Class(ti, list_buf);
        }

        static Class* impl_cdecl(ListElementType* list_buf)
        {
            return new Class(list_buf);
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL
            {
                if constexpr(Template)
                    return &impl_cdecl_template;
                else
                    return &impl_cdecl;
            }
        }
    };

    // Implementation for default initlist policy & notifying GC
    template <
        typename Class,
        bool Template,
        typename ListElementType>
    class list_factory<Class, Template, ListElementType, void, policies::notify_gc>
    {
        using notifier = notify_gc_helper<policies::notify_gc, Template>;

        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            if constexpr(Template)
            {
                auto* ti = *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0);

                Class* ptr = new Class(
                    ti, *(ListElementType**)gen->GetAddressOfArg(1)
                );
                notifier::notify_gc_if_necessary(ptr, ti);

                gen->SetReturnAddress(ptr);
            }
            else
            {
                Class* ptr = new Class(
                    *(ListElementType**)gen->GetAddressOfArg(0)
                );

                // Expects the typeinfo is passed by auxiliary pointer (see the helper "auxiliary(this_type)")
                auto* ti = (AS_NAMESPACE_QUALIFIER asITypeInfo*)gen->GetAuxiliary();
                assert(ti != nullptr);
                notifier::notify_gc_if_necessary(ptr, ti);

                gen->SetReturnAddress(ptr);
            }
        }

        static Class* impl_cdecl_template(
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti, ListElementType* list_buf
        )
        {
            Class* ptr = new Class(ti, list_buf);
            notifier::notify_gc_if_necessary(ptr, ti);
            return ptr;
        }

        // Works together with the helper "auxiliary(this_type)"
        static Class* impl_cdecl_objlast(
            ListElementType* list_buf, AS_NAMESPACE_QUALIFIER asITypeInfo* ti
        )
        {
            Class* ptr = new Class(list_buf);
            notifier::notify_gc_if_necessary(ptr, ti);
            return ptr;
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
            {
                static_assert(!Template, "This calling convention for factory is for templated class");
                return &impl_cdecl_objlast;
            }
            else // CallConv == asCALL_CDECL
            {
                static_assert(Template, "This calling convention for factory is for templated class");
                return &impl_cdecl_template;
            }
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType,
        std::size_t Size,
        policies::factory_policy FactoryPolicy>
    class list_factory<Class, Template, ListElementType, policies::apply_to<Size>, FactoryPolicy>
    {
        using notifier = notify_gc_helper<FactoryPolicy, Template>;

        static Class* apply_helper(ListElementType* list_buf)
        {
            return [&]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                return new Class(list_buf[Is]...);
            }(std::make_index_sequence<Size>());
        }

        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            Class* ptr = apply_helper(*(ListElementType**)gen->GetAddressOfArg(0));
            if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                auto* ti = (AS_NAMESPACE_QUALIFIER asITypeInfo*)gen->GetAuxiliary();
                assert(ti != nullptr);
                notifier::notify_gc_if_necessary(ptr, ti);
            }
            gen->SetReturnAddress(ptr);
        }

        static Class* impl_objlast(ListElementType* list_buf, AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            Class* ptr = apply_helper(list_buf);
            notifier::notify_gc_if_necessary(ptr, ti);
            return ptr;
        }

        static Class* impl_cdecl(ListElementType* list_buf)
        {
            return apply_helper(list_buf);
        }

    public:
        static_assert(!std::is_void_v<ListElementType>, "Invalid list element type");
        static_assert(!Template, "This policy is invalid for a template class");

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                static_assert(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST);
                return &impl_objlast;
            }
            else // CallConv == asCALL_CDECL
            {
                static_assert(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL);
                return &impl_cdecl;
            }
        }
    };

    // Convert to initlist proxy for non-templated classes
    template <
        typename Class,
        typename ListElementType,
        policies::factory_policy FactoryPolicy>
    class list_factory<Class, false, ListElementType, policies::repeat_list_proxy, FactoryPolicy>
    {
        using notifier = notify_gc_helper<FactoryPolicy, false>;

        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                Class* ptr = new Class(script_init_list_repeat(gen));
                // Works together with the helper "auxiliary(this_type)"
                auto* ti = (AS_NAMESPACE_QUALIFIER asITypeInfo*)gen->GetAuxiliary();
                assert(ti != nullptr);
                notifier::notify_gc_if_necessary(ptr, ti);
                gen->SetReturnAddress(ptr);
            }
            else
            {
                gen->SetReturnAddress(
                    new Class(script_init_list_repeat(gen))
                );
            }
        }

        //Works together with the helper "auxiliary(this_type)"
        static Class* impl_cdecl_objlast(
            void* list_buf, AS_NAMESPACE_QUALIFIER asITypeInfo* ti
        )
        {
            Class* ptr = new Class(script_init_list_repeat(list_buf));
            notifier::notify_gc_if_necessary(ptr, ti);
            return ptr;
        }

        static Class* impl_cdecl(void* list_buf)
        {
            return new Class(script_init_list_repeat(list_buf));
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                static_assert(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST);
                return &impl_cdecl_objlast;
            }
            else // CallConv == asCALL_CDECL
            {
                static_assert(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL);
                return &impl_cdecl;
            }
        }
    };

    // Convert to initlist proxy for templated classes
    template <
        typename Class,
        typename ListElementType,
        policies::factory_policy FactoryPolicy>
    class list_factory<Class, true, ListElementType, policies::repeat_list_proxy, FactoryPolicy>
    {
        using notifier = notify_gc_helper<FactoryPolicy, true>;

        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            auto* ti = *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0);
            Class* ptr = new Class(
                ti,
                script_init_list_repeat(gen, 1)
            );
            notifier::notify_gc_if_necessary(ptr, ti);
            gen->SetReturnAddress(ptr);
        }

        static Class* impl_cdecl(
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti, void* list_buf
        )
        {
            Class* ptr = new Class(ti, script_init_list_repeat(list_buf));
            notifier::notify_gc_if_necessary(ptr, ti);
            return ptr;
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static constexpr auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL
                return &impl_cdecl;
        }
    };

    template <
        typename Class,
        bool Template,
        typename ListElementType,
        repeat_list_based_policy IListPolicy,
        policies::factory_policy FactoryPolicy>
    class list_factory<Class, Template, ListElementType, IListPolicy, FactoryPolicy>
    {
        using notifier = notify_gc_helper<FactoryPolicy, Template>;

        static Class* from_list_helper(script_init_list_repeat list)
        {
            if constexpr(std::same_as<IListPolicy, policies::as_iterators>)
            {
                return policies::as_iterators::apply<ListElementType>(
                    [](auto start, auto stop) -> Class*
                    {
                        return new Class(start, stop);
                    },
                    list
                );
            }
            else if constexpr(std::same_as<IListPolicy, policies::pointer_and_size>)
            {
                return new Class((ListElementType*)list.data(), list.size());
            }
#ifdef ASBIND20_HAS_CONTAINERS_RANGES
            else if constexpr(std::same_as<IListPolicy, policies::as_from_range>)
            {
                std::span<ListElementType> rng((ListElementType*)list.data(), list.size());
                return new Class(std::from_range, rng);
            }
#endif
            else if constexpr(
                std::same_as<IListPolicy, policies::as_initializer_list> ||
                std::same_as<IListPolicy, policies::as_span>
            )
            {
                return new Class(IListPolicy::template convert<ListElementType>(list));
            }
        }

        static void impl_generic(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            Class* ptr = from_list_helper(script_init_list_repeat(gen));
            if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                // Expects the typeinfo is passed by auxiliary pointer (see the helper "auxiliary(this_type)")
                auto* ti = (AS_NAMESPACE_QUALIFIER asITypeInfo*)gen->GetAuxiliary();
                assert(ti != nullptr);
                notifier::notify_gc_if_necessary(ptr, ti);
            }
            gen->SetReturnAddress(ptr);
        }

        // Works together with the helper "auxiliary(this_type)"
        static Class* impl_objlast(void* list_buf, AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        {
            Class* ptr = from_list_helper(script_init_list_repeat(list_buf));
            notifier::notify_gc_if_necessary(ptr, ti);
            return ptr;
        }

        static Class* impl_cdecl(void* list_buf)
        {
            return from_list_helper(script_init_list_repeat(list_buf));
        }

    public:
        static_assert(!std::is_void_v<ListElementType>, "Invalid list element type");
        static_assert(!Template, "This policy is invalid for a template class");

        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return impl_generic;
            else if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                static_assert(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST);
                return &impl_objlast;
            }
            else // CallConv == asCALL_CDECL
            {
                static_assert(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL);
                return &impl_cdecl;
            }
        }
    };

    template <typename Class, typename To>
    class opConv
    {
    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
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
    void register_object_type(AS_NAMESPACE_QUALIFIER asQWORD flags, int size)
    {
        [[maybe_unused]]
        int r = 0;
        r = m_engine->RegisterObjectType(
            m_name.c_str(),
            size,
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
                    detail::to_asSFuncPtr(fn),
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
                    detail::to_asSFuncPtr(gfn),
                    AS_NAMESPACE_QUALIFIER asCALL_GENERIC,
                    aux
                );
            },
            decl
        );
        assert(r >= 0);
    }

    template <typename Fn, AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    void method_impl_comp(
        std::string_view decl,
        Fn&& fn,
        call_conv_t<CallConv>,
        composite_wrapper comp,
        void* aux = nullptr
    ) requires(!ForceGeneric)
    {
        [[maybe_unused]]
        int r = with_cstr(
            [this, &fn, &aux, &comp](const char* decl)
            {
                return m_engine->RegisterObjectMethod(
                    m_name.c_str(),
                    decl,
                    detail::to_asSFuncPtr(fn),
                    CallConv,
                    aux,
                    static_cast<int>(comp.get_offset()),
                    true
                );
            },
            decl
        );
        assert(r >= 0);
    }

    void method_impl_comp(
        std::string_view decl,
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>,
        composite_wrapper comp,
        void* aux = nullptr
    )
    {
        [[maybe_unused]]
        int r = with_cstr(
            [this, &gfn, &aux, &comp](const char* decl)
            {
                return m_engine->RegisterObjectMethod(
                    m_name.c_str(),
                    decl,
                    detail::to_asSFuncPtr(gfn),
                    AS_NAMESPACE_QUALIFIER asCALL_GENERIC,
                    aux,
                    static_cast<int>(comp.get_offset()),
                    true
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
            detail::to_asSFuncPtr(fn),
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

    void comp_property_impl(std::string_view decl, std::size_t off, std::size_t comp_off)
    {
        [[maybe_unused]]
        int r = with_cstr(
            [this, off, comp_off](const char* decl)
            {
                return m_engine->RegisterObjectProperty(
                    m_name.c_str(),
                    decl,
                    static_cast<int>(off),
                    static_cast<int>(comp_off),
                    true
                );
            },
            decl
        );
        assert(r >= 0);
    }

    template <typename CompMemberPointer>
    requires(std::is_member_object_pointer_v<CompMemberPointer>)
    void comp_property_impl(std::string_view decl, std::size_t off, CompMemberPointer comp_mp)
    {
        comp_property_impl(decl, off, member_offset(comp_mp));
    }

    template <typename MemberPointer>
    requires(std::is_member_object_pointer_v<MemberPointer>)
    void comp_property_impl(std::string_view decl, MemberPointer mp, std::size_t comp_off)
    {
        comp_property_impl(decl, member_offset(mp), comp_off);
    }

    template <typename CompMemberPointer, typename MemberPointer>
    requires(std::is_member_object_pointer_v<MemberPointer> && std::is_member_object_pointer_v<CompMemberPointer>)
    void comp_property_impl(std::string_view decl, MemberPointer mp, CompMemberPointer comp_mp)
    {
        comp_property_impl(decl, member_offset(mp), member_offset(comp_mp));
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
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opModAssign, %=, (m_name, "& opModAssign(const ", m_name, " &in)"), Class&, , const Class&
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
    ASBIND20_CLASS_BINARY_OP_IMPL(
        opMod, %, (m_name, " opMod(const ", m_name, " &in) const"), Class, const, const Class&
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
        auto wrapper = detail::opConv<std::add_const_t<Class>, To>::generate(
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
        auto wrapper = detail::opConv<std::add_const_t<Class>, To>::generate(generic_call_conv);

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

#define ASBIND20_CLASS_TEMPLATE_CALLBACK(register_type)                                      \
    register_type& template_callback(                                                        \
        AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn                                               \
    ) requires(Template)                                                                     \
    {                                                                                        \
        this->behaviour_impl(                                                                \
            AS_NAMESPACE_QUALIFIER asBEHAVE_TEMPLATE_CALLBACK,                               \
            "bool f(int&in,bool&out)",                                                       \
            gfn,                                                                             \
            generic_call_conv                                                                \
        );                                                                                   \
        return *this;                                                                        \
    }                                                                                        \
    template <native_function Fn>                                                            \
    register_type& template_callback(Fn&& fn) requires(Template && !ForceGeneric)            \
    {                                                                                        \
        this->behaviour_impl(                                                                \
            AS_NAMESPACE_QUALIFIER asBEHAVE_TEMPLATE_CALLBACK,                               \
            "bool f(int&in,bool&out)",                                                       \
            fn,                                                                              \
            call_conv<detail::deduce_function_callconv<std::decay_t<Fn>>()>                  \
        );                                                                                   \
        return *this;                                                                        \
    }                                                                                        \
    template <auto Callback>                                                                 \
    register_type& template_callback(use_generic_t, fp_wrapper<Callback>) requires(Template) \
    {                                                                                        \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                             \
            detail::deduce_beh_callconv<                                                     \
                AS_NAMESPACE_QUALIFIER asBEHAVE_TEMPLATE_CALLBACK,                           \
                Class,                                                                       \
                std::decay_t<decltype(Callback)>>();                                         \
        template_callback(                                                                   \
            detail::to_asGENFUNC_t(fp<Callback>, call_conv<conv>)                            \
        );                                                                                   \
        return *this;                                                                        \
    }                                                                                        \
    template <auto Callback>                                                                 \
    register_type& template_callback(fp_wrapper<Callback>) requires(Template)                \
    {                                                                                        \
        if constexpr(ForceGeneric)                                                           \
            template_callback(use_generic, fp<Callback>);                                    \
        else                                                                                 \
            template_callback(Callback);                                                     \
        return *this;                                                                        \
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
        fp_wrapper<Method>,                                                   \
        call_conv_t<CallConv>                                                 \
    )                                                                         \
    {                                                                         \
        this->method_impl(                                                    \
            decl,                                                             \
            detail::to_asGENFUNC_t(fp<Method>, call_conv<CallConv>),          \
            generic_call_conv                                                 \
        );                                                                    \
        return *this;                                                         \
    }                                                                         \
    template <auto Method>                                                    \
    register_type& method(                                                    \
        use_generic_t,                                                        \
        std::string_view decl,                                                \
        fp_wrapper<Method>                                                    \
    )                                                                         \
    {                                                                         \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =              \
            method_callconv<Method>();                                        \
        this->method_impl(                                                    \
            decl,                                                             \
            detail::to_asGENFUNC_t(fp<Method>, call_conv<conv>),              \
            generic_call_conv                                                 \
        );                                                                    \
        return *this;                                                         \
    }                                                                         \
    template <                                                                \
        auto Method,                                                          \
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                     \
    register_type& method(                                                    \
        std::string_view decl,                                                \
        fp_wrapper<Method>,                                                   \
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
        fp_wrapper<Method>                                                    \
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
        fp_wrapper<Method>,                                                        \
        auxiliary_wrapper<Auxiliary> aux,                                          \
        call_conv_t<CallConv>                                                      \
    )                                                                              \
    {                                                                              \
        this->method_impl(                                                         \
            decl,                                                                  \
            detail::to_asGENFUNC_t(fp<Method>, call_conv<CallConv>),               \
            generic_call_conv,                                                     \
            my_base::get_auxiliary_address(aux)                                    \
        );                                                                         \
        return *this;                                                              \
    }                                                                              \
    template <auto Method, typename Auxiliary>                                     \
    register_type& method(                                                         \
        use_generic_t,                                                             \
        std::string_view decl,                                                     \
        fp_wrapper<Method>,                                                        \
        auxiliary_wrapper<Auxiliary> aux                                           \
    )                                                                              \
    {                                                                              \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                   \
            method_callconv_aux<Method, Auxiliary>();                              \
        this->method_impl(                                                         \
            decl,                                                                  \
            detail::to_asGENFUNC_t(fp<Method>, call_conv<conv>),                   \
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
        fp_wrapper<Method>,                                                        \
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
        fp_wrapper<Method>,                                                        \
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
            detail::to_asGENFUNC_t(Lambda{}, call_conv<CallConv>),          \
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
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>                   \
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
        fp_wrapper<Function>,                                                                    \
        var_type_t<Is...>,                                                                       \
        call_conv_t<CallConv>                                                                    \
    )                                                                                            \
    {                                                                                            \
        this->method_impl(                                                                       \
            decl,                                                                                \
            detail::to_asGENFUNC_t(fp<Function>, call_conv<CallConv>, var_type<Is...>),          \
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
        fp_wrapper<Function>,                                                                    \
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
        fp_wrapper<Function>,                                                                    \
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
        fp_wrapper<Function>,                                                                    \
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
        fp_wrapper<Function>,                                                                         \
        var_type_t<Is...>,                                                                            \
        auxiliary_wrapper<Auxiliary> aux,                                                             \
        call_conv_t<CallConv>                                                                         \
    )                                                                                                 \
    {                                                                                                 \
        this->method_impl(                                                                            \
            decl,                                                                                     \
            detail::to_asGENFUNC_t(fp<Function>, call_conv<CallConv>, var_type<Is...>),               \
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
        fp_wrapper<Function>,                                                                         \
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
        fp_wrapper<Function>,                                                                         \
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
        fp_wrapper<Function>,                                                                         \
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
            detail::to_asGENFUNC_t(Lambda{}, call_conv<CallConv>, var_type<Is...>),          \
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

#define ASBIND20_CLASS_WRAPPED_COMPOSITE_METHOD(register_type)                                                                         \
    template <auto Fn, auto Composite>                                                                                                 \
    requires(std::is_member_function_pointer_v<decltype(Fn)>)                                                                          \
    register_type& method(                                                                                                             \
        use_generic_t,                                                                                                                 \
        std::string_view decl,                                                                                                         \
        fp_wrapper<Fn>,                                                                                                                \
        composite_wrapper_nontype<Composite>,                                                                                          \
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL> = {}                                                                       \
    )                                                                                                                                  \
    {                                                                                                                                  \
        this->method_impl(                                                                                                             \
            decl,                                                                                                                      \
            detail::to_asGENFUNC_t(fp<Fn>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>, composite_wrapper_nontype<Composite>{}), \
            generic_call_conv                                                                                                          \
        );                                                                                                                             \
        return *this;                                                                                                                  \
    }                                                                                                                                  \
    template <auto Fn, auto Composite>                                                                                                 \
    requires(std::is_member_function_pointer_v<decltype(Fn)>)                                                                          \
    register_type& method(                                                                                                             \
        std::string_view decl,                                                                                                         \
        fp_wrapper<Fn>,                                                                                                                \
        composite_wrapper_nontype<Composite>,                                                                                          \
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL> = {}                                                                       \
    )                                                                                                                                  \
    {                                                                                                                                  \
        if constexpr(ForceGeneric)                                                                                                     \
        {                                                                                                                              \
            this->method(                                                                                                              \
                use_generic,                                                                                                           \
                decl,                                                                                                                  \
                fp<Fn>,                                                                                                                \
                composite_wrapper_nontype<Composite>{},                                                                                \
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>                                                                      \
            );                                                                                                                         \
        }                                                                                                                              \
        else                                                                                                                           \
        {                                                                                                                              \
            this->method(                                                                                                              \
                decl,                                                                                                                  \
                Fn,                                                                                                                    \
                composite(Composite),                                                                                                  \
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>                                                                      \
            );                                                                                                                         \
        }                                                                                                                              \
        return *this;                                                                                                                  \
    }

#define ASBIND20_CLASS_WRAPPED_COMPOSITE_VAR_TYPE_METHOD(register_type)      \
    template <auto Fn, auto Composite, std::size_t... Is>                    \
    requires(std::is_member_function_pointer_v<decltype(Fn)>)                \
    register_type& method(                                                   \
        use_generic_t,                                                       \
        std::string_view decl,                                               \
        fp_wrapper<Fn>,                                                      \
        composite_wrapper_nontype<Composite>,                                \
        var_type_t<Is...>,                                                   \
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL> = {}             \
    )                                                                        \
    {                                                                        \
        this->method_impl(                                                   \
            decl,                                                            \
            detail::to_asGENFUNC_t(                                          \
                fp<Fn>,                                                      \
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>,           \
                composite_wrapper_nontype<Composite>{},                      \
                var_type<Is...>                                              \
            ),                                                               \
            generic_call_conv                                                \
        );                                                                   \
        return *this;                                                        \
    }                                                                        \
    template <auto Fn, auto Composite, std::size_t... Is>                    \
    requires(std::is_member_function_pointer_v<decltype(Fn)>)                \
    register_type& method(                                                   \
        std::string_view decl,                                               \
        fp_wrapper<Fn>,                                                      \
        composite_wrapper_nontype<Composite>,                                \
        var_type_t<Is...>,                                                   \
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL> = {}             \
    )                                                                        \
    {                                                                        \
        if constexpr(ForceGeneric)                                           \
        {                                                                    \
            this->method(                                                    \
                use_generic,                                                 \
                decl,                                                        \
                fp<Fn>,                                                      \
                composite_wrapper_nontype<Composite>{},                      \
                var_type<Is...>,                                             \
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>            \
            );                                                               \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            /* Native calling convention doesn't need the `var_type` tag. */ \
            this->method(                                                    \
                decl,                                                        \
                Fn,                                                          \
                composite(Composite),                                        \
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>            \
            );                                                               \
        }                                                                    \
        return *this;                                                        \
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

        this->template register_object_type<Class>(
            flags, static_cast<int>(sizeof(Class))
        );
    }

    template <std::convertible_to<std::string_view> StringView>
    basic_value_class(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        StringView name,
        AS_NAMESPACE_QUALIFIER asQWORD flags = 0
    )
        : basic_value_class(
              engine,
              std::string(static_cast<std::string_view>(name)),
              flags
          )
    {}

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
        fp_wrapper<Constructor>,
        call_conv_t<CallConv>
    )
    {
        this->constructor_function(
            params,
            detail::constructor_to_asGENFUNC_t<Class, Template>(fp<Constructor>, call_conv<CallConv>),
            generic_call_conv
        );

        return *this;
    }

    template <
        auto ConstructorFunc,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        fp_wrapper<ConstructorFunc>,
        call_conv_t<CallConv>
    )
    {
        this->constructor_function(
            params,
            use_explicit,
            detail::constructor_to_asGENFUNC_t<Class, Template>(fp<ConstructorFunc>, call_conv<CallConv>),
            generic_call_conv
        );

        return *this;
    }

    template <auto ConstructorFunc>
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        fp_wrapper<ConstructorFunc>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT, Class, std::decay_t<decltype(ConstructorFunc)>>();
        this->constructor_function(
            use_generic,
            params,
            fp<ConstructorFunc>,
            call_conv<conv>
        );

        return *this;
    }

    template <auto ConstructorFunc>
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        fp_wrapper<ConstructorFunc>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT, Class, std::decay_t<decltype(ConstructorFunc)>>();
        this->constructor_function(
            use_generic,
            params,
            use_explicit,
            fp<ConstructorFunc>,
            call_conv<conv>
        );

        return *this;
    }

    template <auto Constructor>
    basic_value_class& constructor_function(
        std::string_view params,
        fp_wrapper<Constructor>
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
        fp_wrapper<Constructor>
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
        noncapturing_lambda ConstructorLambda,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        const ConstructorLambda&,
        call_conv_t<CallConv>
    )
    {
        this->constructor_function(
            params,
            detail::constructor_to_asGENFUNC_t<Class, Template>(ConstructorLambda{}, call_conv<CallConv>),
            generic_call_conv
        );

        return *this;
    }

    template <
        noncapturing_lambda ConstructorLambda,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        const ConstructorLambda&,
        call_conv_t<CallConv>
    )
    {
        this->constructor_function(
            params,
            use_explicit,
            detail::constructor_to_asGENFUNC_t<Class, Template>(ConstructorLambda{}, call_conv<CallConv>),
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
        detail::constructor<Class, Template, Args...> wrapper;
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
        detail::constructor<Class, Template, Args...> wrapper;
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
            return meta::is_constructible_at_v<Class, AS_NAMESPACE_QUALIFIER asITypeInfo*, Args...>;
        else
            return meta::is_constructible_at_v<Class, Args...>;
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

private:
    static constexpr const char* decl_default_ctor()
    {
        return Template ? "void f(int&in)" : "void f()";
    }

public:
    basic_value_class& default_constructor(use_generic_t)
    {
        using gen_t = detail::constructor<Class, Template>;
        this->behaviour_impl(
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            decl_default_ctor(),
            gen_t::generate(generic_call_conv),
            generic_call_conv,
            nullptr
        );

        return *this;
    }

    basic_value_class& default_constructor()
    {
        if constexpr(ForceGeneric)
            this->default_constructor(use_generic);
        else
        {
            using gen_t = detail::constructor<Class, Template>;
            this->behaviour_impl(
                AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
                decl_default_ctor(),
                gen_t::generate(call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>),
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>,
                nullptr
            );
        }

        return *this;
    }

private:
    std::string decl_copy_ctor() const
    {
        return string_concat("void f(const ", m_name, "&in)");
    }

public:
    basic_value_class& copy_constructor(use_generic_t)
    {
        if constexpr(std::is_array_v<Class>)
        {
            detail::arr_copy_constructor<Class, const Class&> wrapper;
            this->behaviour_impl(
                AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
                decl_copy_ctor().c_str(),
                wrapper.generate(generic_call_conv),
                generic_call_conv
            );
        }
        else
        {
            constructor<const Class&>(
                use_generic,
                string_concat("const ", m_name, " &in")
            );
        }

        return *this;
    }

    basic_value_class& copy_constructor()
    {
        if constexpr(ForceGeneric)
        {
            this->copy_constructor(use_generic);
        }
        else
        {
            if constexpr(std::is_array_v<Class>)
            {
                detail::arr_copy_constructor<Class, const Class&> wrapper;
                this->behaviour_impl(
                    AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
                    decl_copy_ctor().c_str(),
                    wrapper.generate(call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>),
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
                );
            }
            else
            {
                constructor<const Class&>(
                    string_concat("const ", m_name, " &in")
                );
            }
        }

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
            if constexpr(meta::is_constructible_at_v<Class>)
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
            if constexpr(meta::is_constructible_at_v<Class, const Class&>)
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
            if constexpr(meta::is_constructible_at_v<Class>)
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
            if constexpr(meta::is_constructible_at_v<Class, const Class&>)
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
        native_function ListConstructorFunc,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    basic_value_class& list_constructor_function(
        std::string_view pattern,
        ListConstructorFunc&& ctor,
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

    template <native_function ListConstructorFunc>
    basic_value_class& list_constructor_function(
        std::string_view pattern,
        ListConstructorFunc&& ctor
    ) requires(!ForceGeneric)
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_CONSTRUCT, Class, std::decay_t<ListConstructorFunc>>();
        this->list_constructor_function(
            pattern,
            ctor,
            call_conv<conv>
        );

        return *this;
    }

    template <
        auto ListConstructorFunc,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    basic_value_class& list_constructor_function(
        use_generic_t,
        std::string_view pattern,
        fp_wrapper<ListConstructorFunc>,
        call_conv_t<CallConv>
    )
    {
        list_constructor_function(
            pattern,
            detail::list_constructor_to_asGENFUNC_t<Class, Template>(
                fp<ListConstructorFunc>, call_conv<CallConv>
            ),
            generic_call_conv
        );

        return *this;
    }

    template <
        auto ListConstructorFunc,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
    )
    basic_value_class& list_constructor_function(
        std::string_view pattern,
        fp_wrapper<ListConstructorFunc>,
        call_conv_t<CallConv>
    )
    {
        if constexpr(ForceGeneric)
        {
            list_constructor_function(use_generic, pattern, fp<ListConstructorFunc>, call_conv<CallConv>);
        }
        else
        {
            list_constructor_function(pattern, ListConstructorFunc, call_conv<CallConv>);
        }

        return *this;
    }

    template <auto ListConstructorFunc>
    basic_value_class& list_constructor_function(
        use_generic_t,
        std::string_view pattern,
        fp_wrapper<ListConstructorFunc>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_CONSTRUCT, Class, std::decay_t<decltype(ListConstructorFunc)>>();

        this->list_constructor_function(use_generic, pattern, fp<ListConstructorFunc>, call_conv<conv>);

        return *this;
    }

    template <auto ListConstructorFunc>
    basic_value_class& list_constructor_function(
        std::string_view pattern,
        fp_wrapper<ListConstructorFunc>
    )
    {
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_CONSTRUCT, Class, std::decay_t<decltype(ListConstructorFunc)>>();
        if constexpr(ForceGeneric)
        {
            this->list_constructor_function(use_generic, pattern, fp<ListConstructorFunc>, call_conv<conv>);
        }
        else
        {
            this->list_constructor_function(pattern, ListConstructorFunc, call_conv<conv>);
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
            detail::list_constructor<Class, Template, ListElementType, Policy>::generate(generic_call_conv),
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
                detail::list_constructor<Class, Template, ListElementType, Policy>::generate(
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
    ASBIND20_VALUE_CLASS_OP(opModAssign);

    ASBIND20_VALUE_CLASS_OP(opEquals);
    ASBIND20_VALUE_CLASS_OP(opCmp);

    ASBIND20_VALUE_CLASS_OP(opAdd);
    ASBIND20_VALUE_CLASS_OP(opSub);
    ASBIND20_VALUE_CLASS_OP(opMul);
    ASBIND20_VALUE_CLASS_OP(opDiv);
    ASBIND20_VALUE_CLASS_OP(opMod);

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

    template <typename OtherClass, bool OtherUseGeneric>
    requires(!std::same_as<Class, OtherClass>)
    basic_value_class& opConv(use_generic_t, const basic_value_class<OtherClass, false, OtherUseGeneric>& other)
    {
        assert(this->get_engine() == other.get_engine());
        this->opConv<OtherClass>(use_generic, other.get_name());
        return *this;
    }

    template <typename OtherClass, bool OtherUseGeneric>
    requires(!std::same_as<Class, OtherClass>)
    basic_value_class& opConv(const basic_value_class<OtherClass, false, OtherUseGeneric>& other)
    {
        assert(this->get_engine() == other.get_engine());
        this->opConv<OtherClass>(other.get_name());
        return *this;
    }

    template <typename OtherClass, bool OtherUseGeneric>
    requires(!std::same_as<Class, OtherClass>)
    basic_value_class& opImplConv(use_generic_t, const basic_value_class<OtherClass, false, OtherUseGeneric>& other)
    {
        assert(this->get_engine() == other.get_engine());
        this->opImplConv<OtherClass>(use_generic, other.get_name());
        return *this;
    }

    template <typename OtherClass, bool OtherUseGeneric>
    requires(!std::same_as<Class, OtherClass>)
    basic_value_class& opImplConv(const basic_value_class<OtherClass, false, OtherUseGeneric>& other)
    {
        assert(this->get_engine() == other.get_engine());
        this->opImplConv<OtherClass>(other.get_name());
        return *this;
    }

#define ASBIND20_VALUE_CLASS_BEH(func_name, as_beh)                                      \
    template <native_function Fn>                                                        \
    basic_value_class& func_name(Fn&& fn) requires(!ForceGeneric)                        \
    {                                                                                    \
        using func_t = std::decay_t<Fn>;                                                 \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                         \
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER as_beh, Class, func_t>(); \
        this->behaviour_impl(                                                            \
            AS_NAMESPACE_QUALIFIER as_beh,                                               \
            decl::decl_of_beh<AS_NAMESPACE_QUALIFIER as_beh>(),                          \
            fn,                                                                          \
            call_conv<conv>                                                              \
        );                                                                               \
        return *this;                                                                    \
    }                                                                                    \
    basic_value_class& func_name(AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn)                 \
    {                                                                                    \
        this->behaviour_impl(                                                            \
            AS_NAMESPACE_QUALIFIER as_beh,                                               \
            decl::decl_of_beh<AS_NAMESPACE_QUALIFIER as_beh>(),                          \
            gfn,                                                                         \
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>                             \
        );                                                                               \
        return *this;                                                                    \
    }                                                                                    \
    template <auto Function>                                                             \
    basic_value_class& func_name(use_generic_t, fp_wrapper<Function>)                    \
    {                                                                                    \
        using func_t = std::decay_t<decltype(Function)>;                                 \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                         \
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER as_beh, Class, func_t>(); \
        this->func_name(detail::to_asGENFUNC_t(fp<Function>, call_conv<conv>));          \
        return *this;                                                                    \
    }                                                                                    \
    template <auto Function>                                                             \
    basic_value_class& func_name(fp_wrapper<Function>)                                   \
    {                                                                                    \
        if constexpr(ForceGeneric)                                                       \
            this->func_name(use_generic, fp<Function>);                                  \
        else                                                                             \
            this->func_name(Function);                                                   \
        return *this;                                                                    \
    }

    // For garbage collected value type
    // See: https://www.angelcode.com/angelscript/sdk/docs/manual/doc_gc_object.html#doc_reg_gcref_value

    ASBIND20_VALUE_CLASS_BEH(enum_refs, asBEHAVE_ENUMREFS)
    ASBIND20_VALUE_CLASS_BEH(release_refs, asBEHAVE_RELEASEREFS)

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

    template <native_function Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    basic_value_class& method(
        std::string_view decl,
        Fn fn,
        composite_wrapper comp,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL> = {}
    ) requires(!ForceGeneric)
    {
        this->method_impl_comp(
            decl,
            fn,
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>,
            comp
        );
        return *this;
    }

    ASBIND20_CLASS_WRAPPED_COMPOSITE_METHOD(basic_value_class)
    ASBIND20_CLASS_WRAPPED_COMPOSITE_VAR_TYPE_METHOD(basic_value_class)

    template <detail::auto_register<basic_value_class> AutoRegister>
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

    basic_value_class& property(
        std::string_view decl, std::size_t off, composite_wrapper comp
    )
    {
        this->comp_property_impl(decl, off, comp.get_offset());

        return *this;
    }

    template <typename MemberPointer>
    requires(std::is_member_object_pointer_v<MemberPointer>)
    basic_value_class& property(std::string_view decl, MemberPointer mp, composite_wrapper comp)
    {
        this->comp_property_impl(decl, mp, comp.get_offset());

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

        // Size is unnecessary for reference type.
        // Use 0 as size to support registering an incomplete type.
        this->template register_object_type<Class>(flags, 0);
    }

    template <std::convertible_to<std::string_view> StringView>
    basic_ref_class(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        StringView name,
        AS_NAMESPACE_QUALIFIER asQWORD flags = 0
    )
        : basic_ref_class(
              engine,
              std::string(static_cast<std::string_view>(name)),
              flags
          )
    {}

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

    template <native_function Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    basic_ref_class& method(
        std::string_view decl,
        Fn fn,
        composite_wrapper comp,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL> = {}
    ) requires(!ForceGeneric)
    {
        this->method_impl_comp(
            decl,
            fn,
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>,
            comp
        );
        return *this;
    }

    ASBIND20_CLASS_WRAPPED_COMPOSITE_METHOD(basic_ref_class)
    ASBIND20_CLASS_WRAPPED_COMPOSITE_VAR_TYPE_METHOD(basic_ref_class)

    template <detail::auto_register<basic_ref_class> AutoRegister>
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
                    AS_NAMESPACE_QUALIFIER asITypeInfo*,
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
        fp_wrapper<Factory>,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_CDECL> = {}
    )
    {
        factory_function(
            params,
            detail::to_asGENFUNC_t(fp<Factory>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>),
            generic_call_conv
        );

        return *this;
    }

    template <auto Factory>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        fp_wrapper<Factory>,
        call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_CDECL> = {}
    )
    {
        this->factory_function(
            params,
            use_explicit,
            detail::to_asGENFUNC_t(fp<Factory>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>),
            generic_call_conv
        );

        return *this;
    }

    template <
        auto AuxFactoryFunc,
        typename Auxiliary,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        fp_wrapper<AuxFactoryFunc>,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<CallConv>
    )
    {
        factory_function(
            params,
            detail::auxiliary_factory_to_asGENFUNC_t<Template>(fp<AuxFactoryFunc>, call_conv<CallConv>),
            aux,
            generic_call_conv
        );

        return *this;
    }

    template <
        auto AuxFactoryFunc,
        typename Auxiliary,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        fp_wrapper<AuxFactoryFunc>,
        auxiliary_wrapper<Auxiliary> aux,
        call_conv_t<CallConv>
    )
    {
        factory_function(
            params,
            use_explicit,
            detail::auxiliary_factory_to_asGENFUNC_t<Template>(fp<AuxFactoryFunc>, call_conv<CallConv>),
            aux,
            generic_call_conv
        );

        return *this;
    }

    template <auto Factory>
    basic_ref_class& factory_function(
        std::string_view params,
        fp_wrapper<Factory>
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
        fp_wrapper<Factory>
    )
    {
        if constexpr(ForceGeneric)
            factory_function(use_generic, params, use_explicit, fp<Factory>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);
        else
            factory_function(params, use_explicit, Factory, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);

        return *this;
    }

    template <auto Factory, AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL)
    basic_ref_class& factory_function(
        std::string_view params,
        fp_wrapper<Factory>,
        call_conv_t<CallConv>
    )
    {
        if constexpr(ForceGeneric)
            factory_function(use_generic, params, fp<Factory>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);
        else
            factory_function(params, Factory, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);

        return *this;
    }

    template <auto Factory, AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    requires(CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL)
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        fp_wrapper<Factory>,
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
        fp_wrapper<Factory>,
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
        fp_wrapper<Factory>,
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
        fp_wrapper<Factory>,
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
        fp_wrapper<Factory>,
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
            detail::factory<Class, Template, Policy, Args...>::generate(generic_call_conv);

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
                detail::factory<Class, Template, Policy, Args...>::generate(call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>);

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
                detail::factory<Class, Template, Policy, Args...>::generate(call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);

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
        fp_wrapper<ListFactory>,
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
        fp_wrapper<ListFactory>,
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
        fp_wrapper<ListFactory>,
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
        fp_wrapper<ListFactory>,
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

private:
    // For non-templated types,
    // GC notifier needs to access the typeinfo through the auxiliary pointer
    template <typename FactoryPolicy>
    void* aux_for_notifying_gc() const
    {
        void* aux = nullptr;
        if constexpr(std::same_as<FactoryPolicy, policies::notify_gc> && !Template)
        {
            aux = m_engine->GetTypeInfoById(this->get_type_id());
            assert(aux != nullptr);
        }
        return aux;
    }

public:
    template <
        typename ListElementType = void,
        policies::initialization_list_policy IListPolicy,
        policies::factory_policy FactoryPolicy>
    basic_ref_class& list_factory(
        use_generic_t,
        std::string_view pattern,
        use_policy_t<IListPolicy, FactoryPolicy>
    )
    {
        using gen_t =
            detail::list_factory<Class, Template, ListElementType, IListPolicy, FactoryPolicy>;

        this->list_factory_function(
            pattern,
            gen_t::generate(generic_call_conv),
            auxiliary(this->aux_for_notifying_gc<FactoryPolicy>()),
            generic_call_conv
        );

        return *this;
    }

    template <
        typename ListElementType = void,
        policies::initialization_list_policy IListPolicy,
        policies::factory_policy FactoryPolicy>
    basic_ref_class& list_factory(
        std::string_view pattern,
        use_policy_t<IListPolicy, FactoryPolicy>
    )
    {
        if constexpr(ForceGeneric)
        {
            list_factory<ListElementType>(use_generic, pattern, use_policy<IListPolicy, FactoryPolicy>);
        }
        else
        {
            if constexpr(std::same_as<FactoryPolicy, policies::notify_gc> && !Template)
            {
                using gen_t =
                    detail::list_factory<Class, Template, ListElementType, IListPolicy, FactoryPolicy>;

                this->list_factory_function(
                    pattern,
                    gen_t::generate(
                        call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
                    ),
                    auxiliary(this->aux_for_notifying_gc<FactoryPolicy>()),
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
                );
            }
            else
            {
                using gen_t =
                    detail::list_factory<Class, Template, ListElementType, IListPolicy, FactoryPolicy>;

                list_factory_function(
                    pattern,
                    gen_t::generate(
                        call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                    ),
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                );
            }
        }

        return *this;
    }

    template <
        typename ListElementType = void,
        policies::initialization_list_policy IListPolicy = void>
    basic_ref_class& list_factory(
        use_generic_t,
        std::string_view pattern,
        use_policy_t<IListPolicy> = {}
    )
    {
        AS_NAMESPACE_QUALIFIER asGENFUNC_t wrapper =
            detail::list_factory<Class, Template, ListElementType, IListPolicy, void>::generate(generic_call_conv);

        list_factory_function(
            pattern,
            wrapper,
            generic_call_conv
        );

        return *this;
    }

    template <
        typename ListElementType = void,
        policies::initialization_list_policy IListPolicy = void>
    basic_ref_class& list_factory(
        std::string_view pattern,
        use_policy_t<IListPolicy> = {}
    )
    {
        if constexpr(ForceGeneric)
        {
            list_factory<ListElementType>(use_generic, pattern, use_policy<IListPolicy>);
        }
        else
        {
            using gen_t =
                detail::list_factory<Class, Template, ListElementType, IListPolicy, void>;

            list_factory_function(
                pattern,
                gen_t::generate(
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                ),
                call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
            );
        }

        return *this;
    }

    template <
        typename ListElementType = void,
        policies::factory_policy FactoryPolicy>
    requires(!std::is_void_v<FactoryPolicy>)
    basic_ref_class& list_factory(
        use_generic_t,
        std::string_view pattern,
        use_policy_t<FactoryPolicy> = {}
    )
    {
        using gen_t =
            detail::list_factory<Class, Template, ListElementType, void, FactoryPolicy>;

        list_factory_function(
            pattern,
            gen_t::generate(generic_call_conv),
            auxiliary(this->aux_for_notifying_gc<FactoryPolicy>()),
            generic_call_conv
        );

        return *this;
    }

    template <
        typename ListElementType = void,
        policies::factory_policy FactoryPolicy>
    requires(!std::is_void_v<FactoryPolicy>)
    basic_ref_class& list_factory(
        std::string_view pattern,
        use_policy_t<FactoryPolicy> = {}
    )
    {
        if constexpr(ForceGeneric)
        {
            list_factory<ListElementType>(use_generic, pattern, use_policy<FactoryPolicy>);
        }
        else
        {
            using gen_t =
                detail::list_factory<Class, Template, ListElementType, void, FactoryPolicy>;

            if constexpr(Template)
            {
                list_factory_function(
                    pattern,
                    gen_t::generate(
                        call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                    ),
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                );
            }
            else
            {
                list_factory_function(
                    pattern,
                    gen_t::generate(
                        call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
                    ),
                    auxiliary(this->aux_for_notifying_gc<FactoryPolicy>()),
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
                );
            }
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

    ASBIND20_REFERENCE_CLASS_OP(opAssign)
    ASBIND20_REFERENCE_CLASS_OP(opAddAssign)
    ASBIND20_REFERENCE_CLASS_OP(opSubAssign)
    ASBIND20_REFERENCE_CLASS_OP(opMulAssign)
    ASBIND20_REFERENCE_CLASS_OP(opDivAssign)
    ASBIND20_REFERENCE_CLASS_OP(opModAssign)

    ASBIND20_REFERENCE_CLASS_OP(opEquals)
    ASBIND20_REFERENCE_CLASS_OP(opCmp)

    ASBIND20_REFERENCE_CLASS_OP(opPreInc)
    ASBIND20_REFERENCE_CLASS_OP(opPreDec)

    // TODO: Operators returning by value for reference type

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

#define ASBIND20_REFERENCE_CLASS_BEH(func_name, as_beh)                                  \
    template <native_function Fn>                                                        \
    basic_ref_class& func_name(Fn&& fn) requires(!ForceGeneric)                          \
    {                                                                                    \
        using func_t = std::decay_t<Fn>;                                                 \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                         \
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER as_beh, Class, func_t>(); \
        this->behaviour_impl(                                                            \
            AS_NAMESPACE_QUALIFIER as_beh,                                               \
            decl::decl_of_beh<AS_NAMESPACE_QUALIFIER as_beh>(),                          \
            fn,                                                                          \
            call_conv<conv>                                                              \
        );                                                                               \
        return *this;                                                                    \
    }                                                                                    \
    basic_ref_class& func_name(AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn)                   \
    {                                                                                    \
        this->behaviour_impl(                                                            \
            AS_NAMESPACE_QUALIFIER as_beh,                                               \
            decl::decl_of_beh<AS_NAMESPACE_QUALIFIER as_beh>(),                          \
            gfn,                                                                         \
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>                             \
        );                                                                               \
        return *this;                                                                    \
    }                                                                                    \
    template <auto Function>                                                             \
    basic_ref_class& func_name(use_generic_t, fp_wrapper<Function>)                      \
    {                                                                                    \
        using func_t = std::decay_t<decltype(Function)>;                                 \
        constexpr AS_NAMESPACE_QUALIFIER asECallConvTypes conv =                         \
            detail::deduce_beh_callconv<AS_NAMESPACE_QUALIFIER as_beh, Class, func_t>(); \
        this->func_name(detail::to_asGENFUNC_t(fp<Function>, call_conv<conv>));          \
        return *this;                                                                    \
    }                                                                                    \
    template <auto Function>                                                             \
    basic_ref_class& func_name(fp_wrapper<Function>)                                     \
    {                                                                                    \
        if constexpr(ForceGeneric)                                                       \
            this->func_name(use_generic, fp<Function>);                                  \
        else                                                                             \
            this->func_name(Function);                                                   \
        return *this;                                                                    \
    }

    ASBIND20_REFERENCE_CLASS_BEH(get_weakref_flag, asBEHAVE_GET_WEAKREF_FLAG)
    ASBIND20_REFERENCE_CLASS_BEH(addref, asBEHAVE_ADDREF)
    ASBIND20_REFERENCE_CLASS_BEH(release, asBEHAVE_RELEASE)
    ASBIND20_REFERENCE_CLASS_BEH(get_refcount, asBEHAVE_GETREFCOUNT)
    ASBIND20_REFERENCE_CLASS_BEH(set_gc_flag, asBEHAVE_SETGCFLAG)
    ASBIND20_REFERENCE_CLASS_BEH(get_gc_flag, asBEHAVE_GETGCFLAG)
    ASBIND20_REFERENCE_CLASS_BEH(enum_refs, asBEHAVE_ENUMREFS)
    ASBIND20_REFERENCE_CLASS_BEH(release_refs, asBEHAVE_RELEASEREFS)

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

    basic_ref_class& property(
        std::string_view decl, std::size_t off, composite_wrapper comp
    )
    {
        this->comp_property_impl(decl, off, comp.get_offset());

        return *this;
    }

    template <typename MemberPointer>
    requires(std::is_member_object_pointer_v<MemberPointer>)
    basic_ref_class& property(std::string_view decl, MemberPointer mp, composite_wrapper comp)
    {
        this->comp_property_impl(decl, mp, comp.get_offset());

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
#undef ASBIND20_CLASS_WRAPPED_COMPOSITE_METHOD
#undef ASBIND20_CLASS_WRAPPED_COMPOSITE_VAR_TYPE_METHOD

template <typename Class, bool ForceGeneric = false>
using ref_class = basic_ref_class<Class, false, ForceGeneric>;

template <typename Class, bool ForceGeneric = false>
using template_ref_class = basic_ref_class<Class, true, ForceGeneric>;
} // namespace asbind20

#endif
