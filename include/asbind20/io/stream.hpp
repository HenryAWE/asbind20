/**
 * @file io/stream.hpp
 * @author HenryAWE
 * @brief Utilities for `asIBinaryStream`
 */

#ifndef ASBIND20_IO_STREAM_HPP
#define ASBIND20_IO_STREAM_HPP

#pragma once

#include "../detail/include_as.hpp"
#include <cassert>
#include <cstring>
#include <algorithm>
#include <iostream>

namespace asbind20
{
/**
 * @brief Wrappers for `asIBinaryStream`
 */
namespace io
{
    class ostream_wrapper final : public AS_NAMESPACE_QUALIFIER asIBinaryStream
    {
    public:
        ostream_wrapper() = delete;

        explicit ostream_wrapper(std::ostream& stream) noexcept
            : m_os(&stream) {}

        int Read(void* ptr, AS_NAMESPACE_QUALIFIER asUINT size) override
        {
            (void)ptr;
            (void)size;
            return AS_NAMESPACE_QUALIFIER asERROR;
        }

        int Write(const void* ptr, AS_NAMESPACE_QUALIFIER asUINT size) override
        {
            m_os->write(static_cast<const char*>(ptr), size);
            if(m_os->good())
                return AS_NAMESPACE_QUALIFIER asSUCCESS;
            else
                return AS_NAMESPACE_QUALIFIER asERROR;
        }

    private:
        std::ostream* m_os;
    };

    class istream_wrapper final : public AS_NAMESPACE_QUALIFIER asIBinaryStream
    {
    public:
        istream_wrapper() = delete;

        explicit istream_wrapper(std::istream& stream) noexcept
            : m_is(&stream) {}

        int Read(void* ptr, AS_NAMESPACE_QUALIFIER asUINT size) override
        {
            m_is->read(static_cast<char*>(ptr), size);
            if(m_is->good())
                return AS_NAMESPACE_QUALIFIER asSUCCESS;
            else
                return AS_NAMESPACE_QUALIFIER asERROR;
        }

        int Write(const void* ptr, AS_NAMESPACE_QUALIFIER asUINT size) override
        {
            (void)ptr;
            (void)size;
            return AS_NAMESPACE_QUALIFIER asERROR;
        }

    private:
        std::istream* m_is;
    };

    /**
     * @brief Copy content from binary stream to an output interator
     *
     * @tparam OutputIterator Output iterator
     * @tparam ValueType Single byte value type
     */
    template <typename OutputIterator, typename ValueType = std::byte>
    class copy_to : public AS_NAMESPACE_QUALIFIER asIBinaryStream
    {
    public:
        static_assert(sizeof(ValueType) == 1);
        static_assert(std::output_iterator<OutputIterator, ValueType>);

        explicit copy_to(OutputIterator it) noexcept
            : m_out(std::move(it)) {}

        int Read(void* ptr, AS_NAMESPACE_QUALIFIER asUINT size) override
        {
            (void)ptr;
            (void)size;
            return AS_NAMESPACE_QUALIFIER asERROR;
        }

        int Write(const void* ptr, AS_NAMESPACE_QUALIFIER asUINT size) override
        {
            using input_iterator_type = std::add_const_t<ValueType>*;
            m_out = std::copy_n((input_iterator_type)ptr, size, m_out);

            return AS_NAMESPACE_QUALIFIER asSUCCESS;
        }

        OutputIterator out() const
        {
            return m_out;
        }

    private:
        OutputIterator m_out;
    };

    class memory_reader final : public AS_NAMESPACE_QUALIFIER asIBinaryStream
    {
    public:
        memory_reader(const void* mem, std::size_t sz) noexcept
            : m_ptr(static_cast<const std::byte*>(mem)), m_avail(sz)
        {
            assert(mem != nullptr);
        }

        int Read(void* ptr, AS_NAMESPACE_QUALIFIER asUINT size) override
        {
            if(size > m_avail)
                return AS_NAMESPACE_QUALIFIER asOUT_OF_MEMORY;
            std::memcpy(ptr, m_ptr, size);
            m_avail -= size;
            m_ptr += size;

            return AS_NAMESPACE_QUALIFIER asSUCCESS;
        }

        int Write(const void* ptr, AS_NAMESPACE_QUALIFIER asUINT size) override
        {
            (void)ptr;
            (void)size;
            return AS_NAMESPACE_QUALIFIER asERROR;
        }

    private:
        const std::byte* m_ptr;
        std::size_t m_avail;
    };

    struct load_byte_code_result
    {
        int r;
        bool debug_info_stripped;
    };
} // namespace io

inline int save_byte_code(
    std::ostream& os,
    AS_NAMESPACE_QUALIFIER asIScriptModule* m,
    bool strip_debug_info = false
)
{
    io::ostream_wrapper wrapper(os);
    return m->SaveByteCode(&wrapper, strip_debug_info);
}

template <typename OutputIteratorValueType = std::byte>
int save_byte_code(
    std::output_iterator<OutputIteratorValueType> auto out,
    AS_NAMESPACE_QUALIFIER asIScriptModule* m,
    bool strip_debug_info = false
)
{
    io::copy_to<decltype(out), OutputIteratorValueType> wrapper(std::move(out));
    return m->SaveByteCode(&wrapper, strip_debug_info);
}

inline io::load_byte_code_result load_byte_code(
    std::istream& is,
    AS_NAMESPACE_QUALIFIER asIScriptModule* m
)
{
    io::istream_wrapper wrapper(is);
    bool debug_info_stripped;
    int r = m->LoadByteCode(&wrapper, &debug_info_stripped);
    return {r, debug_info_stripped};
}

inline io::load_byte_code_result load_byte_code(
    const void* mem,
    std::size_t size,
    AS_NAMESPACE_QUALIFIER asIScriptModule* m
)
{
    io::memory_reader wrapper(mem, size);
    bool debug_info_stripped;
    int r = m->LoadByteCode(&wrapper, &debug_info_stripped);
    return {r, debug_info_stripped};
}
} // namespace asbind20

#endif
