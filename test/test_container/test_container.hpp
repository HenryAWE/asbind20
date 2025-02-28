#pragma once

// IWYU pragma: begin_exports

#include <shared_test_lib.hpp>

// IWYU pragma: end_exports

#if defined(__GNUC__) && !defined(__clang__)
#    if __GNUC__ <= 12
#        define ASBIND20_TEST_SKIP_SEQUENCE_TEST
#    endif
#endif
