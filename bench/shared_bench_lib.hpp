#pragma once

// IWYU pragma: begin_exports

#include <benchmark/benchmark.h>
#include <asbind20/asbind.hpp>

// IWYU pragma: end_exports

namespace asbind_bench
{
/**
 * @brief Returns value without modification, preventing optimization.
 *
 * This aims to simulate user input at runtime.
 *
 * @param val Return value
 */
int int_identity(int val);
} // namespace asbind_bench
