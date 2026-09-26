#ifndef ASBIND20_MEMORY_INL
#define ASBIND20_MEMORY_INL

#pragma once

#include "memory.hpp"
#include <new>
#include <limits>

namespace asbind20
{
template <typename T>
constexpr auto script_allocator<T>::allocate(size_type n)
    -> pointer
{
    if(std::is_constant_evaluated())
    {
        std::allocator<T> tmp;
        return tmp.allocate(n);
    }
    else
    {
        if(std::numeric_limits<size_type>::max() / sizeof(T) < n) [[unlikely]]
            detail::throw_<std::bad_array_new_length>();

        void* mem = AS_NAMESPACE_QUALIFIER asAllocMem(n * sizeof(T));
        if(!mem) [[unlikely]]
            asbind20::detail::throw_<std::bad_alloc>();
        return pointer(mem);
    }
}

template <typename T>
constexpr void script_allocator<T>::deallocate(pointer mem, size_type n) noexcept
{
    if(std::is_constant_evaluated())
    {
        std::allocator<T> tmp;
        tmp.deallocate(mem, n);
    }
    else
    {
        (void)n; // unused
        AS_NAMESPACE_QUALIFIER asFreeMem(static_cast<void*>(mem));
    }
}
} // namespace asbind20

#endif
