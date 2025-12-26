#ifndef ASBIND20_DEBUGGING_DUMP_BYTECODE_HPP
#define ASBIND20_DEBUGGING_DUMP_BYTECODE_HPP

#include <span>
#include "../detail/config.hpp"
#ifdef ASBIND20_HAS_LIB_FORMAT
#    include <format>
#endif
#include <iterator>
#include <iostream>
#include "../detail/include_as.hpp"

namespace asbind20::debugging
{
using bytecode_span = std::span<const AS_NAMESPACE_QUALIFIER asDWORD>;

[[nodiscard]]
inline bytecode_span get_bytecode(
    AS_NAMESPACE_QUALIFIER asIScriptFunction* f
)
{
    if(!f) [[unlikely]]
        return {};
    AS_NAMESPACE_QUALIFIER asUINT len;
    const auto* pbc = f->GetByteCode(&len);
    return {pbc, len};
}

#ifdef ASBIND20_HAS_LIB_FORMAT

template <typename OutputIt>
std::pair<OutputIt, int> dump_single_bytecode(
    OutputIt out, const AS_NAMESPACE_QUALIFIER asDWORD* bc
)
{
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
            std::begin(name), std::end(name), out
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

    return {std::move(out), asBCTypeSize[info.type]};
}

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

#endif

} // namespace asbind20::debugging

#endif
