#ifndef ASBIND20_DEBUGGING_DUMP_BYTECODE_HPP
#define ASBIND20_DEBUGGING_DUMP_BYTECODE_HPP

#include <span>
#include "../detail/config.hpp"
#ifdef ASBIND20_HAS_LIB_FORMAT
#    include <format>
#else
#    include <ostream>
#    include <streambuf>
#    include <iomanip>
#endif
#include <charconv>
#include <iterator>
#include <utility>
#include "../fwd.hpp"

namespace asbind20::debugging
{
using bytecode_span = std::span<const AS_NAMESPACE_QUALIFIER asDWORD>;

[[nodiscard]]
inline bytecode_span get_bytecode(function_reference f)
{
    AS_NAMESPACE_QUALIFIER asUINT len = 0;
    const auto* pbc = f.GetByteCode(&len);
    return {pbc, len};
}

[[nodiscard]]
inline bytecode_span get_bytecode(function_pointer f)
{
    if(!f) [[unlikely]]
        return {};
    return get_bytecode(*f);
}

#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic push
// asBC_* macros have C-style cast in their implementation
#    pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#ifdef ASBIND20_HAS_LIB_FORMAT

template <typename OutputIt>
std::pair<OutputIt, int> dump_single_bytecode(
    OutputIt out, const AS_NAMESPACE_QUALIFIER asDWORD* bc
)
{
#    if defined(__GNUC__) || defined(__clang__)
#        pragma GCC diagnostic push
    // asBC_* macros have C-style cast in their implementation
#        pragma GCC diagnostic ignored "-Wold-style-cast"
#    endif

    if(!bc) [[unlikely]]
        return {std::move(out), 0};

#    ifdef AS_USE_NAMESPACE
    // Workaround for macros
    // See: https://github.com/anjo76/angelscript/issues/36
    using namespace AngelScript;
#    endif

    const auto c = *reinterpret_cast<const asBYTE*>(bc);
    auto& info = asBCInfo[c];

    std::string_view name = info.name;
    switch(info.type)
    {
    default:
    case asBCTYPE_NO_ARG:
        out = std::copy(
            name.cbegin(), name.cend(), out
        );
        break;

    case asBCTYPE_W_ARG:
        out = std::format_to(
            out, "{:<8} {}", name, asBC_WORDARG0(bc)
        );
        break;

    case asBCTYPE_wW_ARG:
    case asBCTYPE_rW_ARG:
        out = std::format_to(
            out, "{:<8} v{}", name, asBC_WORDARG0(bc)
        );
        break;

    case asBCTYPE_wW_rW_ARG:
    case asBCTYPE_rW_rW_ARG:
        out = std::format_to(
            out, "{:<8s} v{}, v{}", name, asBC_WORDARG0(bc), asBC_WORDARG1(bc)
        );
        break;

    case asBCTYPE_wW_W_ARG:
        out = std::format_to(
            out, "{:<8s} v{}, {}", name, asBC_WORDARG0(bc), asBC_WORDARG1(bc)
        );
        break;

    case asBCTYPE_wW_rW_DW_ARG:
    case asBCTYPE_rW_W_DW_ARG:
        switch(c)
        {
        case asBC_ADDIf:
        case asBC_SUBIf:
        case asBC_MULIf:
            out = std::format_to(
                out, "{:<8s} v{}, {}, {:f}", name, asBC_WORDARG0(bc), asBC_WORDARG1(bc), asBC_FLOATARG(bc)
            );
            break;
        default:
            out = std::format_to(
                out, "{:<8s} v{}, {}, {}", name, asBC_WORDARG0(bc), asBC_WORDARG1(bc), asBC_INTARG(bc)
            );
            break;
        }
        break;

    case asBCTYPE_rW_DW_ARG:
    case asBCTYPE_wW_DW_ARG:
    case asBCTYPE_W_DW_ARG:
        if(c == asBC_SetV1)
        {
            out = std::format_to(
                out,
                "{:<8s} v{}, {:#x}",
                name,
                asBC_WORDARG0(bc),
                static_cast<asBYTE>(asBC_DWORDARG(bc))
            );
        }
        else if(c == asBC_SetV2)
        {
            out = std::format_to(
                out,
                "{:<8s} v{}, {:#x}",
                name,
                asBC_WORDARG0(bc),
                static_cast<asWORD>(asBC_DWORDARG(bc))
            );
        }
        else if(c == asBC_SetV4)
        {
            out = std::format_to(
                out,
                "{:<8s} v{}, {:#x} (i:{}, f:{})",
                name,
                asBC_WORDARG0(bc),
                static_cast<asUINT>(asBC_DWORDARG(bc)),
                asBC_INTARG(bc),
                asBC_FLOATARG(bc)
            );
        }
        else if(c == asBC_CMPIf)
        {
            out = std::format_to(
                out, " {:<8s} v{}, {:f}", name, asBC_WORDARG0(bc), asBC_FLOATARG(bc)
            );
        }
        else
        {
            out = std::format_to(
                out,
                "{:<8s} v{}, {}",
                name,
                asBC_WORDARG0(bc),
                static_cast<asUINT>(asBC_DWORDARG(bc))
            );
        }
        break;
    }

#    if defined(__GNUC__) || defined(__clang__)
#        pragma GCC diagnostic pop
#    endif

    return {std::move(out), asBCTypeSize[info.type]};
}

#else

namespace detail
{
    template <typename OutputIt>
    class dump_bc_stream : public std::streambuf
    {
    public:
        dump_bc_stream(OutputIt it)
            : m_out(std::move(it)) {}

