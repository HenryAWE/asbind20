#ifndef ASBIND20_IO_HPP
#define ASBIND20_IO_HPP

#pragma once

#include "detail/include_as.hpp"
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

inline std::pair<int, bool> load_byte_code(
    std::istream& is,
    AS_NAMESPACE_QUALIFIER asIScriptModule* m
)
{
    io::istream_wrapper wrapper(is);
    bool debug_info_stripped;
    int r = m->LoadByteCode(&wrapper, &debug_info_stripped);
    return {r, debug_info_stripped};
}
} // namespace asbind20

#endif
