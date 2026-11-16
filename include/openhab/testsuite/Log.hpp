#pragma once
// Messages of the testers (Python: print).
#include <functional>
#include <string>

namespace openhab {
namespace testsuite {

/** Receives the messages of the testers; level 0 = info, 1 = error. */
using LogFunction = std::function<void(int level, const std::string& message)>;

/** Default: info to stdout, errors to stderr. An empty function restores the default. */
void setLog(LogFunction fn);

}  // namespace testsuite
}  // namespace openhab
