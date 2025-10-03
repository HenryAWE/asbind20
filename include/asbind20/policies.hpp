/**
 * @file policies.hpp
 * @author HenryAWE
 * @brief Policies for some special functions/behaviours
 */

#ifndef ASBIND20_POLICIES_HPP
#define ASBIND20_POLICIES_HPP

#pragma once

#include <bit>
#include <span>
#include <cstddef>
#include "meta.hpp"
#include "utility.hpp"

/**
 * @brief Policies for customizing some special functions/behaviours
 */
namespace asbind20::policies
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

            tmp_type(const T* p, std::size_t s) noexcept
                : ptr(p), size(s) {}
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

#ifdef __cpp_lib_containers_ranges
#    define ASBIND20_HAS_CONTAINERS_RANGES __cpp_lib_containers_ranges
#elif defined(ASBIND20_DOXYGEN)
#    define ASBIND20_HAS_CONTAINERS_RANGES
#endif

#ifdef ASBIND20_HAS_CONTAINERS_RANGES

/**
 * @brief Converting the initialization list for constructors accepting C++23 `std::from_range(_t)`
 */
struct as_from_range
{
    using initialization_list_policy_tag = void;
};

#endif

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
} // namespace asbind20::policies

#endif
