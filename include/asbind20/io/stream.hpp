/**
 * @file io/stream.hpp
 * @author HenryAWE
 * @brief Utilities for `asIBinaryStream`
 */

#ifndef ASBIND20_IO_STREAM_HPP
#define ASBIND20_IO_STREAM_HPP

#pragma once

#include "../fwd.hpp"
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
    /**
     * @brief Wrapper for `std::ostream`
     */
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

        [[nodiscard]]
        std::ostream& get() const noexcept
        {
            return *m_os;
        }

    private:
        std::ostream* m_os;
    };

    /**
     * @brief Wrapper for `std::istream`
     */
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

        [[nodiscard]]
        std::istream& get() const noexcept
        {
            return *m_is;
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

    /**
     * @brief Wrapper for reading from memory
     */
    class memory_reader final : public AS_NAMESPACE_QUALIFIER asIBinaryStream
    {
    public:
        memory_reader(const void* mem, std::size_t sz) noexcept
            : m_ptr(static_cast<const std::byte*>(mem)), m_avail(sz)
        {
            if(m_ptr == nullptr) [[unlikely]]
                m_avail = 0;
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

        [[nodiscard]]
        const std::byte* get() const noexcept
        {
            return m_ptr;
        }

        [[nodiscard]]
        std::size_t available() const noexcept
        {
            return m_avail;
        }

    private:
        const std::byte* m_ptr;
        std::size_t m_avail;
    };

    /**
     * @brief Result of loading byte code
     */
    struct load_byte_code_result
    {
        /// Loading result
        int r;
        bool debug_info_stripped;

        /**
         * @brief Will return true if `r` indicates the byte code was successfully loaded
         */
        explicit operator bool() const noexcept
        {
            return r >= 0;
        }
    };
} // namespace io

/**
 * @defgroup ByteCode Byte code manipulation
 */
/// @{

/**
 * @brief Save byte code to `std::ostream`
 *
 * @param os Output stream
 * @param m Script module to save
 * @param strip_debug_info Strip debug information
 * @return Result of `asIScriptModule::SaveByteCode`
 */
inline int save_byte_code(
    std::ostream& os,
    module_reference m,
    bool strip_debug_info = false
)
{
    io::ostream_wrapper wrapper(os);
    return m.SaveByteCode(&wrapper, strip_debug_info);
}

/**
 * @brief Save byte code to `std::ostream`
 *
 * @param os Output stream
 * @param m Script module to save
 * @param strip_debug_info Strip debug information
 * @return Result of `asIScriptModule::SaveByteCode`
 */
inline int save_byte_code(
    std::ostream& os,
    module_pointer m,
    bool strip_debug_info = false
)
{
    if(!m) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return save_byte_code(os, *m, strip_debug_info);
}

/**
 * @brief Save byte code to an output iterator
 *
 * @tparam OutputIteratorValueType Value type of output iterator.
 *         It can be any integral type whose size equals to `std::byte`.
 *
 * @param out Output iterator
 * @param m Script module
 * @param strip_debug_info Strip debug information
 * @return Result of `asIScriptModule::SaveByteCode`
 */
template <typename OutputIteratorValueType = std::byte>
int save_byte_code(
    std::output_iterator<OutputIteratorValueType> auto out,
    module_reference m,
    bool strip_debug_info = false
)
{
    io::copy_to<decltype(out), OutputIteratorValueType> wrapper(std::move(out));
    return m.SaveByteCode(&wrapper, strip_debug_info);
}

template <typename OutputIteratorValueType = std::byte>
int save_byte_code(
    std::output_iterator<OutputIteratorValueType> auto out,
    module_pointer m,
    bool strip_debug_info = false
)
{
    if(!m) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return save_byte_code<OutputIteratorValueType>(std::move(out), *m, strip_debug_info);
}

/**
 * @brief Load byte code from `std::istream`
 *
 * @param is Input stream
 * @param m Script module
 * @return Loading result
 */
inline io::load_byte_code_result load_byte_code(
    std::istream& is,
    module_reference m
)
{
    io::istream_wrapper wrapper(is);
    bool debug_info_stripped;
    int r = m.LoadByteCode(&wrapper, &debug_info_stripped);
    return {r, debug_info_stripped};
}

/**
 * @brief Load byte code from `std::istream`
 *
 * @param is Input stream
 * @param m Script module
 * @return Loading result
 */
inline io::load_byte_code_result load_byte_code(
    std::istream& is,
    module_pointer m
)
{
    if(!m) [[unlikely]]
        return {AS_NAMESPACE_QUALIFIER asINVALID_ARG, false};
    return load_byte_code(is, *m);
}

/**
 * @brief Load byte code from memory buffer
 *
 * @param mem Memory buffer
 * @param size Buffer size
 * @param m Script module
 * @return Loading result
 */
inline io::load_byte_code_result load_byte_code(
    const void* mem,
    std::size_t size,
    module_reference m
)
{
    io::memory_reader wrapper(mem, size);
    bool debug_info_stripped;
    int r = m.LoadByteCode(&wrapper, &debug_info_stripped);
    return {r, debug_info_stripped};
}

/**
 * @brief Load byte code from memory buffer
 *
 * @param mem Memory buffer
 * @param size Buffer size
 * @param m Script module
 * @return Loading result
 */
inline io::load_byte_code_result load_byte_code(
    const void* mem,
    std::size_t size,
    module_pointer m
)
{
    if(!m) [[unlikely]]
        return {AS_NAMESPACE_QUALIFIER asINVALID_ARG, false};
    return load_byte_code(mem, size, *m);
}

inline io::load_byte_code_result load_byte_code(
    std::span<const std::byte> mem,
    module_reference m
)
{
    return load_byte_code(mem.data(), mem.size_bytes(), m);
}

inline io::load_byte_code_result load_byte_code(
    std::span<const std::byte> mem,
    module_pointer m
)
{
    if(!m) [[unlikely]]
        return {AS_NAMESPACE_QUALIFIER asINVALID_ARG, false};
    return load_byte_code(mem, *m);
}

/// @}
} // namespace asbind20

#endif
