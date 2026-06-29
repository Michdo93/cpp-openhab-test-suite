#pragma once
#include <openhab/openhab.h>
#include <string>
#include <iostream>

namespace openhab { namespace testsuite {

class PersistenceTester {
public:
    explicit PersistenceTester(OpenHABClient& client) : persistence_(client) {}

    bool isItemPersisted(const std::string& serviceId, const std::string& itemName) {
        try {
            auto items = persistence_.getItemsFromService(serviceId);
            if (!items.is_array()) return false;
            for (const auto& e : items) {
                std::string name = e.is_object() ? e.value("name","") : e.get<std::string>();
                if (name == itemName) return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error checking persistence for '" << itemName
                      << "': " << e.what() << "\n";
        }
        return false;
    }

    bool hasDataInRange(const std::string& serviceId, const std::string& itemName,
                        const std::string& startTime, const std::string& endTime) {
        try {
            auto data = persistence_.getItemPersistenceData(
                itemName, serviceId, startTime, endTime);
            return data.contains("data") && data["data"].is_array() && !data["data"].empty();
        } catch (const std::exception& e) {
            std::cerr << "Error reading persistence data for '" << itemName
                      << "': " << e.what() << "\n";
            return false;
        }
    }

    bool checkLastPersistedState(const std::string& serviceId, const std::string& itemName,
                                 const std::string& expectedState) {
        try {
            auto data    = persistence_.getItemPersistenceData(itemName, serviceId);
            if (!data.contains("data") || !data["data"].is_array()) return false;
            const auto& arr = data["data"];
            if (arr.empty()) return false;
            return arr.back().value("state","") == expectedState;
        } catch (const std::exception& e) {
            std::cerr << "Error reading last persisted state for '" << itemName
                      << "': " << e.what() << "\n";
            return false;
        }
    }

private:
    Persistence persistence_;
};

}} // namespace openhab::testsuite
