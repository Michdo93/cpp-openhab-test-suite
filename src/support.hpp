// Internal helpers of the test suite.
#pragma once
#include "openhab/testsuite.hpp"

#include <sstream>
#include <string>

namespace openhab {
namespace testsuite {
namespace detail {

void emit(int level, const std::string& message);
inline void info(const std::string& m) { emit(0, m); }
inline void error(const std::string& m) { emit(1, m); }

/** str(value) like Python: strings as they are, None, True/False, numbers. */
std::string text(const json& v);

void sleepMs(int ms);
long long nowMs();

}  // namespace detail
}  // namespace testsuite
}  // namespace openhab
