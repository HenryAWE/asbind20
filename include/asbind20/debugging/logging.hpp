#ifndef ASBIND20_DEBUGGING_LOGGING_HPP
#define ASBIND20_DEBUGGING_LOGGING_HPP

#include "../fwd.hpp"
#include "../io/fmtlib.hpp"
#include "source_location.hpp"

namespace asbind20::debugging::inline logging
{
using log_level_type = AS_NAMESPACE_QUALIFIER asEMsgType;

#ifdef ASBIND20_HAS_FMTLIB

inline int vlog(
    log_level_type type,
    engine_reference engine,
    const script_source_location& loc,
    std::string_view fmt,
    io::fmtlib::format_args args
)
{
    return engine.WriteMessage(
        loc.section_name().c_str(),
        loc.line(),
        loc.column(),
        type,
        io::fmtlib::vformat(fmt, args).c_str()
    );
}

inline int vlog(
    log_level_type type,
    engine_pointer engine,
    const script_source_location& loc,
    std::string_view fmt,
    io::fmtlib::format_args args
)
{
    if(!engine) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return vlog(type, *engine, loc, fmt, args);
}

template <script_engine_pointer_like Engine>
int vlog(
    log_level_type type,
    const Engine& engine,
    const script_source_location& loc,
    std::string_view fmt,
    io::fmtlib::format_args args
)
{
    return vlog(type, engine.get(), loc, fmt, args);
}

template <typename... Args>
int log(
    log_level_type type,
    engine_reference engine,
    const script_source_location& loc,
    io::format_string<Args...> fmt,
    Args&&... args
)
{
    return vlog(
        type,
        engine,
        loc,
        io::detail::fmt_string_to_view(fmt),
        io::fmtlib::make_format_args(args...)
    );
}

template <typename... Args>
int log(
    log_level_type type,
    engine_pointer engine,
    const script_source_location& loc,
    io::format_string<Args...> fmt,
    Args&&... args
)
{
    if(!engine) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return log(type, *engine, loc, fmt, std::forward<Args>(args)...);
}

template <script_engine_pointer_like Engine, typename... Args>
int log(
    log_level_type type,
    const Engine& engine,
    const script_source_location& loc,
    io::format_string<Args...> fmt,
    Args&&... args
)
{
    return log(type, engine.get(), loc, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
int info(
    engine_reference engine,
    const script_source_location& loc,
    io::format_string<Args...> fmt,
    Args&&... args
)
{
    return vlog(
        AS_NAMESPACE_QUALIFIER asMSGTYPE_INFORMATION,
        engine,
        loc,
        io::detail::fmt_string_to_view(fmt),
        io::fmtlib::make_format_args(args...)
    );
}

template <typename... Args>
int info(
    engine_pointer engine,
    const script_source_location& loc,
    io::format_string<Args...> fmt,
    Args&&... args
)
{
    if(!engine) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return info(*engine, loc, fmt, std::forward<Args>(args)...);
}

template <script_engine_pointer_like Engine, typename... Args>
int info(
    const Engine& engine,
    const script_source_location& loc,
    io::format_string<Args...> fmt,
    Args&&... args
)
{
    return info(engine.get(), loc, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
int warn(
    engine_reference engine,
    const script_source_location& loc,
    io::format_string<Args...> fmt,
    Args&&... args
)
{
    return vlog(
        AS_NAMESPACE_QUALIFIER asMSGTYPE_WARNING,
        engine,
        loc,
        io::detail::fmt_string_to_view(fmt),
        io::fmtlib::make_format_args(args...)
    );
}

template <typename... Args>
int warn(
    engine_pointer engine,
    const script_source_location& loc,
    io::format_string<Args...> fmt,
    Args&&... args
)
{
    if(!engine) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return warn(*engine, loc, fmt, std::forward<Args>(args)...);
}

template <script_engine_pointer_like Engine, typename... Args>
int warn(
    const Engine& engine,
    const script_source_location& loc,
    io::format_string<Args...> fmt,
    Args&&... args
)
{
    return warn(engine.get(), loc, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
int error(
    engine_reference engine,
    const script_source_location& loc,
    io::format_string<Args...> fmt,
    Args&&... args
)
{
    return vlog(
        AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR,
        engine,
        loc,
        io::detail::fmt_string_to_view(fmt),
        io::fmtlib::make_format_args(args...)
    );
}

template <typename... Args>
int error(
    engine_pointer engine,
    const script_source_location& loc,
    io::format_string<Args...> fmt,
    Args&&... args
)
{
    if(!engine) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return error(*engine, loc, fmt, std::forward<Args>(args)...);
}

template <script_engine_pointer_like Engine, typename... Args>
int error(
    const Engine& engine,
    const script_source_location& loc,
    io::format_string<Args...> fmt,
    Args&&... args
)
{
    return error(engine.get(), loc, fmt, std::forward<Args>(args)...);
}

#endif
} // namespace asbind20::debugging::inline logging

#endif
