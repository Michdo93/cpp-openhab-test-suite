#pragma once
#include <openhab/openhab.h>
#include <string>
#include <iostream>

namespace openhab { namespace testsuite {

class ThingTester {
public:
    explicit ThingTester(OpenHABClient& client)
        : things_(client) {}

    std::string getThingStatus(const std::string& uid) {
        try {
            auto thing = things_.getThing(uid);
            if (thing.contains("statusInfo") && thing["statusInfo"].contains("status"))
                return thing["statusInfo"]["status"].get<std::string>();
        } catch (const std::exception& e) {
            std::cerr << "Error reading status of '" << uid << "': " << e.what() << "\n";
        }
        return "UNKNOWN";
    }

    bool isThingStatus(const std::string& uid, const std::string& status) {
        return getThingStatus(uid) == status;
    }
    bool isThingOnline        (const std::string& uid) { return isThingStatus(uid, "ONLINE"); }
    bool isThingOffline       (const std::string& uid) { return isThingStatus(uid, "OFFLINE"); }
    bool isThingPending       (const std::string& uid) { return isThingStatus(uid, "PENDING"); }
    bool isThingUnknown       (const std::string& uid) { return isThingStatus(uid, "UNKNOWN"); }
    bool isThingUninitialized (const std::string& uid) { return isThingStatus(uid, "UNINITIALIZED"); }
    bool isThingError         (const std::string& uid) { return isThingStatus(uid, "ERROR"); }

    bool enableThing(const std::string& uid) {
        try {
            things_.enableThing(uid);
            std::cout << "Thing '" << uid << "' enabled successfully.\n";
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error enabling '" << uid << "': " << e.what() << "\n";
            return false;
        }
    }

    bool disableThing(const std::string& uid) {
        try {
            things_.disableThing(uid);
            std::cout << "Thing '" << uid << "' disabled successfully.\n";
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error disabling '" << uid << "': " << e.what() << "\n";
            return false;
        }
    }

private:
    Things things_;
};

}} // namespace openhab::testsuite
