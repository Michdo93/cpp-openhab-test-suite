#pragma once
// ChannelTester – the C++ counterpart of ChannelTester.py of openhab-test-suite.
#include <openhab/openhab.hpp>

#include <string>

namespace openhab {
namespace testsuite {

class ChannelTester {
public:
    explicit ChannelTester(OpenHABClient& client);

    bool isItemLinkedToChannel(const std::string& itemName, const std::string& channelUID);
    /** All links of an item (array, empty on errors). */
    json getLinksForItem(const std::string& itemName);
    bool hasOrphanedLinks();
    bool isItemLinkedToAnyChannel(const std::string& itemName);

private:
    Links links_;
};

}  // namespace testsuite
}  // namespace openhab
