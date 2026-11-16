#include "support.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

namespace openhab {
namespace testsuite {

static std::mutex logMutex;
static LogFunction logFn;

void setLog(LogFunction fn) {
    std::lock_guard<std::mutex> l(logMutex);
    logFn = std::move(fn);
}

namespace detail {

void emit(int level, const std::string& message) {
    std::lock_guard<std::mutex> l(logMutex);
    if (logFn) {
        logFn(level, message);
    } else {
        std::ostream& out = level ? std::cerr : std::cout;
        out << message << std::endl;
    }
}

std::string text(const json& v) {
    if (v.is_null()) return "None";
    if (v.is_string()) return v.get<std::string>();
    if (v.is_boolean()) return v.get<bool>() ? "True" : "False";
    return v.dump();
}

void sleepMs(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

long long nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

}  // namespace detail
}  // namespace testsuite
}  // namespace openhab
