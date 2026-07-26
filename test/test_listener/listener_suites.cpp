#include "listener_suites.hpp"

namespace test_listener
{
record_global::record_global()
    : mock(std::make_shared<mock_type>())
{}
} // namespace test_listener
