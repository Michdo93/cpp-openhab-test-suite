#pragma once
// ThingTester – the C++ counterpart of ThingTester.py of openhab-test-suite.
#include <openhab/openhab.hpp>

#include <string>

namespace openhab {
namespace testsuite {

class ThingTester {
public:
    explicit ThingTester(OpenHABClient& client);

    /** "ONLINE", "OFFLINE", … or "UNKNOWN" (Python: _getThingStatus). */
    std::string getThingStatus(const std::string& thingUID);
    bool isThingStatus(const std::string& thingUID, const std::string& statusToCheck);
    bool isThingOnline(const std::string& thingUID);
    bool isThingOffline(const std::string& thingUID);
    bool isThingPending(const std::string& thingUID);
    bool isThingUnknown(const std::string& thingUID);
    bool isThingUninitialized(const std::string& thingUID);
    bool isThingError(const std::string& thingUID);
    bool enableThing(const std::string& thingUID);
    bool disableThing(const std::string& thingUID);

private:
    bool setEnabled(const std::string& thingUID, bool enabled);
    Things things_;
};

}  // namespace testsuite
}  // namespace openhab
