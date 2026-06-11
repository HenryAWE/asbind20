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
#include "genfunc.hpp"
#include "../policies.hpp"
#include "behaviour.hpp"

namespace asbind20
{
namespace detail
{
    // Helper for the `as_iterators` policy
    template <typename T, typename Fn>
    decltype(auto) apply_iter_pair(Fn&& fn, script_init_list_repeat list)
    {
        T* const start = (T*)list.data();
        T* const stop = start + list.size();

        return std::invoke(std::forward<Fn>(fn), start, stop);
    }

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

        static void impl_generic(generic_pointer gen)
        {
            using args_tuple = std::tuple<Args...>;

            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                void* mem = gen->GetObject();
                new(mem) Class(
                    get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                        gen, static_cast<gen_idx_t>(Is)
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

        static void impl_generic(generic_pointer gen)
        {
            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                using args_tuple = std::tuple<Args...>;

                void* mem = gen->GetObject();
                new(mem) Class(
                    get_generic_typeinfo(gen),
                    get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                        gen, static_cast<gen_idx_t>(Is) + 1
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

        static void impl_generic(generic_pointer gen)
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
                return +[](generic_pointer gen) -> void
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
        static void impl_generic(generic_pointer gen)
        {
            if constexpr(Template)
            {
                void* mem = gen->GetObject();
                new(mem) Class(
                    get_generic_typeinfo(gen),
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
        static void impl_generic(generic_pointer gen)
        {
            if constexpr(Template)
            {
                void* mem = gen->GetObject();
                new(mem) Class(
                    get_generic_typeinfo(gen),
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

        static void impl_generic(generic_pointer gen)
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
                apply_iter_pair<ListElementType>(
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

        static void impl_generic(generic_pointer gen)
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
        static void impl_generic(generic_pointer gen)
        {
            using args_tuple = std::tuple<Args...>;
            if constexpr(Template)
            {
                [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                {
                    auto* ptr = new Class(
                        get_generic_typeinfo(gen),
                        get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                            gen, static_cast<gen_idx_t>(Is) + 1
                        )...
                    );
                    gen->SetReturnAddress(ptr);
                }(std::index_sequence_for<Args...>());
            }
            else
            {
                [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                {
                    auto* ptr = new Class(
                        get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                            gen, static_cast<gen_idx_t>(Is)
                        )...
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

        static void impl_generic(generic_pointer gen)
        {
            using args_tuple = std::tuple<Args...>;

            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                auto* ti = (AS_NAMESPACE_QUALIFIER asITypeInfo*)gen->GetAuxiliary();
                auto* ptr = new Class(
                    get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                        gen, static_cast<gen_idx_t>(Is)
                    )...
                );
                ASBIND20_ASSERT(ti->GetEngine() == gen->GetEngine());
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
            auto* ptr = new Class(std::forward<Args>(args)...);
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

        static void impl_generic(generic_pointer gen)
        {
            using args_tuple = std::tuple<Args...>;

            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                auto* ti = get_generic_typeinfo(gen);
                auto* ptr = new Class(
                    ti,
                    get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                        gen, static_cast<gen_idx_t>(Is) + 1
                    )...
                );

                if(has_script_exception()) [[unlikely]]
                {
                    delete ptr;
                    return;
                }

                if(ti->GetFlags() & AS_NAMESPACE_QUALIFIER asOBJ_GC)
                {
                    ASBIND20_ASSERT(ti->GetEngine() == gen->GetEngine());
                    gen->GetEngine()->NotifyGarbageCollectorOfNewObject(ptr, ti);
                }

                gen->SetReturnAddress(ptr);
            }(std::index_sequence_for<Args...>());
        }

        static Class* impl_cdecl(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, Args... args)
        {
            auto* ptr = new Class(ti, std::forward<Args>(args)...);

            if(has_script_exception()) [[unlikely]]
            {
                delete ptr;
                return nullptr;
            }

            if(ti->GetFlags() & AS_NAMESPACE_QUALIFIER asOBJ_GC)
                ti->GetEngine()->NotifyGarbageCollectorOfNewObject(ptr, ti);

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
        static void impl_generic(generic_pointer gen)
        {
            if constexpr(Template)
            {
                Class* ptr = new Class(
                    get_generic_typeinfo(gen),
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

        static void impl_generic(generic_pointer gen)
        {
            if constexpr(Template)
            {
                auto* ti = get_generic_typeinfo(gen);
                auto* ptr = new Class(
                    ti, *(ListElementType**)gen->GetAddressOfArg(1)
                );
                notifier::notify_gc_if_necessary(ptr, ti);

                gen->SetReturnAddress(ptr);
            }
            else
            {
                auto* ptr = new Class(
                    *(ListElementType**)gen->GetAddressOfArg(0)
                );

                // Expects the typeinfo is passed by auxiliary pointer (see the helper "auxiliary(this_type)")
                auto* ti = get_generic_auxiliary<AS_NAMESPACE_QUALIFIER asITypeInfo*>(gen);
                ASBIND20_ASSERT(ti != nullptr);
                notifier::notify_gc_if_necessary(ptr, ti);

                gen->SetReturnAddress(ptr);
            }
        }

        static Class* impl_cdecl_template(
            AS_NAMESPACE_QUALIFIER asITypeInfo* ti, ListElementType* list_buf
        )
        {
            auto* ptr = new Class(ti, list_buf);
            notifier::notify_gc_if_necessary(ptr, ti);
            return ptr;
        }

        // Works together with the helper "auxiliary(this_type)"
        static Class* impl_cdecl_objlast(
            ListElementType* list_buf, AS_NAMESPACE_QUALIFIER asITypeInfo* ti
        )
        {
            auto* ptr = new Class(list_buf);
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

        static void impl_generic(generic_pointer gen)
        {
            auto* ptr = apply_helper(*(ListElementType**)gen->GetAddressOfArg(0));
            if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                auto* ti = get_generic_auxiliary<AS_NAMESPACE_QUALIFIER asITypeInfo*>(gen);
                ASBIND20_ASSERT(ti != nullptr);
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

        static void impl_generic(generic_pointer gen)
        {
            if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                auto* ptr = new Class(script_init_list_repeat(gen));
                // Works together with the helper "auxiliary(this_type)"
                auto* ti = get_generic_auxiliary<AS_NAMESPACE_QUALIFIER asITypeInfo*>(gen);
                ASBIND20_ASSERT(ti != nullptr);
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
            auto* ptr = new Class(script_init_list_repeat(list_buf));
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

        static void impl_generic(generic_pointer gen)
        {
            auto* ti = get_generic_typeinfo(gen);
            auto* ptr = new Class(
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
            auto* ptr = new Class(ti, script_init_list_repeat(list_buf));
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
                return apply_iter_pair<ListElementType>(
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

        static void impl_generic(generic_pointer gen)
        {
            Class* ptr = from_list_helper(script_init_list_repeat(gen));
            if constexpr(std::same_as<FactoryPolicy, policies::notify_gc>)
            {
                // Expects the typeinfo is passed by auxiliary pointer (see the helper "auxiliary(this_type)")
                auto* ti = (AS_NAMESPACE_QUALIFIER asITypeInfo*)gen->GetAuxiliary();
                ASBIND20_ASSERT(ti != nullptr);
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

    template <typename Class>
    class opCmp
    {
        static int do_compare(const Class& lhs, const Class& rhs)
        {
            return translate_three_way(
                std::compare_weak_order_fallback(lhs, rhs)
            );
        }

        static void impl_generic(generic_pointer gen)
        {
            const Class& lhs = get_generic_object<const Class>(gen);
            const Class& rhs = get_generic_arg<const Class&>(gen, 0);
            set_generic_return<int>(
                gen, do_compare(lhs, rhs)
            );
        }

        static int impl_objfirst(const Class& lhs, const Class& rhs)
        {
            return do_compare(lhs, rhs);
        }

    public:
        template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
        static auto generate(call_conv_t<CallConv>) noexcept
        {
            if constexpr(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
                return &impl_generic;
            else // CallConv == asCALL_CDECL_OBJFIRST
                return &impl_objfirst;
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
                return +[](generic_pointer gen) -> void
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

template <bool ForceGeneric, typename Listener = default_listener>
class class_binding_generator_base :
    public binding_generator_interface<ForceGeneric, Listener>
{
    using my_base = binding_generator_interface<ForceGeneric, Listener>;

public:
    using my_base::get_engine;

    [[nodiscard]]
    int get_type_id() const noexcept
    {
        ASBIND20_ASSERT(m_this_type_id > 0);
        return m_this_type_id;
    }

    template <typename Auxiliary>
    [[nodiscard]]
    void* get_auxiliary_address(auxiliary_wrapper<Auxiliary> aux) const
    {
        if constexpr(std::same_as<Auxiliary, this_type_t>)
        {
            return get_engine()->GetTypeInfoById(get_type_id());
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

private:
    std::string m_name;
    // 0 == asTYPEID_VOID, as a placeholder
    int m_this_type_id = 0;

public:
    using flag_type = AS_NAMESPACE_QUALIFIER asQWORD;

    class_binding_generator_base() = delete;
    class_binding_generator_base(const class_binding_generator_base&) = default;

    class_binding_generator_base(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, std::string name)
        : my_base(engine), m_name(std::move(name)) {}

protected:
    int register_object_type(flag_type flags, int size)
    {
        int r = 0;
        r = get_engine()->RegisterObjectType(
            m_name.c_str(),
            size,
            flags
        );
        if(r > 0) [[likely]]
            m_this_type_id = r;
        return r;
    }

    int append_type(bool append_only, flag_type flags, int size)
    {
        AS_NAMESPACE_QUALIFIER asITypeInfo* ti = get_engine()->GetTypeInfoByName(m_name.c_str());
        if(!ti)
        {
            if(append_only)
                return AS_NAMESPACE_QUALIFIER asINVALID_TYPE;
            return register_object_type(flags, size);
        }
        int r = ti->GetTypeId();
#ifndef ASBIND20_CONFIG_NO_APPEND_CHECK
        if (flags != 0)
        {
            [[maybe_unused]]
            const flag_type existing_flags = ti->GetFlags();
            ASBIND20_ASSERT(existing_flags == flags);
        }
#endif
        if(r > 0) [[likely]]
            m_this_type_id = r;
        return r;
    }

    template <typename Fn>
    [[nodiscard]]
    int register_method(
        cstring_ref decl,
        Fn&& fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes conv,
        void* aux = nullptr
    )
    {
        return get_engine()->RegisterObjectMethod(
            m_name.c_str(),
            decl.c_str(),
            detail::to_asSFuncPtr(fn),
            conv,
            aux
        );
    }

    template <typename Fn>
    int register_comp_method(
        cstring_ref decl,
        Fn&& fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes conv,
        composite_wrapper comp,
        void* aux = nullptr
    )
    {
        return get_engine()->RegisterObjectMethod(
            m_name.c_str(),
            decl.c_str(),
            detail::to_asSFuncPtr(fn),
            conv,
            aux,
            static_cast<int>(comp.get_offset()),
            true
        );
    }

    template <typename Fn>
    int register_behaviour(
        AS_NAMESPACE_QUALIFIER asEBehaviours beh,
        cstring_ref decl,
        Fn&& fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes conv,
        void* aux = nullptr
    )
    {
        return get_engine()->RegisterObjectBehaviour(
            m_name.c_str(),
            beh,
            decl.c_str(),
            detail::to_asSFuncPtr(fn),
            conv,
            aux
        );
    }

    int register_property(cstring_ref decl, std::size_t off)
    {
        return get_engine()->RegisterObjectProperty(
            m_name.c_str(),
            decl.c_str(),
            static_cast<int>(off)
        );
    }

    template <typename MemberPointer>
    int register_property(cstring_ref decl, MemberPointer mp)
    {
        static_assert(std::is_member_object_pointer_v<MemberPointer>);
        return register_property(decl, member_offset(mp));
    }

    int register_comp_property(cstring_ref decl, std::size_t off, std::size_t comp_off)
    {
        return get_engine()->RegisterObjectProperty(
            m_name.c_str(),
            decl.c_str(),
            static_cast<int>(off),
            static_cast<int>(comp_off),
            true
        );
    }

    template <typename CompMemberPointer>
    requires(std::is_member_object_pointer_v<CompMemberPointer>)
    int register_comp_property(cstring_ref decl, std::size_t off, CompMemberPointer comp_mp)
    {
        return register_comp_property(decl, off, member_offset(comp_mp));
    }

    template <typename MemberPointer>
    requires(std::is_member_object_pointer_v<MemberPointer>)
    int register_comp_property(cstring_ref decl, MemberPointer mp, std::size_t comp_off)
    {
        return register_comp_property(decl, member_offset(mp), comp_off);
    }

    template <typename CompMemberPointer, typename MemberPointer>
    requires(std::is_member_object_pointer_v<MemberPointer> && std::is_member_object_pointer_v<CompMemberPointer>)
    int register_comp_property(cstring_ref decl, MemberPointer mp, CompMemberPointer comp_mp)
    {
        return register_comp_property(decl, member_offset(mp), member_offset(comp_mp));
    }

#define ASBIND20_IMPL_REGISTER_UNARY_PREFIX_OP(as_op_sig, cpp_op, decl_arg_list, return_type, const_) \
    std::string decl_##as_op_sig() const                                                              \
    {                                                                                                 \
        return string_concat decl_arg_list;                                                           \
    }                                                                                                 \
    template <typename Class>                                                                         \
    int register_##as_op_sig##_generic()                                                              \
    {                                                                                                 \
        return register_method(                                                                       \
            decl_##as_op_sig(),                                                                       \
            +[](generic_pointer gen) -> void                                                          \
            {                                                                                         \
                using this_arg_t = std::conditional_t<(#const_[0] != '\0'), const Class&, Class&>;    \
                set_generic_return<return_type>(                                                      \
                    gen,                                                                              \
                    cpp_op get_generic_object<this_arg_t>(gen)                                        \
                );                                                                                    \
            },                                                                                        \
            detail::generic_cc                                                                        \
        );                                                                                            \
    }                                                                                                 \
    template <typename Class>                                                                         \
    int register_##as_op_sig##_native()                                                               \
    {                                                                                                 \
        static constexpr bool has_member_func = requires() {                                          \
            static_cast<return_type (Class::*)() const_>(&Class::operator cpp_op);                    \
        };                                                                                            \
        if constexpr(has_member_func)                                                                 \
        {                                                                                             \
            return register_method(                                                                   \
                decl_##as_op_sig(),                                                                   \
                static_cast<return_type (Class::*)() const_>(&Class::operator cpp_op),                \
                detail::cc<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>                                    \
            );                                                                                        \
        }                                                                                             \
        else                                                                                          \
        {                                                                                             \
            using this_arg_t = std::conditional_t<(#const_[0] != '\0'), const Class&, Class&>;        \
            return this->register_method(                                                             \
                decl_##as_op_sig(),                                                                   \
                +[](this_arg_t this_) -> return_type                                                  \
                {                                                                                     \
                    return cpp_op this_;                                                              \
                },                                                                                    \
                detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>                              \
            );                                                                                        \
        }                                                                                             \
    }

    ASBIND20_IMPL_REGISTER_UNARY_PREFIX_OP(
        opNeg, -, (m_name, " opNeg() const"), Class, const
    )

    ASBIND20_IMPL_REGISTER_UNARY_PREFIX_OP(
        opPreInc, ++, (m_name, "& opPreInc()"), Class&,
    )
    ASBIND20_IMPL_REGISTER_UNARY_PREFIX_OP(
        opPreDec, --, (m_name, "& opPreDec()"), Class&,
    )

#undef ASBIND20_CLASS_REGISTER_UNARY_PREFIX_OP

    // Only for operator++/--(int)
#define ASBIND20_IMPL_REGISTER_UNARY_SUFFIX_OP(as_op_sig, cpp_op)           \
    std::string decl_##as_op_sig() const                                    \
    {                                                                       \
        return string_concat(m_name, " " #as_op_sig "()");                  \
    }                                                                       \
    template <typename Class>                                               \
    int register_##as_op_sig##_generic()                                    \
    {                                                                       \
        return register_method(                                             \
            decl_##as_op_sig(),                                             \
            +[](generic_pointer gen) -> void                                \
            {                                                               \
                set_generic_return<Class>(                                  \
                    gen,                                                    \
                    get_generic_object<Class&>(gen) cpp_op                  \
                );                                                          \
            },                                                              \
            detail::generic_cc                                              \
        );                                                                  \
    }                                                                       \
    template <typename Class>                                               \
    int register_##as_op_sig##_native()                                     \
    {                                                                       \
        /* Use a wrapper to deal with the hidden int of postfix operator */ \
        return register_method(                                             \
            decl_##as_op_sig(),                                             \
            +[](Class& this_) -> Class                                      \
            { return this_ cpp_op; },                                       \
            detail::cdecl_last_cc                                           \
        );                                                                  \
    }

    ASBIND20_IMPL_REGISTER_UNARY_SUFFIX_OP(opPostInc, ++)
    ASBIND20_IMPL_REGISTER_UNARY_SUFFIX_OP(opPostDec, --)

#undef ASBIND20_IMPL_REGISTER_UNARY_SUFFIX_OP

#define ASBIND20_IMPL_REGISTER_BINARY_OP_GENERIC(as_decl, cpp_op, return_type, const_, rhs_type) \
    do                                                                                           \
    {                                                                                            \
        return this->register_method(                                                            \
            as_decl,                                                                             \
            +[](generic_pointer gen) -> void                                                     \
            {                                                                                    \
                using this_arg_t = std::conditional_t<                                           \
                    (#const_[0] != '\0'), /* Check whether const_ is empty */                    \
                    const Class&,                                                                \
                    Class&>;                                                                     \
                set_generic_return<return_type>(                                                 \
                    gen,                                                                         \
                    get_generic_object<this_arg_t>(gen) cpp_op get_generic_arg<rhs_type>(gen, 0) \
                );                                                                               \
            },                                                                                   \
            detail::generic_cc                                                                   \
        );                                                                                       \
    } while(0)

#define ASBIND20_IMPL_REGISTER_BINARY_OP_NATIVE(as_decl, cpp_op, return_type, const_, rhs_type) \
    do                                                                                          \
    {                                                                                           \
        static constexpr bool has_member_func = requires() {                                    \
            static_cast<return_type (Class::*)(rhs_type) const_>(&Class::operator cpp_op);      \
        };                                                                                      \
        if constexpr(has_member_func)                                                           \
        {                                                                                       \
            return this->register_method(                                                       \
                as_decl,                                                                        \
                static_cast<return_type (Class::*)(rhs_type) const_>(&Class::operator cpp_op),  \
                detail::cc<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>                              \
            );                                                                                  \
        }                                                                                       \
        else                                                                                    \
        {                                                                                       \
            using this_arg_t = std::conditional_t<                                              \
                (#const_[0] != '\0'), /* Check whether const_ is empty */                       \
                const Class&,                                                                   \
                Class&>;                                                                        \
            return this->register_method(                                                       \
                as_decl,                                                                        \
                +[](this_arg_t lhs, rhs_type rhs) -> return_type                                \
                {                                                                               \
                    return lhs cpp_op rhs;                                                      \
                },                                                                              \
                detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>                        \
            );                                                                                  \
        }                                                                                       \
    } while(0)

#define ASBIND20_IMPL_REGISTER_BINARY_OP(as_op_sig, cpp_op, decl_arg_list, return_type, const_, rhs_type) \
    std::string decl_##as_op_sig() const                                                                  \
    {                                                                                                     \
        return string_concat decl_arg_list;                                                               \
    }                                                                                                     \
    template <typename Class>                                                                             \
    int register_##as_op_sig##_generic()                                                                  \
    {                                                                                                     \
        ASBIND20_IMPL_REGISTER_BINARY_OP_GENERIC(                                                         \
            decl_##as_op_sig(), cpp_op, return_type, const_, rhs_type                                     \
        );                                                                                                \
    }                                                                                                     \
    template <typename Class>                                                                             \
    int register_##as_op_sig##_native()                                                                   \
    {                                                                                                     \
        ASBIND20_IMPL_REGISTER_BINARY_OP_NATIVE(                                                          \
            decl_##as_op_sig(), cpp_op, return_type, const_, rhs_type                                     \
        );                                                                                                \
    }

    // Predefined method names:
    // https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_class_ops.html

    // Assignment operators

    ASBIND20_IMPL_REGISTER_BINARY_OP(
        opAssign, =, (m_name, "& opAssign(const ", m_name, " &in)"), Class&, , const Class&
    )
    ASBIND20_IMPL_REGISTER_BINARY_OP(
        opAddAssign, +=, (m_name, "& opAddAssign(const ", m_name, " &in)"), Class&, , const Class&
    )
    ASBIND20_IMPL_REGISTER_BINARY_OP(
        opSubAssign, -=, (m_name, "& opSubAssign(const ", m_name, " &in)"), Class&, , const Class&
    )
    ASBIND20_IMPL_REGISTER_BINARY_OP(
        opMulAssign, *=, (m_name, "& opMulAssign(const ", m_name, " &in)"), Class&, , const Class&
    )
    ASBIND20_IMPL_REGISTER_BINARY_OP(
        opDivAssign, /=, (m_name, "& opDivAssign(const ", m_name, " &in)"), Class&, , const Class&
    )
    ASBIND20_IMPL_REGISTER_BINARY_OP(
        opModAssign, %=, (m_name, "& opModAssign(const ", m_name, " &in)"), Class&, , const Class&
    )

    // Comparison operators

    ASBIND20_IMPL_REGISTER_BINARY_OP(
        opEquals, ==, ("bool opEquals(const ", m_name, " &in) const"), bool, const, const Class&
    )

    // opCmp needs special logic to translate the result of operator<=> from C++

    std::string decl_opCmp() const
    {
        return string_concat("int opCmp(const ", m_name, "&in) const");
    }

    template <typename Class>
    int register_opCmp_generic()
    {
        using gen_t = detail::opCmp<Class>;
        return register_method(
            decl_opCmp(),
            gen_t::generate(detail::generic_cc),
            detail::generic_cc
        );
    }

    template <typename Class>
    int register_opCmp_native()
    {
        using gen_t = detail::opCmp<Class>;
        return register_method(
            decl_opCmp(),
            gen_t::generate(detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>),
            AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST
        );
    }

    ASBIND20_IMPL_REGISTER_BINARY_OP(
        opAdd, +, (m_name, " opAdd(const ", m_name, " &in) const"), Class, const, const Class&
    )
    ASBIND20_IMPL_REGISTER_BINARY_OP(
        opSub, -, (m_name, " opSub(const ", m_name, " &in) const"), Class, const, const Class&
    )
    ASBIND20_IMPL_REGISTER_BINARY_OP(
        opMul, *, (m_name, " opMul(const ", m_name, " &in) const"), Class, const, const Class&
    )
    ASBIND20_IMPL_REGISTER_BINARY_OP(
        opDiv, /, (m_name, " opDiv(const ", m_name, " &in) const"), Class, const, const Class&
    )
    ASBIND20_IMPL_REGISTER_BINARY_OP(
        opMod, %, (m_name, " opMod(const ", m_name, " &in) const"), Class, const, const Class&
    )

#undef ASBIND20_IMPL_REGISTER_BINARY_OP_GENERIC
#undef ASBIND20_IMPL_REGISTER_BINARY_OP_NATIVE
#undef ASBIND20_IMPL_REGISTER_BINARY_OP

    int register_member_funcdef(std::string_view decl) const
    {
        std::string full_decl = detail::generate_member_funcdef(
            m_name, decl
        );
        return register_full_funcdef(full_decl);
    }

    static std::string decl_opConv(std::string_view ret, bool implicit = false)
    {
        if(implicit)
            return string_concat(ret, " opImplConv() const");
        else
            return string_concat(ret, " opConv() const");
    }

    template <typename Class, typename To>
    int opConv_impl_native(std::string_view ret, bool implicit)
    {
        using gen_t = detail::opConv<std::add_const_t<Class>, To>;
        return register_method(
            decl_opConv(ret, implicit),
            gen_t::generate(
                detail::cdecl_last_cc
            ),
            AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
        );
    }

    template <typename Class, typename To>
    int opConv_impl_generic(std::string_view ret, bool implicit)
    {
        using gen_t = detail::opConv<std::add_const_t<Class>, To>;
        return register_method(
            decl_opConv(ret, implicit),
            gen_t::generate(detail::generic_cc),
            detail::generic_cc
        );
    }

    void register_as_string(
        AS_NAMESPACE_QUALIFIER asIStringFactory* factory
    )
    {
        [[maybe_unused]]
        int r = 0;
        r = get_engine()->RegisterStringFactory(get_name().c_str(), factory);
        ASBIND20_ASSERT(r >= 0);
    }

private:
    // Internal interface because AS doesn't provide a direct interface to register member funcdef
    int register_full_funcdef(cstring_ref decl) const
    {
        return get_engine()->RegisterFuncdef(decl.c_str());
    }
};

/**
 * @brief CRTP interface for binding generators of classes
 */
template <
    typename Derived,
    typename Class,
    bool Template,
    bool ForceGeneric,
    typename Listener = default_listener>
class class_binding_generator_interface :
    public class_binding_generator_base<ForceGeneric, Listener>
{
    using my_base = class_binding_generator_base<ForceGeneric, Listener>;
    using listener_traits_type = listener_traits<Listener>;

protected:
    using my_base::my_base;

    template <typename Method>
    static constexpr auto method_callconv() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        if constexpr(noncapturing_native_lambda<Method>)
            return detail::deduce_lambda_callconv<Class, Method>();
        else
            return detail::deduce_method_callconv<Class, Method>();
    }

    template <auto Method>
    static constexpr auto method_callconv() noexcept
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        return method_callconv<decltype(Method)>();
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
        return method_callconv_aux<decltype(Method), Auxiliary>();
    }

private:
    template <typename Fn>
    void register_temp_cb(
        Fn&& fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes conv,
        void* aux = nullptr
    )
    {
        this->register_behaviour(
            AS_NAMESPACE_QUALIFIER asBEHAVE_TEMPLATE_CALLBACK,
            "bool f(int&in,bool&out)",
            fn,
            conv,
            aux
        );
    }

    template <
        typename Fn,
        typename Auxiliary = void>
    static consteval auto deduce_temp_cb_cc()
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        if constexpr(std::is_void_v<Auxiliary>)
        {
            return detail::deduce_beh_callconv<
                AS_NAMESPACE_QUALIFIER asBEHAVE_TEMPLATE_CALLBACK,
                Class,
                Fn>();
        }
        else
        {
            return detail::deduce_beh_callconv_aux<
                AS_NAMESPACE_QUALIFIER asBEHAVE_TEMPLATE_CALLBACK,
                Class,
                Fn,
                Auxiliary>();
        }
    }

public:
    using my_base::get_listener;

    [[nodiscard]]
    static constexpr bool is_template_class() noexcept
    {
        return Template;
    }

    Derived& template_callback(
        generic_function gfn
    ) requires(Template)
    {
        this->register_temp_cb(gfn, detail::generic_cc);
        return derived();
    }

    template <typename Auxiliary>
    Derived& template_callback(
        generic_function gfn,
        auxiliary_wrapper<Auxiliary> aux
    ) requires(Template)
    {
        this->register_temp_cb(
            gfn,
            detail::generic_cc,
            this->get_auxiliary_address(aux)
        );
        return derived();
    }

    template <native_function Fn>
    Derived& template_callback(Fn&& fn) requires(Template && !ForceGeneric)
    {
        constexpr auto conv = deduce_temp_cb_cc<Fn>();
        this->register_temp_cb(fn, conv);
        return derived();
    }

    template <native_function Fn, typename Auxiliary>
    requires(std::is_member_function_pointer_v<Fn>)
    Derived& template_callback(
        Fn&& fn, auxiliary_wrapper<Auxiliary> aux
    ) requires(Template && !ForceGeneric)
    {
        constexpr auto conv = deduce_temp_cb_cc<Fn, Auxiliary>();
        this->register_temp_cb(
            fn, conv, this->get_auxiliary_address(aux)
        );
        return derived();
    }

    template <auto Callback>
    Derived& template_callback(use_generic_t, fp_wrapper<Callback>) requires(Template)
    {
        constexpr auto conv = deduce_temp_cb_cc<decltype(Callback)>();
        this->register_temp_cb(
            detail::to_asGENFUNC_t(fp<Callback>, detail::cc<conv>),
            detail::generic_cc
        );
        return derived();
    }

    template <auto Callback, typename Auxiliary>
    Derived& template_callback(
        use_generic_t, fp_wrapper<Callback>, auxiliary_wrapper<Auxiliary> aux
    ) requires(Template)
    {
        constexpr auto conv = deduce_temp_cb_cc<decltype(Callback), Auxiliary>();
        this->register_temp_cb(
            detail::to_asGENFUNC_t(fp<Callback>, detail::cc<conv>),
            detail::generic_cc,
            this->get_auxiliary_address(aux)
        );
        return derived();
    }

    template <auto Callback>
    Derived& template_callback(fp_wrapper<Callback>) requires(Template)
    {
        if constexpr(ForceGeneric)
            template_callback(use_generic, fp<Callback>);
        else
            template_callback(Callback);
        return derived();
    }

    template <auto Callback, typename Auxiliary>
    Derived& template_callback(
        fp_wrapper<Callback>, auxiliary_wrapper<Auxiliary> aux
    ) requires(Template)
    {
        if constexpr(ForceGeneric)
            template_callback(use_generic, fp<Callback>, aux);
        else
            template_callback(Callback, aux);
        return derived();
    }

    template <native_function Fn>
    Derived& method(
        cstring_ref decl,
        Fn&& fn
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = method_callconv<Fn>();
        int r = this->register_method(decl, fn, conv);
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <native_function Fn, bool ObjFirst>
    Derived& method(
        cstring_ref decl,
        Fn&& fn,
        obj_loc_t<ObjFirst>
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = detail::conv_of_loc(obj_loc<ObjFirst>, false);
        int r = this->register_method(
            decl,
            fn,
            conv
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    Derived& method(
        cstring_ref decl,
        generic_function gfn
    )
    {
        int r = this->register_method(
            decl,
            gfn,
            detail::generic_cc
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <
        native_function Fn,
        typename Auxiliary>
    Derived& method(
        cstring_ref decl,
        Fn&& fn,
        auxiliary_wrapper<Auxiliary> aux
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = method_callconv_aux<Fn, Auxiliary>();
        int r = this->register_method(
            decl,
            std::forward<Fn>(fn),
            conv,
            my_base::get_auxiliary_address(aux)
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <typename Auxiliary>
    Derived& method(
        cstring_ref decl,
        generic_function gfn,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        int r = this->register_method(
            decl,
            gfn,
            detail::generic_cc,
            my_base::get_auxiliary_address(aux)
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <auto Method>
    Derived& method(
        use_generic_t,
        cstring_ref decl,
        fp_wrapper<Method>
    )
    {
        constexpr auto conv = method_callconv<Method>();
        int r = this->register_method(
            decl,
            detail::to_asGENFUNC_t(fp<Method>, detail::cc<conv>),
            detail::generic_cc
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <auto Method>
    Derived& method(
        cstring_ref decl,
        fp_wrapper<Method>
    )
    {
        if constexpr(ForceGeneric)
            this->method(use_generic, decl, fp<Method>);
        else
            this->method(decl, Method);
        return derived();
    }

    template <auto Method, typename Auxiliary>
    Derived& method(
        use_generic_t,
        cstring_ref decl,
        fp_wrapper<Method>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        constexpr auto conv = method_callconv_aux<Method, Auxiliary>();
        int r = this->register_method(
            decl,
            detail::to_asGENFUNC_t(fp<Method>, detail::cc<conv>),
            detail::generic_cc,
            my_base::get_auxiliary_address(aux)
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <auto Method, typename Auxiliary>
    Derived& method(
        cstring_ref decl,
        fp_wrapper<Method>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        if constexpr(ForceGeneric)
            this->method(use_generic, decl, fp<Method>, aux);
        else
            this->method(decl, Method, aux);
        return derived();
    }

    template <auto Method, typename Auxiliary, bool ObjFirst>
    Derived& method(
        use_generic_t,
        cstring_ref decl,
        fp_wrapper<Method>,
        auxiliary_wrapper<Auxiliary> aux,
        obj_loc_t<ObjFirst>
    )
    {
        // For method with auxiliary object,
        // its calling convention should be THISCALL_OBJFIRST/LAST,
        // so we are getting convention with is_thiscall: true here.
        constexpr auto conv = detail::conv_of_loc(obj_loc<ObjFirst>, true);
        int r = this->register_method(
            decl,
            detail::to_asGENFUNC_t(fp<Method>, detail::cc<conv>),
            detail::generic_cc,
            my_base::get_auxiliary_address(aux)
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <auto Method, typename Auxiliary, bool ObjFirst>
    Derived& method(
        cstring_ref decl,
        fp_wrapper<Method>,
        auxiliary_wrapper<Auxiliary> aux,
        obj_loc_t<ObjFirst>
    )
    {
        if constexpr(ForceGeneric)
            this->method(use_generic, decl, fp<Method>, aux, obj_loc<ObjFirst>);
        else
        {
            // For method with auxiliary object,
            // its calling convention should be THISCALL_OBJFIRST/LAST,
            // so we are getting convention with is_thiscall: true here.
            constexpr auto conv = detail::conv_of_loc(obj_loc<ObjFirst>, true);
            int r = this->register_method(
                decl,
                Method,
                conv,
                my_base::get_auxiliary_address(aux)
            );
            listener_traits_type::on_method(get_listener(), derived(), r);
        }
        return derived();
    }

    template <noncapturing_native_lambda Lambda>
    Derived& method(
        use_generic_t,
        cstring_ref decl,
        const Lambda&
    )
    {
        constexpr auto conv = method_callconv<Lambda>();
        int r = this->register_method(
            decl,
            detail::to_asGENFUNC_t(Lambda{}, detail::cc<conv>),
            detail::generic_cc
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <noncapturing_native_lambda Lambda, bool ObjFirst>
    Derived& method(
        use_generic_t,
        cstring_ref decl,
        const Lambda&,
        obj_loc_t<ObjFirst>
    )
    {
        constexpr auto conv = detail::conv_of_loc(obj_loc<ObjFirst>, false);
        int r = this->register_method(
            decl,
            detail::to_asGENFUNC_t(Lambda{}, detail::cc<conv>),
            detail::generic_cc
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <noncapturing_native_lambda Lambda>
    Derived& method(
        cstring_ref decl,
        const Lambda&
    )
    {
        if constexpr(ForceGeneric)
            this->method(use_generic, decl, Lambda{});
        else
        {
            constexpr auto conv = method_callconv<Lambda>();
            int r = this->register_method(
                decl,
                +Lambda{},
                detail::cc<conv>
            );
            listener_traits_type::on_method(get_listener(), derived(), r);
        }
        return derived();
    }

    template <noncapturing_native_lambda Lambda, bool ObjFirst>
    Derived& method(
        cstring_ref decl,
        const Lambda&,
        obj_loc_t<ObjFirst>
    )
    {
        if constexpr(ForceGeneric)
            this->method(use_generic, decl, Lambda{}, obj_loc<ObjFirst>);
        else
        {
            constexpr auto conv = detail::conv_of_loc(obj_loc<ObjFirst>, false);
            int r = this->register_method(
                decl,
                detail::to_asGENFUNC_t(Lambda{}, detail::cc<conv>),
                detail::generic_cc
            );
            listener_traits_type::on_method(get_listener(), derived(), r);
        }
        return derived();
    }

    template <
        auto Function,
        std::size_t... Is>
    Derived& method(
        use_generic_t,
        cstring_ref decl,
        fp_wrapper<Function>,
        var_type_t<Is...>
    )
    {
        constexpr auto conv = method_callconv<Function>();
        int r = this->register_method(
            decl,
            detail::to_asGENFUNC_t(
                fp<Function>,
                detail::cc<conv>,
                var_type<Is...>
            ),
            detail::generic_cc
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <
        auto Function,
        std::size_t... Is>
    Derived& method(
        cstring_ref decl,
        fp_wrapper<Function>,
        var_type_t<Is...>
    )
    {
        if constexpr(ForceGeneric)
            this->method(use_generic, decl, fp<Function>, var_type<Is...>);
        else
            this->method(decl, Function);
        return derived();
    }

    template <
        auto Function,
        std::size_t... Is,
        typename Auxiliary>
    Derived& method(
        use_generic_t,
        cstring_ref decl,
        fp_wrapper<Function>,
        var_type_t<Is...>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        constexpr auto conv = method_callconv_aux<Function, Auxiliary>();
        int r = this->register_method(
            decl,
            detail::to_asGENFUNC_t(
                fp<Function>,
                detail::cc<conv>,
                var_type<Is...>
            ),
            detail::generic_cc,
            my_base::get_auxiliary_address(aux)
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <
        auto Function,
        std::size_t... Is,
        typename Auxiliary>
    Derived& method(
        cstring_ref decl,
        fp_wrapper<Function>,
        var_type_t<Is...>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        if constexpr(ForceGeneric)
            this->method(use_generic, decl, fp<Function>, var_type<Is...>, aux);
        else
        {
            constexpr auto conv = method_callconv_aux<Function, Auxiliary>();
            int r = this->register_method(
                decl,
                detail::to_asGENFUNC_t(
                    fp<Function>,
                    detail::cc<conv>,
                    var_type<Is...>
                ),
                detail::generic_cc,
                my_base::get_auxiliary_address(aux)
            );
            listener_traits_type::on_method(get_listener(), derived(), r);
        }
        return derived();
    }

    template <
        noncapturing_native_lambda Lambda,
        std::size_t... Is>
    Derived& method(
        use_generic_t,
        cstring_ref decl,
        const Lambda&,
        var_type_t<Is...>
    )
    {
        constexpr auto conv = method_callconv<Lambda>();
        int r = this->register_method(
            decl,
            detail::to_asGENFUNC_t(
                Lambda{},
                detail::cc<conv>,
                var_type<Is...>
            ),
            detail::generic_cc
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <
        noncapturing_native_lambda Lambda,
        std::size_t... Is>
    Derived& method(
        cstring_ref decl,
        const Lambda&,
        var_type_t<Is...>
    )
    {
        constexpr auto conv = method_callconv<Lambda>();
        if constexpr(ForceGeneric)
            this->method(use_generic, decl, Lambda{}, var_type<Is...>);
        else
        {
            int r = this->register_method(
                decl,
                +Lambda{},
                detail::cc<conv>
            );
            listener_traits_type::on_method(get_listener(), derived(), r);
        }
        return derived();
    }

    template <native_function Fn>
    requires(std::is_member_function_pointer_v<Fn>)
    Derived& method(
        cstring_ref decl,
        Fn fn,
        composite_wrapper comp
    ) requires(!ForceGeneric)
    {
        this->register_comp_method(
            decl,
            fn,
            detail::cc<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>,
            comp
        );
        return derived();
    }

    template <auto Fn, auto Composite>
    requires(std::is_member_function_pointer_v<decltype(Fn)>)
    Derived& method(
        use_generic_t,
        cstring_ref decl,
        fp_wrapper<Fn>,
        composite_wrapper_nontype<Composite>
    )
    {
        int r = this->register_method(
            decl,
            // The composite offset will be handled by the generic wrapper
            detail::to_asGENFUNC_t(
                fp<Fn>,
                detail::cc<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>,
                composite_wrapper_nontype<Composite>{}
            ),
            detail::generic_cc
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <auto Fn, auto Composite>
    requires(std::is_member_function_pointer_v<decltype(Fn)>)
    Derived& method(
        cstring_ref decl,
        fp_wrapper<Fn>,
        composite_wrapper_nontype<Composite>
    )
    {
        if constexpr(ForceGeneric)
        {
            this->method(
                use_generic,
                decl,
                fp<Fn>,
                composite_wrapper_nontype<Composite>{}
            );
        }
        else
        {
            this->method(
                decl,
                Fn,
                composite(Composite)
            );
        }
        return derived();
    }

    template <auto Fn, auto Composite, std::size_t... Is>
    requires(std::is_member_function_pointer_v<decltype(Fn)>)
    Derived& method(
        use_generic_t,
        cstring_ref decl,
        fp_wrapper<Fn>,
        composite_wrapper_nontype<Composite>,
        var_type_t<Is...>
    )
    {
        int r = this->register_method(
            decl,
            detail::to_asGENFUNC_t(
                fp<Fn>,
                detail::cc<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>,
                composite_wrapper_nontype<Composite>{},
                var_type<Is...>
            ),
            detail::generic_cc
        );
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <auto Fn, auto Composite, std::size_t... Is>
    requires(std::is_member_function_pointer_v<decltype(Fn)>)
    Derived& method(
        cstring_ref decl,
        fp_wrapper<Fn>,
        composite_wrapper_nontype<Composite>,
        var_type_t<Is...>
    )
    {
        if constexpr(ForceGeneric)
        {
            this->method(
                use_generic,
                decl,
                fp<Fn>,
                composite_wrapper_nontype<Composite>{},
                var_type<Is...>
            );
        }
        else
        { // Native calling convention doesn't need the `var_type` tag.
            this->method(
                decl,
                Fn,
                composite(Composite)
            );
        }
        return derived();
    }

    Derived& property(cstring_ref decl, std::size_t off)
    {
        this->register_property(decl, off);
        return derived();
    }

    template <typename MemberPointer>
    requires(std::is_member_object_pointer_v<MemberPointer>)
    Derived& property(cstring_ref decl, MemberPointer mp)
    {
        this->template register_property<MemberPointer>(decl, mp);
        return derived();
    }

    Derived& property(
        cstring_ref decl, std::size_t off, composite_wrapper comp
    )
    {
        this->register_comp_property(decl, off, comp.get_offset());
        return derived();
    }

    template <typename MemberPointer>
    requires(std::is_member_object_pointer_v<MemberPointer>)
    Derived& property(cstring_ref decl, MemberPointer mp, composite_wrapper comp)
    {
        this->register_comp_property(decl, mp, comp.get_offset());
        return derived();
    }

    Derived& funcdef(cstring_ref decl)
    {
        int r = this->register_member_funcdef(decl);
        listener_traits_type::on_funcdef(
            this->get_listener(), *this, r
        );
        return derived();
    }

    Derived& as_string(
        AS_NAMESPACE_QUALIFIER asIStringFactory* str_factory
    )
    {
        this->register_as_string(str_factory);
        return derived();
    }

    template <usable_by_generator<Derived> Usable>
    Derived& use(Usable&& u)
    {
        u(derived());
        return derived();
    }

    template <typename To>
    Derived& opConv(use_generic_t, std::string_view to_decl)
    {
        int r = this->template opConv_impl_generic<Class, To>(to_decl, false);
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <typename To>
    Derived& opConv(std::string_view to_decl)
    {
        if constexpr(ForceGeneric)
            opConv<To>(use_generic, to_decl);
        else
        {
            int r = this->template opConv_impl_native<Class, To>(to_decl, false);
            listener_traits_type::on_method(get_listener(), derived(), r);
        }
        return derived();
    }

    template <typename To>
    Derived& opImplConv(use_generic_t, std::string_view to_decl)
    {
        int r = this->template opConv_impl_generic<Class, To>(to_decl, true);
        listener_traits_type::on_method(get_listener(), derived(), r);
        return derived();
    }

    template <typename To>
    Derived& opImplConv(std::string_view to_decl)
    {
        if constexpr(ForceGeneric)
            opImplConv<To>(use_generic, to_decl);
        else
        {
            int r = this->template opConv_impl_native<Class, To>(to_decl, true);
            listener_traits_type::on_method(get_listener(), derived(), r);
        }
        return derived();
    }

    template <has_static_name To>
    Derived& opConv(use_generic_t)
    {
        opConv<To>(use_generic, name_of<To>());
        return derived();
    }

    template <has_static_name To>
    Derived& opConv()
    {
        opConv<To>(name_of<To>());
        return derived();
    }

    template <has_static_name To>
    Derived& opImplConv(use_generic_t)
    {
        opImplConv<To>(use_generic, name_of<To>());
        return derived();
    }

    template <has_static_name To>
    Derived& opImplConv()
    {
        opImplConv<To>(name_of<To>());
        return derived();
    }

#define ASBIND20_BG_INTERFACE_DEFINE_OP(bg_type, op_name)                \
    bg_type& op_name(use_generic_t)                                      \
    {                                                                    \
        int r = this->template register_##op_name##_generic<Class>();    \
        listener_traits_type::on_method(                                 \
            this->get_listener(), static_cast<bg_type&>(*this), r        \
        );                                                               \
        return static_cast<bg_type&>(*this);                             \
    }                                                                    \
    bg_type& op_name()                                                   \
    {                                                                    \
        if constexpr(ForceGeneric)                                       \
            this->op_name(use_generic);                                  \
        else                                                             \
        {                                                                \
            int r = this->template register_##op_name##_native<Class>(); \
            listener_traits_type::on_method(                             \
                this->get_listener(), static_cast<bg_type&>(*this), r    \
            );                                                           \
        }                                                                \
        return static_cast<bg_type&>(*this);                             \
    }

    // The following operators are returning either primitives
    // or reference of type being registered.
    // It's safe to have them available for both value and reference classes.

    // FIXME: Handle primitive types
    ASBIND20_BG_INTERFACE_DEFINE_OP(Derived, opAssign)
    ASBIND20_BG_INTERFACE_DEFINE_OP(Derived, opAddAssign)
    ASBIND20_BG_INTERFACE_DEFINE_OP(Derived, opSubAssign)
    ASBIND20_BG_INTERFACE_DEFINE_OP(Derived, opMulAssign)
    ASBIND20_BG_INTERFACE_DEFINE_OP(Derived, opDivAssign)
    ASBIND20_BG_INTERFACE_DEFINE_OP(Derived, opModAssign)

    ASBIND20_BG_INTERFACE_DEFINE_OP(Derived, opEquals)
    ASBIND20_BG_INTERFACE_DEFINE_OP(Derived, opCmp)

    ASBIND20_BG_INTERFACE_DEFINE_OP(Derived, opPreInc)
    ASBIND20_BG_INTERFACE_DEFINE_OP(Derived, opPreDec)

private:
    template <
        typename Fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
    static consteval bool check_opCmp_sig()
    {
#ifndef ASBIND20_CONFIG_NO_COMPILE_TIME_CHECKS
        // TODO: Complete this
        return true;

#else
        return true;
#endif
    }

public:
    template <auto Comparator>
    Derived& opCmp(use_generic_t, fp_wrapper<Comparator>)
    {
        constexpr auto conv = detail::deduce_method_callconv<Class, Comparator>();
        static_assert(
            check_opCmp_sig<decltype(Comparator), conv>(),
            "Invalid signature for opCmp"
        );
        int r = this->register_method(
            this->decl_opCmp(),
            detail::to_asGENFUNC_t(fp<Comparator>, detail::cc<conv>),
            detail::generic_cc
        );
        listener_traits_type::on_method(
            this->get_listener(), derived(), r
        );
        return derived();
    }

    template <auto Comparator>
    Derived& opCmp(fp_wrapper<Comparator>)
    {
        if constexpr(ForceGeneric)
            this->opCmp(use_generic, fp<Comparator>);
        else
        {
            constexpr auto conv = detail::deduce_method_callconv<Class, Comparator>();
            static_assert(
                check_opCmp_sig<decltype(Comparator), conv>(),
                "Invalid signature for opCmp"
            );
            int r = this->register_method(
                this->decl_opCmp(),
                Comparator,
                conv
            );
            listener_traits_type::on_method(
                this->get_listener(), derived(), r
            );
        }
        return derived();
    }

    template <noncapturing_native_lambda Comparator>
    Derived& opCmp(const Comparator&)
    {
        constexpr auto conv = detail::deduce_lambda_callconv<Class, Comparator>();
        static_assert(
            check_opCmp_sig<decltype(+Comparator{}), conv>(),
            "Invalid signature for opCmp"
        );
        int r = this->register_method(
            this->decl_opCmp(),
            detail::to_asGENFUNC_t(Comparator{}, detail::cc<conv>),
            detail::generic_cc
        );
        listener_traits_type::on_method(
            this->get_listener(), derived(), r
        );

        return derived();
    }

#define ASBIND20_BG_INTERFACE_DEFINE_BEH(bg_type, as_beh, func_name)  \
    template <native_function Fn>                                     \
    bg_type& func_name(Fn&& fn) requires(!ForceGeneric)               \
    {                                                                 \
        constexpr auto conv = detail::deduce_beh_callconv<            \
            AS_NAMESPACE_QUALIFIER as_beh,                            \
            Class,                                                    \
            Fn>();                                                    \
        static_assert(                                                \
            detail::match_beh_sig<AS_NAMESPACE_QUALIFIER as_beh, Fn>( \
                asbind20::detail::cc<conv>                            \
            ),                                                        \
            "Invalid signature for behaviour"                         \
        );                                                            \
        this->register_behaviour(                                     \
            AS_NAMESPACE_QUALIFIER as_beh,                            \
            detail::decl_of_beh<AS_NAMESPACE_QUALIFIER as_beh>(),     \
            fn,                                                       \
            conv                                                      \
        );                                                            \
        return static_cast<bg_type&>(*this);                          \
    }                                                                 \
    template <native_function Fn, typename Auxiliary>                 \
    bg_type& func_name(                                               \
        Fn&& fn, auxiliary_wrapper<Auxiliary> aux                     \
    ) requires(!ForceGeneric)                                         \
    {                                                                 \
        constexpr auto conv = detail::deduce_beh_callconv_aux<        \
            AS_NAMESPACE_QUALIFIER as_beh,                            \
            Class,                                                    \
            Fn,                                                       \
            Auxiliary>();                                             \
        this->register_behaviour(                                     \
            AS_NAMESPACE_QUALIFIER as_beh,                            \
            detail::decl_of_beh<AS_NAMESPACE_QUALIFIER as_beh>(),     \
            fn,                                                       \
            conv,                                                     \
            this->get_auxiliary_address(aux)                          \
        );                                                            \
        return static_cast<bg_type&>(*this);                          \
    }                                                                 \
    bg_type& func_name(generic_function gfn)                          \
    {                                                                 \
        this->register_behaviour(                                     \
            AS_NAMESPACE_QUALIFIER as_beh,                            \
            detail::decl_of_beh<AS_NAMESPACE_QUALIFIER as_beh>(),     \
            gfn,                                                      \
            detail::generic_cc                                        \
        );                                                            \
        return static_cast<bg_type&>(*this);                          \
    }                                                                 \
    template <typename Auxiliary>                                     \
    bg_type& func_name(                                               \
        generic_function gfn, auxiliary_wrapper<Auxiliary> aux        \
    )                                                                 \
    {                                                                 \
        this->register_behaviour(                                     \
            AS_NAMESPACE_QUALIFIER as_beh,                            \
            detail::decl_of_beh<AS_NAMESPACE_QUALIFIER as_beh>(),     \
            gfn,                                                      \
            detail::generic_cc,                                       \
            this->get_auxiliary_address(aux)                          \
        );                                                            \
        return static_cast<bg_type&>(*this);                          \
    }                                                                 \
    template <auto Function>                                          \
    bg_type& func_name(use_generic_t, fp_wrapper<Function>)           \
    {                                                                 \
        using func_t = decltype(Function);                            \
        constexpr auto conv = detail::deduce_beh_callconv<            \
            AS_NAMESPACE_QUALIFIER as_beh,                            \
            Class,                                                    \
            func_t>();                                                \
        this->func_name(                                              \
            detail::to_asGENFUNC_t(fp<Function>, detail::cc<conv>)    \
        );                                                            \
        return static_cast<bg_type&>(*this);                          \
    }                                                                 \
    template <auto Function, typename Auxiliary>                      \
    bg_type& func_name(                                               \
        use_generic_t,                                                \
        fp_wrapper<Function>,                                         \
        auxiliary_wrapper<Auxiliary> aux                              \
    )                                                                 \
    {                                                                 \
        using func_t = decltype(Function);                            \
        constexpr auto conv = detail::deduce_beh_callconv_aux<        \
            AS_NAMESPACE_QUALIFIER as_beh,                            \
            Class,                                                    \
            func_t,                                                   \
            Auxiliary>();                                             \
        this->func_name(                                              \
            detail::to_asGENFUNC_t(fp<Function>, detail::cc<conv>),   \
            aux                                                       \
        );                                                            \
        return static_cast<bg_type&>(*this);                          \
    }                                                                 \
    template <auto Function>                                          \
    bg_type& func_name(fp_wrapper<Function>)                          \
    {                                                                 \
        if constexpr(ForceGeneric)                                    \
            this->func_name(use_generic, fp<Function>);               \
        else                                                          \
            this->func_name(Function);                                \
        return static_cast<bg_type&>(*this);                          \
    }                                                                 \
    template <auto Function, typename Auxiliary>                      \
    bg_type& func_name(                                               \
        fp_wrapper<Function>, auxiliary_wrapper<Auxiliary> aux        \
    )                                                                 \
    {                                                                 \
        if constexpr(ForceGeneric)                                    \
            this->func_name(use_generic, fp<Function>, aux);          \
        else                                                          \
            this->func_name(Function, aux);                           \
        return static_cast<bg_type&>(*this);                          \
    }

    // Reference types with GC flag support all GC-related behaviours.
    // For garbage collected value types,
    // please check the official document:
    // https://www.angelcode.com/angelscript/sdk/docs/manual/doc_gc_object.html#doc_reg_gcref_value

    ASBIND20_BG_INTERFACE_DEFINE_BEH(Derived, asBEHAVE_ENUMREFS, enum_refs)
    ASBIND20_BG_INTERFACE_DEFINE_BEH(Derived, asBEHAVE_RELEASEREFS, release_refs)

    // #undef these macros at the end of this file
    // because the following generators will use them.

private:
    Derived& derived() noexcept
    {
        return static_cast<Derived&>(*this);
    }
};

/**
 * @brief Register helper for value class
 *
 * @tparam Class Class to be registered
 * @tparam Template True if the class is a templated type
 * @tparam ForceGeneric Force all registered methods and behaviours to use the generic calling convention
 * @tparam Listener The listener type
 */
template <
    typename Class,
    bool Template = false,
    bool ForceGeneric = false,
    typename Listener = default_listener>
class basic_value_class final :
    public class_binding_generator_interface<
        basic_value_class<Class, Template, ForceGeneric, Listener>,
        Class,
        Template,
        ForceGeneric,
        Listener>
{
    using my_base = class_binding_generator_interface<
        basic_value_class<Class, Template, ForceGeneric, Listener>,
        Class,
        Template,
        ForceGeneric,
        Listener>;
    using listener_traits_type = listener_traits<Listener>;

public:
    using flag_type = AS_NAMESPACE_QUALIFIER asQWORD;
    using class_type = Class;
    using class_binding_generator_tag = void;

    basic_value_class(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        std::string name,
        flag_type flags = 0
    )
        : my_base(engine, std::move(name))
    {
        flags |= AS_NAMESPACE_QUALIFIER asOBJ_VALUE;

        ASBIND20_ASSERT(!(flags & (AS_NAMESPACE_QUALIFIER asOBJ_REF)));

        if constexpr(!Template)
        {
            ASBIND20_ASSERT(!(flags & (AS_NAMESPACE_QUALIFIER asOBJ_TEMPLATE)));
            flags |= AS_NAMESPACE_QUALIFIER asGetTypeTraits<Class>();
        }
        else
        {
            flags |= AS_NAMESPACE_QUALIFIER asOBJ_TEMPLATE;
        }

        int r = this->register_object_type(
            flags, static_cast<int>(sizeof(Class))
        );
        listener_traits_type::on_class(
            this->get_listener(), *this, r
        );
    }

    template <string_like StringLike>
    basic_value_class(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        StringLike&& name,
        AS_NAMESPACE_QUALIFIER asQWORD flags = 0
    )
        : basic_value_class(
              engine,
              util::string_like_to_string(std::forward<StringLike>(name)),
              flags
          )
    {}

    basic_value_class(
        appending_t<true>,
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        std::string name
    )
        : my_base(engine, std::move(name))
    {
        this->append_type(true, 0, 0);
    }

    template <string_like StringLike>
    basic_value_class(
        appending_t<true>,
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        StringLike&& name
    )
        : basic_value_class(
              appending_t<true>{},
              engine,
              util::string_like_to_string(std::forward<StringLike>(name))
          )
    {}

    basic_value_class(const basic_value_class&) = default;

    basic_value_class& operator=(const basic_value_class&) = delete;

    using my_base::get_name;

private:
    static std::string decl_ctor(std::string_view params)
    {
        if constexpr(Template)
        {
            return params.empty() ?
                       "void f(int&in)" :
                       string_concat("void f(int&in,", params, ')');
        }
        else
        {
            return params.empty() ?
                       "void f()" :
                       string_concat("void f(", params, ')');
        }
    }

    static std::string decl_ctor(std::string_view params, use_explicit_t)
    {
        if constexpr(Template)
        {
            return params.empty() ?
                       "void f(int&in)explicit" :
                       string_concat("void f(int&in,", params, ")explicit");
        }
        else
        {
            return params.empty() ?
                       "void f()explicit" :
                       string_concat("void f(", params, ")explicit");
        }
    }

    static std::string decl_ctor(std::string_view params, bool explicit_)
    {
        return explicit_ ?
                   decl_ctor(params, use_explicit) :
                   decl_ctor(params);
    }

    template <typename Fn>
    void register_constructor_function(
        bool explicit_,
        std::string_view params,
        Fn&& fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes conv,
        void* aux = nullptr
    )
    {
        this->register_behaviour(
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            decl_ctor(params, explicit_),
            std::forward<Fn>(fn),
            conv,
            aux
        );
    }

public:
    basic_value_class& constructor_function(
        std::string_view params,
        generic_function gfn
    )
    {
        this->register_constructor_function(
            false,
            params,
            gfn,
            detail::generic_cc
        );

        return *this;
    }

    basic_value_class& constructor_function(
        std::string_view params,
        use_explicit_t,
        generic_function gfn
    )
    {
        this->register_constructor_function(
            true,
            params,
            gfn,
            detail::generic_cc
        );

        return *this;
    }

    template <native_function ConstructorFunc>
    basic_value_class& constructor_function(
        std::string_view params,
        ConstructorFunc ctor
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = detail::deduce_beh_callconv<
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            Class,
            ConstructorFunc>();
        this->register_constructor_function(
            false,
            params,
            ctor,
            conv
        );

        return *this;
    }

    template <native_function ConstructorFunc>
    basic_value_class& constructor_function(
        std::string_view params,
        use_explicit_t,
        ConstructorFunc ctor
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = detail::deduce_beh_callconv<
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            Class,
            ConstructorFunc>();
        this->register_constructor_function(
            true,
            params,
            ctor,
            conv
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
        constexpr auto conv = detail::deduce_beh_callconv<
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            Class,
            decltype(ConstructorFunc)>();
        this->register_constructor_function(
            false,
            params,
            detail::constructor_to_asGENFUNC_t<Class, Template>(
                fp<ConstructorFunc>,
                detail::cc<conv>
            ),
            detail::generic_cc
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
        constexpr auto conv = detail::deduce_beh_callconv<
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            Class,
            decltype(ConstructorFunc)>();
        this->register_constructor_function(
            true,
            params,
            detail::constructor_to_asGENFUNC_t<Class, Template>(
                fp<ConstructorFunc>,
                detail::cc<conv>
            ),
            detail::generic_cc
        );

        return *this;
    }

    template <auto Constructor>
    basic_value_class& constructor_function(
        std::string_view params,
        fp_wrapper<Constructor>
    )
    {
        if constexpr(ForceGeneric)
            this->constructor_function(use_generic, params, fp<Constructor>);
        else
            this->constructor_function(params, Constructor);
        return *this;
    }

    template <auto Constructor>
    basic_value_class& constructor_function(
        std::string_view params,
        use_explicit_t,
        fp_wrapper<Constructor>
    )
    {
        if constexpr(ForceGeneric)
            this->constructor_function(use_generic, params, use_explicit, fp<Constructor>);
        else
            this->constructor_function(params, use_explicit, fp<Constructor>);
        return *this;
    }

    template <noncapturing_native_lambda ConstructorFunc>
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        const ConstructorFunc&
    )
    {
        constexpr auto conv = detail::deduce_beh_callconv<
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            Class,
            decltype(+ConstructorFunc{})>();
        this->register_constructor_function(
            false,
            params,
            detail::constructor_to_asGENFUNC_t<Class, Template>(
                ConstructorFunc{}, detail::cc<conv>
            ),
            detail::generic_cc
        );

        return *this;
    }

    template <noncapturing_native_lambda ConstructorFunc>
    basic_value_class& constructor_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        const ConstructorFunc&
    )
    {
        constexpr auto conv = detail::deduce_beh_callconv<
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            Class,
            decltype(+ConstructorFunc{})>();
        this->register_constructor_function(
            true,
            params,
            detail::constructor_to_asGENFUNC_t<Class, Template>(
                ConstructorFunc{}, detail::cc<conv>
            ),
            detail::generic_cc
        );

        return *this;
    }

    template <noncapturing_native_lambda ConstructorFunc>
    basic_value_class& constructor_function(
        std::string_view params,
        const ConstructorFunc&
    )
    {
        if constexpr(ForceGeneric)
            this->constructor_function(use_generic, params, ConstructorFunc{});
        else
            this->constructor_function(params, +ConstructorFunc{});
        return *this;
    }

    template <noncapturing_native_lambda ConstructorFunc>
    basic_value_class& constructor_function(
        std::string_view params,
        use_explicit_t,
        const ConstructorFunc&
    )
    {
        if constexpr(ForceGeneric)
            this->constructor_function(use_generic, params, use_explicit, ConstructorFunc{});
        else
            this->constructor_function(params, use_explicit, +ConstructorFunc{});

        return *this;
    }

private:
    template <typename... Args>
    void register_constructor_generic(std::string_view params, bool explicit_)
    {
        detail::constructor<Class, Template, Args...> wrapper;
        this->register_constructor_function(
            explicit_,
            params,
            wrapper.generate(detail::generic_cc),
            detail::generic_cc
        );
    }

    template <typename... Args>
    void register_constructor_native(std::string_view params, bool explicit_)
    {
        detail::constructor<Class, Template, Args...> wrapper;
        this->register_constructor_function(
            explicit_,
            params,
            wrapper.generate(
                detail::cdecl_last_cc
            ),
            detail::cdecl_last_cc
        );
    }

    template <typename... Args>
    static consteval bool check_constructible(bool has_hidden_arg = Template)
    {
        return has_hidden_arg ?
                   meta::placement_newable_from<Class, AS_NAMESPACE_QUALIFIER asITypeInfo*, Args...> :
                   meta::placement_newable_from<Class, Args...>;
    }

public:
    template <typename... Args>
    basic_value_class& constructor(
        use_generic_t,
        std::string_view params
    ) requires(check_constructible<Args...>())
    {
        this->template register_constructor_generic<Args...>(params, false);
        return *this;
    }

    template <typename... Args>
    basic_value_class& constructor(
        use_generic_t,
        std::string_view params,
        use_explicit_t
    ) requires(check_constructible<Args...>())
    {
        this->register_constructor_generic<Args...>(params, true);
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
            this->register_constructor_native<Args...>(params, false);
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
            this->register_constructor_native<Args...>(params, true);
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
        this->register_behaviour(
            AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
            decl_default_ctor(),
            gen_t::generate(detail::generic_cc),
            detail::generic_cc,
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
            this->register_behaviour(
                AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
                decl_default_ctor(),
                gen_t::generate(detail::cdecl_last_cc),
                detail::cdecl_last_cc,
                nullptr
            );
        }

        return *this;
    }

private:
    std::string decl_copy_ctor() const
    {
        return string_concat("void f(const ", get_name(), "&in)");
    }

public:
    basic_value_class& copy_constructor(use_generic_t)
    {
        if constexpr(std::is_array_v<Class>)
        {
            using gen_t = detail::arr_copy_constructor<Class, const Class&>;
            this->register_behaviour(
                AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
                decl_copy_ctor(),
                gen_t::generate(detail::generic_cc),
                detail::generic_cc
            );
        }
        else
        {
            constructor<const Class&>(
                use_generic,
                string_concat("const ", get_name(), " &in")
            );
        }

        return *this;
    }

    basic_value_class& copy_constructor()
    {
        if constexpr(ForceGeneric)
            this->copy_constructor(use_generic);
        else
        {
            if constexpr(std::is_array_v<Class>)
            {
                using gen_t = detail::arr_copy_constructor<Class, const Class&>;
                this->register_behaviour(
                    AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT,
                    decl_copy_ctor(),
                    gen_t::generate(detail::cdecl_last_cc),
                    detail::cdecl_last_cc
                );
            }
            else
            {
                constructor<const Class&>(
                    string_concat("const ", get_name(), " &in")
                );
            }
        }

        return *this;
    }

private:
    template <typename Fn>
    void register_destructor_fn(
        Fn fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes conv,
        void* aux = nullptr
    )
    {
        this->register_behaviour(
            AS_NAMESPACE_QUALIFIER asBEHAVE_DESTRUCT,
            "void f()",
            fn,
            conv,
            aux
        );
    }

    template <typename FuncSig, typename Auxiliary = void>
    static consteval auto deduce_dtor_cc()
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        if constexpr(std::is_void_v<Auxiliary>)
        {
            return detail::deduce_beh_callconv<
                AS_NAMESPACE_QUALIFIER asBEHAVE_DESTRUCT,
                Class,
                FuncSig>();
        }
        else
        {
            return detail::deduce_beh_callconv_aux<
                AS_NAMESPACE_QUALIFIER asBEHAVE_DESTRUCT,
                Class,
                FuncSig,
                Auxiliary>();
        }
    }

public:
    basic_value_class& destructor(use_generic_t)
    {
        this->register_destructor_fn(
            +[](generic_pointer gen) -> void
            {
                std::destroy_at(get_generic_object<Class*>(gen));
            },
            detail::generic_cc
        );

        return *this;
    }

    basic_value_class& destructor()
    {
        if constexpr(ForceGeneric)
            destructor(use_generic);
        else
        {
            this->register_destructor_fn(
                +[](Class* ptr) -> void
                {
                    std::destroy_at(ptr);
                },
                detail::cdecl_last_cc
            );
        }

        return *this;
    }

    basic_value_class& destructor_function(generic_function destroyer)
    {
        this->register_destructor_fn(
            destroyer, detail::generic_cc
        );
        return *this;
    }

    template <typename Auxiliary>
    basic_value_class& destructor_function(
        generic_function destroyer,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        this->register_destructor_fn(
            destroyer,
            detail::generic_cc,
            this->get_auxiliary_address(aux)
        );
        return *this;
    }

    template <native_function DestructorFunc>
    basic_value_class& destructor_function(DestructorFunc destroyer)
        requires(!ForceGeneric)
    {
        constexpr auto conv = deduce_dtor_cc<DestructorFunc>();
        this->register_destructor_fn(
            destroyer, conv
        );
        return *this;
    }

    template <native_function DestructorFunc, typename Auxiliary>
    basic_value_class& destructor_function(
        DestructorFunc destroyer,
        auxiliary_wrapper<Auxiliary> aux
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = deduce_dtor_cc<
            DestructorFunc,
            Auxiliary>();
        this->register_destructor_fn(
            destroyer,
            conv,
            this->get_auxiliary_address(aux)
        );
        return *this;
    }

    template <auto DestructorFunc>
    basic_value_class& destructor_function(
        use_generic_t,
        fp_wrapper<DestructorFunc>
    )
    {
        constexpr auto conv = deduce_dtor_cc<decltype(DestructorFunc)>();
        this->register_destructor_fn(
            detail::to_asGENFUNC_t(fp<DestructorFunc>, detail::cc<conv>),
            detail::generic_cc
        );
        return *this;
    }

    template <auto DestructorFunc, typename Auxiliary>
    basic_value_class& destructor_function(
        use_generic_t,
        fp_wrapper<DestructorFunc>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        constexpr auto conv = deduce_dtor_cc<
            decltype(DestructorFunc),
            Auxiliary>();
        this->register_destructor_fn(
            detail::to_asGENFUNC_t(fp<DestructorFunc>, detail::cc<conv>),
            detail::generic_cc,
            this->get_auxiliary_address(aux)
        );
        return *this;
    }

    template <auto DestructorFunc>
    basic_value_class& destructor_function(
        fp_wrapper<DestructorFunc>
    )
    {
        if constexpr(ForceGeneric)
            this->destructor_function(use_generic, fp<DestructorFunc>);
        else
            this->destructor_function(DestructorFunc);
        return *this;
    }

    template <auto DestructorFunc, typename Auxiliary>
    basic_value_class& destructor_function(
        fp_wrapper<DestructorFunc>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        if constexpr(ForceGeneric)
            this->destructor_function(use_generic, fp<DestructorFunc>, aux);
        else
            this->destructor_function(DestructorFunc, aux);
        return *this;
    }

    template <noncapturing_native_lambda DestructorFunc>
    basic_value_class& destructor_function(
        use_generic_t,
        const DestructorFunc&
    )
    {
        constexpr auto conv = deduce_dtor_cc<decltype(+DestructorFunc{})>();
        this->register_destructor_fn(
            detail::to_asGENFUNC_t(DestructorFunc{}, detail::cc<conv>),
            detail::generic_cc
        );
        return *this;
    }

    template <noncapturing_native_lambda DestructorFunc>
    basic_value_class& destructor_function(
        const DestructorFunc&
    )
    {
        if constexpr(ForceGeneric)
            this->destructor_function(use_generic, DestructorFunc{});
        else
            this->destructor_function(+DestructorFunc{});
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
            if constexpr(meta::placement_newable_from<Class>)
                default_constructor(use_generic);
            else
                ASBIND20_ASSERT(false && "missing default constructor");
        }
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_D))
        {
            if constexpr(std::is_destructible_v<Class>)
                destructor(use_generic);
            else
                ASBIND20_ASSERT(false && "missing destructor");
        }
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_A))
        {
            if constexpr(std::is_copy_assignable_v<Class>)
                this->opAssign(use_generic);
            else
                ASBIND20_ASSERT(false && "missing assignment operator");
        }
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_K))
        {
            if constexpr(meta::placement_newable_from<Class, const Class&>)
                copy_constructor(use_generic);
            else
                ASBIND20_ASSERT(false && "missing copy constructor");
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
            if constexpr(meta::placement_newable_from<Class>)
                default_constructor();
            else
                ASBIND20_ASSERT(false && "missing default constructor");
        }
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_D))
        {
            if constexpr(std::is_destructible_v<Class>)
                destructor();
            else
                ASBIND20_ASSERT(false && "missing destructor");
        }
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_A))
        {
            if constexpr(std::is_copy_assignable_v<Class>)
                this->opAssign();
            else
                ASBIND20_ASSERT(false && "missing assignment operator");
        }
        if(traits & (AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_K))
        {
            if constexpr(meta::placement_newable_from<Class, const Class&>)
                copy_constructor();
            else
                ASBIND20_ASSERT(false && "missing copy constructor");
        }

        return *this;
    }

private:
    static constexpr std::string decl_list_constructor(std::string_view pattern)
    {
        if constexpr(Template)
            return string_concat("void f(int&in,int&in){", pattern, '}');
        else
            return string_concat("void f(int&in){", pattern, '}');
    }

    template <typename Fn>
    void register_list_ctor_func(
        std::string_view pattern,
        Fn&& fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes conv,
        void* aux = nullptr
    )
    {
        this->register_behaviour(
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_CONSTRUCT,
            decl_list_constructor(pattern),
            fn,
            conv,
            aux
        );
    }

public:
    basic_value_class& list_constructor_function(
        std::string_view pattern,
        generic_function gfn
    )
    {
        this->register_list_ctor_func(
            pattern,
            gfn,
            detail::generic_cc
        );

        return *this;
    }

    template <native_function ListConstructorFunc>
    basic_value_class& list_constructor_function(
        std::string_view pattern,
        ListConstructorFunc&& ctor
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = detail::deduce_beh_callconv<
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_CONSTRUCT,
            Class,
            ListConstructorFunc>();
        this->register_list_ctor_func(
            pattern, ctor, conv
        );

        return *this;
    }

    template <auto ListConstructorFunc>
    basic_value_class& list_constructor_function(
        use_generic_t,
        std::string_view pattern,
        fp_wrapper<ListConstructorFunc>
    )
    {
        constexpr auto conv = detail::deduce_beh_callconv<
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_CONSTRUCT,
            Class,
            decltype(ListConstructorFunc)>();
        this->register_list_ctor_func(
            pattern,
            detail::list_constructor_to_asGENFUNC_t<Class, Template>(
                fp<ListConstructorFunc>, detail::cc<conv>
            ),
            detail::generic_cc
        );

        return *this;
    }

    template <auto ListConstructorFunc>
    basic_value_class& list_constructor_function(
        std::string_view pattern,
        fp_wrapper<ListConstructorFunc>
    )
    {
        if constexpr(ForceGeneric)
            this->list_constructor_function(use_generic, pattern, fp<ListConstructorFunc>);
        else
            this->list_constructor_function(pattern, ListConstructorFunc);
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
        using gen_t = detail::list_constructor<
            Class,
            Template,
            ListElementType,
            Policy>;
        this->register_list_ctor_func(
            pattern,
            gen_t::generate(detail::generic_cc),
            detail::generic_cc
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
            using gen_t = detail::list_constructor<Class, Template, ListElementType, Policy>;
            this->register_list_ctor_func(
                pattern,
                gen_t::generate(
                    detail::cdecl_last_cc
                ),
                detail::cdecl_last_cc
            );
        }

        return *this;
    }

    ASBIND20_BG_INTERFACE_DEFINE_OP(basic_value_class, opNeg)

    ASBIND20_BG_INTERFACE_DEFINE_OP(basic_value_class, opPostInc)
    ASBIND20_BG_INTERFACE_DEFINE_OP(basic_value_class, opPostDec)

    ASBIND20_BG_INTERFACE_DEFINE_OP(basic_value_class, opAdd)
    ASBIND20_BG_INTERFACE_DEFINE_OP(basic_value_class, opSub)
    ASBIND20_BG_INTERFACE_DEFINE_OP(basic_value_class, opMul)
    ASBIND20_BG_INTERFACE_DEFINE_OP(basic_value_class, opDiv)
    ASBIND20_BG_INTERFACE_DEFINE_OP(basic_value_class, opMod)

    using my_base::opConv;
    using my_base::opImplConv;

    template <typename OtherClass, bool OtherUseGeneric>
    requires(!std::same_as<Class, OtherClass>)
    basic_value_class& opConv(use_generic_t, const basic_value_class<OtherClass, false, OtherUseGeneric>& other)
    {
        ASBIND20_ASSERT(this->get_engine() == other.get_engine());
        this->opConv<OtherClass>(use_generic, other.get_name());
        return *this;
    }

    template <typename OtherClass, bool OtherUseGeneric>
    requires(!std::same_as<Class, OtherClass>)
    basic_value_class& opConv(const basic_value_class<OtherClass, false, OtherUseGeneric>& other)
    {
        ASBIND20_ASSERT(this->get_engine() == other.get_engine());
        this->opConv<OtherClass>(other.get_name());
        return *this;
    }

    template <typename OtherClass, bool OtherUseGeneric>
    requires(!std::same_as<Class, OtherClass>)
    basic_value_class& opImplConv(use_generic_t, const basic_value_class<OtherClass, false, OtherUseGeneric>& other)
    {
        ASBIND20_ASSERT(this->get_engine() == other.get_engine());
        this->opImplConv<OtherClass>(use_generic, other.get_name());
        return *this;
    }

    template <typename OtherClass, bool OtherUseGeneric>
    requires(!std::same_as<Class, OtherClass>)
    basic_value_class& opImplConv(const basic_value_class<OtherClass, false, OtherUseGeneric>& other)
    {
        ASBIND20_ASSERT(this->get_engine() == other.get_engine());
        this->opImplConv<OtherClass>(other.get_name());
        return *this;
    }
};

template <typename Class, bool ForceGeneric = false, typename Listener = default_listener>
using value_class = basic_value_class<Class, false, ForceGeneric, Listener>;

template <typename Class, bool ForceGeneric = false, typename Listener = default_listener>
using template_value_class = basic_value_class<Class, true, ForceGeneric, Listener>;

template <
    typename Class,
    bool Template = false,
    bool ForceGeneric = false,
    typename Listener = default_listener>
class basic_ref_class :
    public class_binding_generator_interface<
        basic_ref_class<Class, Template, ForceGeneric, Listener>,
        Class,
        Template,
        ForceGeneric,
        Listener>
{
    using my_base = class_binding_generator_interface<
        basic_ref_class<Class, Template, ForceGeneric, Listener>,
        Class,
        Template,
        ForceGeneric,
        Listener>;
    using listener_traits_type = listener_traits<Listener>;

public:
    using flag_type = typename my_base::flag_type;
    using class_type = Class;
    using class_binding_generator_tag = void;

    basic_ref_class(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        std::string name,
        AS_NAMESPACE_QUALIFIER asQWORD flags = 0
    )
        : my_base(engine, std::move(name))
    {
        flags |= AS_NAMESPACE_QUALIFIER asOBJ_REF;
        ASBIND20_ASSERT(!(flags & (AS_NAMESPACE_QUALIFIER asOBJ_VALUE)));

        if constexpr(!Template)
        {
            ASBIND20_ASSERT(!(flags & (AS_NAMESPACE_QUALIFIER asOBJ_TEMPLATE)));
        }
        else
        {
            flags |= AS_NAMESPACE_QUALIFIER asOBJ_TEMPLATE;
        }

        // Size is unnecessary for reference type.
        // Use 0 as size to support registering an incomplete type.
        int r = this->register_object_type(flags, 0);
        listener_traits_type::on_class(
            this->get_listener(), *this, r
        );
    }

    template <string_like StringLike>
    basic_ref_class(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        StringLike&& name,
        AS_NAMESPACE_QUALIFIER asQWORD flags = 0
    )
        : basic_ref_class(
              engine,
              util::string_like_to_string(std::forward<StringLike>(name)),
              flags
          )
    {}

    template <bool AppendOnly>
    basic_ref_class(
        appending_t<AppendOnly>,
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        std::string name
    )
        : my_base(engine, std::move(name))
    {
        flag_type flags = AS_NAMESPACE_QUALIFIER asOBJ_REF;
        if constexpr(Template)
            flags |= AS_NAMESPACE_QUALIFIER asOBJ_TEMPLATE;
        this->append_type(AppendOnly, flags, 0);
    }

    template <bool AppendOnly, string_like StringLike>
    basic_ref_class(
        appending_t<AppendOnly>,
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
        StringLike&& name
    )
        : basic_ref_class(
              appending_t<AppendOnly>{},
              engine,
              util::string_like_to_string(std::forward<StringLike>(name))
          )
    {}

    basic_ref_class(const basic_ref_class&) = default;

    basic_ref_class& operator=(const basic_ref_class&) = delete;

    using my_base::get_engine;
    using my_base::get_name;

private:
    std::string decl_factory(std::string_view params) const
    {
        if constexpr(Template)
        {
            return params.empty() ?
                       string_concat(get_name(), "@f(int&in)") :
                       string_concat(get_name(), "@f(int&in,", params, ')');
        }
        else
        {
            return params.empty() ?
                       string_concat(get_name(), "@f()") :
                       string_concat(get_name(), "@f(", params, ')');
        }
    }

    std::string decl_factory(std::string_view params, use_explicit_t) const
    {
        if constexpr(Template)
        {
            return params.empty() ?
                       string_concat(get_name(), "@f(int&in)explicit") :
                       string_concat(get_name(), "@f(int&in,", params, ")explicit");
        }
        else
        {
            return params.empty() ?
                       string_concat(get_name(), "@f()explicit") :
                       string_concat(get_name(), "@f(", params, ")explicit");
        }
    }

    std::string decl_factory(std::string_view params, bool explicit_) const
    {
        return explicit_ ?
                   decl_factory(params, use_explicit) :
                   decl_factory(params);
    }

    template <typename Fn>
    void register_factory_function(
        bool explicit_,
        std::string_view params,
        Fn&& fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes conv,
        void* aux = nullptr
    )
    {
        std::string decl = decl_factory(params, explicit_);
        this->register_behaviour(
            AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            decl,
            std::forward<Fn>(fn),
            conv,
            aux
        );
    }

    template <typename FuncSig, typename Auxiliary = void>
    static consteval auto deduce_factory_cc()
        -> AS_NAMESPACE_QUALIFIER asECallConvTypes
    {
        if constexpr(std::is_void_v<Auxiliary>)
        {
            return detail::deduce_beh_callconv<
                AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
                Class,
                FuncSig>();
        }
        else
        {
            return detail::deduce_beh_callconv_aux<
                AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
                Class,
                FuncSig,
                Auxiliary>();
        }
    }

public:
    template <typename Factory>
    requires(!std::is_member_function_pointer_v<Factory>)
    basic_ref_class& factory_function(
        std::string_view params,
        Factory fn
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = deduce_factory_cc<Factory>();
        this->register_factory_function(
            false, params, fn, conv
        );

        return *this;
    }

    template <typename Factory>
    requires(!std::is_member_function_pointer_v<Factory>)
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        Factory fn
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = deduce_factory_cc<Factory>();
        this->register_factory_function(
            true, params, fn, conv
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
        constexpr auto conv = deduce_factory_cc<Factory, Auxiliary>();
        this->register_factory_function(
            false,
            params,
            fn,
            conv,
            my_base::get_auxiliary_address(aux)
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
        constexpr auto conv = deduce_factory_cc<Factory, Auxiliary>();
        this->register_factory_function(
            true,
            params,
            fn,
            conv,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        typename Factory,
        typename Auxiliary,
        bool ObjFirst>
    basic_ref_class& factory_function(
        std::string_view params,
        Factory fn,
        auxiliary_wrapper<Auxiliary> aux,
        obj_loc_t<ObjFirst>
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = detail::conv_of_loc(obj_loc<ObjFirst>, false);
        this->register_factory_function(
            false,
            params,
            fn,
            conv,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        typename Factory,
        typename Auxiliary,
        bool ObjFirst>
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        Factory fn,
        auxiliary_wrapper<Auxiliary> aux,
        obj_loc_t<ObjFirst>
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = detail::conv_of_loc(obj_loc<ObjFirst>, false);
        this->register_factory_function(
            true,
            params,
            fn,
            conv,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    basic_ref_class& factory_function(
        std::string_view params,
        generic_function gfn
    )
    {
        this->register_factory_function(
            false,
            params,
            gfn,
            detail::generic_cc
        );

        return *this;
    }

    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        generic_function gfn
    )
    {
        this->register_factory_function(
            true,
            params,
            gfn,
            detail::generic_cc
        );

        return *this;
    }

    template <typename Auxiliary>
    basic_ref_class& factory_function(
        std::string_view params,
        generic_function gfn,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        this->register_factory_function(
            false,
            params,
            gfn,
            detail::generic_cc,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <typename Auxiliary>
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        generic_function gfn,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        this->register_factory_function(
            true,
            params,
            gfn,
            detail::generic_cc,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <auto Factory>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        fp_wrapper<Factory>
    )
    {
        constexpr auto conv = deduce_factory_cc<decltype(Factory)>();
        this->register_factory_function(
            false,
            params,
            detail::to_asGENFUNC_t(
                fp<Factory>, detail::cc<conv>
            ),
            detail::generic_cc
        );

        return *this;
    }

    template <auto Factory>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        fp_wrapper<Factory>
    )
    {
        constexpr auto conv =
            deduce_factory_cc<decltype(Factory)>();
        this->register_factory_function(
            true,
            params,
            detail::to_asGENFUNC_t(
                fp<Factory>, detail::cc<conv>
            ),
            detail::generic_cc
        );

        return *this;
    }

    template <
        auto AuxFactoryFunc,
        typename Auxiliary>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        fp_wrapper<AuxFactoryFunc>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        constexpr auto conv =
            deduce_factory_cc<decltype(AuxFactoryFunc), Auxiliary>();
        this->register_factory_function(
            false,
            params,
            detail::auxiliary_factory_to_asGENFUNC_t<Template>(
                fp<AuxFactoryFunc>, detail::cc<conv>
            ),
            detail::generic_cc,
            my_base::get_auxiliary_address(aux)
        );
        return *this;
    }

    template <
        auto AuxFactoryFunc,
        typename Auxiliary>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        fp_wrapper<AuxFactoryFunc>,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        constexpr auto conv =
            deduce_factory_cc<decltype(AuxFactoryFunc), Auxiliary>();
        this->register_factory_function(
            true,
            params,
            detail::auxiliary_factory_to_asGENFUNC_t<Template>(
                fp<AuxFactoryFunc>, detail::cc<conv>
            ),
            detail::generic_cc,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        auto AuxFactoryFunc,
        typename Auxiliary,
        bool ObjFirst>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        fp_wrapper<AuxFactoryFunc>,
        auxiliary_wrapper<Auxiliary> aux,
        obj_loc_t<ObjFirst>
    )
    {
        constexpr auto conv = detail::conv_of_loc(obj_loc<ObjFirst>, false);
        this->register_factory_function(
            false,
            params,
            detail::auxiliary_factory_to_asGENFUNC_t<Template>(
                fp<AuxFactoryFunc>, detail::cc<conv>
            ),
            detail::generic_cc,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        auto AuxFactoryFunc,
        typename Auxiliary,
        bool ObjFirst>
    basic_ref_class& factory_function(
        use_generic_t,
        std::string_view params,
        use_explicit_t,
        fp_wrapper<AuxFactoryFunc>,
        auxiliary_wrapper<Auxiliary> aux,
        obj_loc_t<ObjFirst>
    )
    {
        constexpr auto conv = detail::conv_of_loc(obj_loc<ObjFirst>, false);
        this->register_factory_function(
            true,
            params,
            detail::auxiliary_factory_to_asGENFUNC_t<Template>(
                fp<AuxFactoryFunc>, detail::cc<conv>
            ),
            detail::generic_cc,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        auto AuxFactoryFunc,
        typename Auxiliary,
        bool ObjFirst>
    basic_ref_class& factory_function(
        std::string_view params,
        fp_wrapper<AuxFactoryFunc>,
        auxiliary_wrapper<Auxiliary> aux,
        obj_loc_t<ObjFirst>
    )
    {
        if constexpr(ForceGeneric)
            this->factory_function(use_generic, params, fp<AuxFactoryFunc>, aux, obj_loc<ObjFirst>);
        else
            this->factory_function(params, AuxFactoryFunc, aux, obj_loc<ObjFirst>);
        return *this;
    }

    template <
        auto AuxFactoryFunc,
        typename Auxiliary,
        bool ObjFirst>
    basic_ref_class& factory_function(
        std::string_view params,
        use_explicit_t,
        fp_wrapper<AuxFactoryFunc>,
        auxiliary_wrapper<Auxiliary> aux,
        obj_loc_t<ObjFirst>
    )
    {
        if constexpr(ForceGeneric)
            this->factory_function(use_generic, params, use_explicit, fp<AuxFactoryFunc>, aux, obj_loc<ObjFirst>);
        else
            this->factory_function(params, use_explicit, AuxFactoryFunc, aux, obj_loc<ObjFirst>);
        return *this;
    }

    template <auto Factory>
    basic_ref_class& factory_function(
        std::string_view params,
        fp_wrapper<Factory>
    )
    {
        if constexpr(ForceGeneric)
            factory_function(use_generic, params, fp<Factory>);
        else
            factory_function(params, Factory);

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
            factory_function(use_generic, params, use_explicit, fp<Factory>);
        else
            factory_function(params, use_explicit, Factory);

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
        if constexpr(ForceGeneric)
            factory_function(use_generic, params, fp<Factory>, aux);
        else
            factory_function(params, Factory, aux);
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
        if constexpr(ForceGeneric)
            factory_function(use_generic, params, use_explicit, fp<Factory>, aux);
        else
            factory_function(params, use_explicit, Factory, aux);

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
            default_factory(use_generic, use_policy<Policy>);
        else
            this->factory<>("", use_policy<Policy>);
        return *this;
    }

private:
    std::string decl_copy_factory() const
    {
        return string_concat("const ", this->get_name(), "&in");
    }

public:
    template <policies::factory_policy Policy = void>
    basic_ref_class& copy_factory(use_generic_t, use_policy_t<Policy> = {})
    {
        this->factory<const Class&>(use_generic, decl_copy_factory(), use_policy<Policy>);

        return *this;
    }

    template <policies::factory_policy Policy = void>
    basic_ref_class& copy_factory(use_policy_t<Policy> = {})
    {
        if constexpr(ForceGeneric)
            copy_factory(use_generic, use_policy<Policy>);
        else
            this->factory<const Class&>(decl_copy_factory(), use_policy<Policy>);
        return *this;
    }

private:
    template <typename... Args, typename Policy>
    void register_factory_generic(
        std::string_view params, use_policy_t<Policy>, bool explicit_
    )
    {
        std::string decl = decl_factory(params, explicit_);
        using gen_t = detail::factory<Class, Template, Policy, Args...>;

        void* aux = nullptr;
        // For non-template class that notifies GC,
        // store the type information as the auxiliary pointer.
        if constexpr(std::same_as<Policy, policies::notify_gc> && !Template)
            aux = get_engine()->GetTypeInfoById(this->get_type_id());

        this->register_behaviour(
            AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            decl,
            gen_t::generate(detail::generic_cc),
            detail::generic_cc,
            aux
        );
    }

    template <typename... Args, typename Policy>
    void register_factory_native(
        std::string_view params, use_policy_t<Policy>, bool explicit_
    ) requires(!ForceGeneric)
    {
        using gen_t = detail::factory<Class, Template, Policy, Args...>;

        std::string decl = decl_factory(params, explicit_);
        if constexpr(std::same_as<Policy, policies::notify_gc> && !Template)
        {
            // For non-template class that notifies GC,
            // store the type information as the auxiliary pointer.
            void* ti = get_engine()->GetTypeInfoById(this->get_type_id());

            this->register_behaviour(
                AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
                decl,
                gen_t::generate(
                    detail::cdecl_last_cc
                ),
                detail::cdecl_last_cc,
                ti // auxiliary
            );
        }
        else
        {
            this->register_behaviour(
                AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
                decl,
                gen_t::generate(
                    detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                ),
                detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
            );
        }
    }

public:
    template <typename... Args, policies::factory_policy Policy = void>
    basic_ref_class& factory(
        use_generic_t, std::string_view params, use_policy_t<Policy> = {}
    )
    {
        this->template register_factory_generic<Args...>(
            params, use_policy<Policy>, false
        );
        return *this;
    }

    template <typename... Args, policies::factory_policy Policy = void>
    basic_ref_class& factory(
        use_generic_t, std::string_view params, use_explicit_t, use_policy_t<Policy> = {}
    )
    {
        this->template register_factory_generic<Args...>(
            params, use_policy<Policy>, true
        );
        return *this;
    }

    template <typename... Args, policies::factory_policy Policy = void>
    basic_ref_class& factory(
        std::string_view params, use_policy_t<Policy> = {}
    )
    {
        if constexpr(ForceGeneric)
            factory<Args...>(use_generic, params, use_policy<Policy>);
        else
        {
            this->template register_factory_native<Args...>(
                params, use_policy<Policy>, false
            );
        }

        return *this;
    }

    template <typename... Args, policies::factory_policy Policy = void>
    basic_ref_class& factory(
        std::string_view params, use_explicit_t, use_policy_t<Policy> = {}
    )
    {
        if constexpr(ForceGeneric)
            factory<Args...>(use_generic, params, use_explicit, use_policy<Policy>);
        else
        {
            this->template register_factory_native<Args...>(
                params, use_policy<Policy>, true
            );
        }

        return *this;
    }

private:
    std::string decl_list_factory(std::string_view pattern) const
    {
        if constexpr(Template)
            return string_concat(get_name(), "@f(int&in,int&in){", pattern, "}");
        else
            return string_concat(get_name(), "@f(int&in){", pattern, "}");
    }

    template <typename Fn>
    void register_list_factory_func(
        std::string_view pattern,
        Fn&& fn,
        AS_NAMESPACE_QUALIFIER asECallConvTypes conv,
        void* aux = nullptr
    )
    {
        this->register_behaviour(
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY,
            decl_list_factory(pattern),
            fn,
            conv,
            aux
        );
    }

public:
    template <native_function ListFactory>
    basic_ref_class& list_factory_function(
        std::string_view pattern,
        ListFactory ctor
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = detail::deduce_beh_callconv<
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY,
            Class,
            ListFactory>();
        this->register_list_factory_func(
            pattern,
            ctor,
            conv
        );

        return *this;
    }

    basic_ref_class& list_factory_function(
        std::string_view pattern,
        generic_function gfn
    )
    {
        this->register_list_factory_func(
            pattern,
            gfn,
            detail::generic_cc
        );

        return *this;
    }

    template <typename Auxiliary>
    basic_ref_class& list_factory_function(
        std::string_view pattern,
        generic_function ctor,
        auxiliary_wrapper<Auxiliary> aux
    )
    {
        this->register_list_factory_func(
            pattern,
            ctor,
            detail::generic_cc,
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
        constexpr auto conv = detail::deduce_beh_callconv_aux<
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY,
            Class,
            ListFactory,
            Auxiliary>();
        this->register_list_factory_func(
            pattern,
            ctor,
            conv,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        native_function ListFactory,
        typename Auxiliary,
        bool ObjFirst>
    basic_ref_class& list_factory_function(
        std::string_view pattern,
        ListFactory ctor,
        auxiliary_wrapper<Auxiliary> aux,
        obj_loc_t<ObjFirst>
    ) requires(!ForceGeneric)
    {
        constexpr auto conv = detail::conv_of_loc(obj_loc<ObjFirst>, false);
        this->register_list_factory_func(
            pattern,
            ctor,
            conv,
            my_base::get_auxiliary_address(aux)
        );

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
        constexpr auto conv = detail::deduce_beh_callconv_aux<
            AS_NAMESPACE_QUALIFIER asBEHAVE_LIST_FACTORY,
            Class,
            decltype(ListFactory),
            Auxiliary>();
        using gen_t = detail::list_factory_func<
            Class,
            Auxiliary,
            ListFactory,
            Template,
            conv>;

        this->register_list_factory_func(
            pattern,
            gen_t::generate(),
            detail::generic_cc,
            my_base::get_auxiliary_address(aux)
        );

        return *this;
    }

    template <
        auto ListFactory,
        typename Auxiliary,
        bool ObjFirst>
    basic_ref_class& list_factory_function(
        use_generic_t,
        std::string_view pattern,
        fp_wrapper<ListFactory>,
        auxiliary_wrapper<Auxiliary> aux,
        obj_loc_t<ObjFirst>
    )
    {
        constexpr auto conv = detail::conv_of_loc(obj_loc<ObjFirst>, false);
        using gen_t = detail::list_factory_func<
            Class,
            Auxiliary,
            ListFactory,
            Template,
            conv>;
        this->register_list_factory_func(
            pattern,
            gen_t::generate(),
            detail::generic_cc,
            my_base::get_auxiliary_address(aux)
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
        if constexpr(ForceGeneric)
        {
            this->list_factory_function(
                use_generic,
                pattern,
                fp<ListFactory>,
                aux
            );
        }
        else
        {
            this->list_factory_function(
                pattern,
                ListFactory,
                aux
            );
        }

        return *this;
    }

    template <
        auto ListFactory,
        typename Auxiliary,
        bool ObjFirst>
    basic_ref_class& list_factory_function(
        std::string_view pattern,
        fp_wrapper<ListFactory>,
        auxiliary_wrapper<Auxiliary> aux,
        obj_loc_t<ObjFirst>
    )
    {
        if constexpr(ForceGeneric)
        {
            this->list_factory_function(
                use_generic,
                pattern,
                fp<ListFactory>,
                aux,
                obj_loc<ObjFirst>
            );
        }
        else
        {
            this->list_factory_function(
                pattern,
                ListFactory,
                aux,
                obj_loc<ObjFirst>
            );
        }

        return *this;
    }

private:
    // For non-templated types,
    // GC notifier needs to access the typeinfo through the auxiliary pointer
    template <typename FactoryPolicy>
    void* aux_for_policy() const
    {
        void* aux = nullptr;
        if constexpr(std::same_as<FactoryPolicy, policies::notify_gc> && !Template)
        {
            aux = get_engine()->GetTypeInfoById(this->get_type_id());
            ASBIND20_ASSERT(aux != nullptr);
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
        using gen_t = detail::list_factory<
            Class,
            Template,
            ListElementType,
            IListPolicy,
            FactoryPolicy>;
        this->register_list_factory_func(
            pattern,
            gen_t::generate(detail::generic_cc),
            detail::generic_cc,
            this->aux_for_policy<FactoryPolicy>()
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
            using gen_t = detail::list_factory<
                Class,
                Template,
                ListElementType,
                IListPolicy,
                FactoryPolicy>;
            if constexpr(std::same_as<FactoryPolicy, policies::notify_gc> && !Template)
            {
                this->register_list_factory_func(
                    pattern,
                    gen_t::generate(
                        detail::cdecl_last_cc
                    ),
                    detail::cdecl_last_cc,
                    this->aux_for_policy<FactoryPolicy>()
                );
            }
            else
            {
                this->register_list_factory_func(
                    pattern,
                    gen_t::generate(
                        detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                    ),
                    detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
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
        using gen_t = detail::list_factory<
            Class,
            Template,
            ListElementType,
            IListPolicy,
            void>;
        this->register_list_factory_func(
            pattern,
            gen_t::generate(detail::generic_cc),
            detail::generic_cc
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
            using gen_t = detail::list_factory<
                Class,
                Template,
                ListElementType,
                IListPolicy,
                void>;
            this->register_list_factory_func(
                pattern,
                gen_t::generate(
                    detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                ),
                detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
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
        using gen_t = detail::list_factory<
            Class,
            Template,
            ListElementType,
            void,
            FactoryPolicy>;
        this->register_list_factory_func(
            pattern,
            gen_t::generate(detail::generic_cc),
            detail::generic_cc,
            this->aux_for_policy<FactoryPolicy>()
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
            using gen_t = detail::list_factory<
                Class,
                Template,
                ListElementType,
                void,
                FactoryPolicy>;

            if constexpr(Template)
            {
                this->register_list_factory_func(
                    pattern,
                    gen_t::generate(
                        detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                    ),
                    detail::cc<AS_NAMESPACE_QUALIFIER asCALL_CDECL>
                );
            }
            else
            {
                this->register_list_factory_func(
                    pattern,
                    gen_t::generate(
                        detail::cdecl_last_cc
                    ),
                    detail::cdecl_last_cc,
                    this->aux_for_policy<FactoryPolicy>()
                );
            }
        }

        return *this;
    }

    // reference type-only GC behaviours

    ASBIND20_BG_INTERFACE_DEFINE_BEH(basic_ref_class, asBEHAVE_GET_WEAKREF_FLAG, get_weakref_flag)
    ASBIND20_BG_INTERFACE_DEFINE_BEH(basic_ref_class, asBEHAVE_ADDREF, addref)
    ASBIND20_BG_INTERFACE_DEFINE_BEH(basic_ref_class, asBEHAVE_RELEASE, release)
    ASBIND20_BG_INTERFACE_DEFINE_BEH(basic_ref_class, asBEHAVE_GETREFCOUNT, get_refcount)
    ASBIND20_BG_INTERFACE_DEFINE_BEH(basic_ref_class, asBEHAVE_SETGCFLAG, set_gc_flag)
    ASBIND20_BG_INTERFACE_DEFINE_BEH(basic_ref_class, asBEHAVE_GETGCFLAG, get_gc_flag)

    basic_ref_class& as_array() requires(Template)
    {
        [[maybe_unused]]
        int r = 0;
        r = get_engine()->RegisterDefaultArrayType(get_name().c_str());
        ASBIND20_ASSERT(r >= 0);

        return *this;
    }
};

// #undef macros defined by binding generator interface here
#undef ASBIND20_BG_INTERFACE_DEFINE_OP
#undef ASBIND20_BG_INTERFACE_DEFINE_BEH

template <typename Class, bool ForceGeneric = false>
using ref_class = basic_ref_class<Class, false, ForceGeneric>;

template <typename Class, bool ForceGeneric = false>
using template_ref_class = basic_ref_class<Class, true, ForceGeneric>;

template <typename T>
concept class_binding_generator =
    binding_generator<T> &&
    requires() {
        typename T::class_binding_generator_tag;
    };
} // namespace asbind20

#endif
