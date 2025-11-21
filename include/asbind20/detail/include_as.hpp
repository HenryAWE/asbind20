/**
 * @file detail/include_as.hpp
 * @author HenryAWE
 *
 * @brief Helper header for including AngelScript
 */

#ifndef ASBIND20_DETAIL_INCLUDE_AS_HPP
#define ASBIND20_DETAIL_INCLUDE_AS_HPP

#pragma once

#ifdef __clang__
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#ifndef ANGELSCRIPT_H

// Avoid having to inform include path if header is already included before
#    include <angelscript.h> // IWYU pragma: export
#endif

#ifdef __clang__
#    pragma clang diagnostic pop
#endif

#endif
