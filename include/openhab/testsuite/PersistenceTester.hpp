#pragma once
// PersistenceTester – the C++ counterpart of PersistenceTester.py of openhab-test-suite.
#include <openhab/openhab.hpp>

#include <string>

namespace openhab {
namespace testsuite {

class PersistenceTester {
public:
    explicit PersistenceTester(OpenHABClient& client);

    bool isItemPersisted(const std::string& serviceID, const std::string& itemName);
    /** startTime, endTime: ISO 8601, e.g. "2026-11-01T00:00:00.000Z". */
    bool hasDataInRange(const std::string& serviceID, const std::string& itemName,
                        const std::string& startTime, const std::string& endTime);
    bool checkLastPersistedState(const std::string& serviceID, const std::string& itemName,
                                 const std::string& expectedState);

private:
    Persistence persistence_;
};

}  // namespace testsuite
}  // namespace openhab