        OutputIt out() const
        {
            return m_out;
        }

    protected:
        int_type overflow(int_type c) override
        {
            if(c != traits_type::eof())
            {
                *m_out = traits_type::to_char_type(c);
                ++m_out;
                return c;
            }
            return traits_type::eof();
        }

    private:
        OutputIt m_out;
    };
} // namespace detail

template <typename OutputIt>
std::pair<OutputIt, int> dump_single_bytecode(
    OutputIt out, const AS_NAMESPACE_QUALIFIER asDWORD* bc
)
{
    // ostream version for compatibility with the standard libraries
    // which don't provide <format> yet
    if(!bc) [[unlikely]]
        return {std::move(out), 0};
    detail::dump_bc_stream<OutputIt> dump_buf(std::move(out));
    std::ostream os(&dump_buf);

#    ifdef AS_USE_NAMESPACE
    // Workaround for macros
    // See: https://github.com/anjo76/angelscript/issues/36
    using namespace AngelScript;
#    endif

    const auto c = *reinterpret_cast<const asBYTE*>(bc);
    auto& info = asBCInfo[c];

    const std::string_view name = info.name;

    // Buffer for the floating point formatting
    char fp_buf[64];

    // Print the name of the instruction, with the same padding as
    // the std::format version, i.e. "{:<8s}"
    auto put_name = [&]() -> std::ostream&
    {
        return os << std::left << std::setw(8) << std::setfill(' ') << name;
    };

    // Print the argument as a hexadecimal number, i.e. "{:#x}".
    // std::showbase doesn't print the prefix for zero, so it's printed manually
    auto put_hex = [&](unsigned int value) -> std::ostream&
    {
        return os << "0x" << std::hex << value << std::dec;
    };

    // Print the argument as a floating point number.
    // `fixed_notation` selects "{:f}", otherwise the default "{}" is used.
    // std::to_chars is used here, because the default formatting of
    // std::ostream rounds the value to 6 significant digits,
    // which is not the same as std::format
    auto put_float = [&](float value, bool fixed_notation = true) -> std::ostream&
    {
        if(std::isnan(value))
            return os << (std::signbit(value) ? "-nan" : "nan");
        if(std::isinf(value))
            return os << (std::signbit(value) ? "-inf" : "inf");

        auto res = fixed_notation ?
                       std::to_chars(
                           std::begin(fp_buf),
                           std::end(fp_buf),
                           value,
                           std::chars_format::fixed,
                           6
                       ) :
                       std::to_chars(std::begin(fp_buf), std::end(fp_buf), value);
        ASBIND20_ASSERT(res.ec == std::errc{});
        return os
               << std::string_view(
                      fp_buf, static_cast<std::size_t>(res.ptr - fp_buf)
                  );
    };

    switch(info.type)
    {
    default:
    case asBCTYPE_NO_ARG:
        os << name;
        break;

    case asBCTYPE_W_ARG:
        put_name() << ' ' << asBC_WORDARG0(bc);
        break;

    case asBCTYPE_wW_ARG:
    case asBCTYPE_rW_ARG:
        put_name() << " v" << asBC_WORDARG0(bc);
        break;

    case asBCTYPE_wW_rW_ARG:
    case asBCTYPE_rW_rW_ARG:
        put_name() << " v" << asBC_WORDARG0(bc)
                   << ", v" << asBC_WORDARG1(bc);
        break;

    case asBCTYPE_wW_W_ARG:
        put_name() << " v" << asBC_WORDARG0(bc)
                   << ", " << asBC_WORDARG1(bc);
        break;

    case asBCTYPE_wW_rW_DW_ARG:
    case asBCTYPE_rW_W_DW_ARG:
        put_name() << " v" << asBC_WORDARG0(bc)
                   << ", " << asBC_WORDARG1(bc)
                   << ", ";
        switch(c)
        {
        case asBC_ADDIf:
        case asBC_SUBIf:
        case asBC_MULIf:
            put_float(asBC_FLOATARG(bc));
            break;
        default:
            os << asBC_INTARG(bc);
            break;
        }
        break;

    case asBCTYPE_rW_DW_ARG:
    case asBCTYPE_wW_DW_ARG:
    case asBCTYPE_W_DW_ARG:
        if(c == asBC_CMPIf)
        {
            // NOTE: The leading space is intentional for keeping
            // the output identical to the std::format version
            os << ' ';
            put_name() << " v" << asBC_WORDARG0(bc) << ", ";
            put_float(asBC_FLOATARG(bc));
        }
        else
        {
            put_name() << " v" << asBC_WORDARG0(bc) << ", ";
            if(c == asBC_SetV1)
            {
                put_hex(static_cast<asBYTE>(asBC_DWORDARG(bc)));
            }
            else if(c == asBC_SetV2)
            {
                put_hex(static_cast<asWORD>(asBC_DWORDARG(bc)));
            }
            else if(c == asBC_SetV4)
            {
                put_hex(static_cast<asUINT>(asBC_DWORDARG(bc)));
                os << " (i:" << asBC_INTARG(bc) << ", f:";
                put_float(asBC_FLOATARG(bc), false);
                os << ')';
            }
            else
            {
                os << static_cast<asUINT>(asBC_DWORDARG(bc));
            }
        }
        break;
    }

    return {dump_buf.out(), asBCTypeSize[info.type]};
}

#endif

#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic pop
#endif

template <typename OutputIt>
OutputIt dump_bytecode(OutputIt out, bytecode_span bcs, char sep = '\n')
{
    for(auto it = bcs.begin(); it != bcs.end();)
    {
        int consumed;
        std::tie(out, consumed) = dump_single_bytecode(
            out, std::to_address(it)
        );
        it += consumed;

        // Output the separator
        *out = sep;
        ++out;
    }

    return out;
}

inline std::ostream& print_bytecode(
    std::ostream& os, bytecode_span bcs, char sep = '\n'
)
{
    dump_bytecode(
        std::ostream_iterator<char>(os),
        bcs,
        sep
    );
    return os;
}

inline std::ostream& print_bytecode(
    std::ostream& os, function_reference func, char sep = '\n'
)
{
    return print_bytecode(os, get_bytecode(func), sep);
}

inline std::ostream& print_bytecode(
    std::ostream& os, function_pointer func, char sep = '\n'
)
{
    if(!func) [[unlikely]]
        return os;
    return print_bytecode(os, *func, sep);
}


} // namespace asbind20::debugging

#endif
