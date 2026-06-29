#pragma once
#include <openhab/openhab.h>
#include <string>
#include <iostream>

namespace openhab { namespace testsuite {

class ChannelTester {
public:
    explicit ChannelTester(OpenHABClient& client) : links_(client) {}

    bool isItemLinkedToChannel(const std::string& item, const std::string& channelUID) {
        try {
            auto link = links_.getLink(item, channelUID);
            return link.value("itemName","") == item;
        } catch (const std::exception& e) {
            std::cerr << "Error checking link '" << item << "' → '"
                      << channelUID << "': " << e.what() << "\n";
            return false;
        }
    }

    json getLinksForItem(const std::string& item) {
        try { return links_.getLinks("", item); }
        catch (const std::exception& e) {
            std::cerr << "Error reading links for '" << item << "': " << e.what() << "\n";
            return json::array();
        }
    }

    bool isItemLinkedToAnyChannel(const std::string& item) {
        auto links = getLinksForItem(item);
        return links.is_array() && !links.empty();
    }

    bool hasOrphanedLinks() {
        try {
            auto orphans = links_.getOrphanLinks();
            return orphans.is_array() && !orphans.empty();
        } catch (const std::exception& e) {
            std::cerr << "Error reading orphan links: " << e.what() << "\n";
            return false;
        }
    }

private:
    Links links_;
};

}} // namespace openhab::testsuite
