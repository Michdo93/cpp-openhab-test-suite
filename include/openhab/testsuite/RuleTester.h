#pragma once
#include <openhab/openhab.h>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

namespace openhab { namespace testsuite {

class RuleTester {
public:
    explicit RuleTester(OpenHABClient& client)
        : rules_(client), items_(client) {}

    json getRuleStatus(const std::string& uid) {
        try {
            auto rule = rules_.getRule(uid);
            if (rule.contains("status")) {
                auto& s = rule["status"];
                return json{
                    {"status",       s.value("status",       "UNKNOWN")},
                    {"statusDetail", s.value("statusDetail", "UNKNOWN")},
                    {"editable",     rule.value("editable",  false)},
                    {"name",         rule.value("name",      "")},
                    {"uid",          rule.value("uid",       "")},
                };
            }
        } catch (const std::exception& e) {
            std::cerr << "Error reading status of rule '" << uid << "': " << e.what() << "\n";
        }
        return json::object();
    }

    bool isRuleActive  (const std::string& uid) { return getRuleStatus(uid).value("status","UNINITIALIZED") != "UNINITIALIZED"; }
    bool isRuleDisabled(const std::string& uid) {
        auto s = getRuleStatus(uid);
        return s.value("status","") == "UNINITIALIZED" && s.value("statusDetail","") == "DISABLED";
    }
    bool isRuleRunning (const std::string& uid) { return getRuleStatus(uid).value("status","") == "RUNNING"; }
    bool isRuleIdle    (const std::string& uid) { return getRuleStatus(uid).value("status","") == "IDLE"; }

    bool enableRule(const std::string& uid) {
        try {
            rules_.enable(uid);
            std::cout << "Rule '" << uid << "' enabled successfully.\n";
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error enabling rule '" << uid << "': " << e.what() << "\n";
            return false;
        }
    }

    bool disableRule(const std::string& uid) {
        try {
            rules_.disable(uid);
            std::cout << "Rule '" << uid << "' disabled successfully.\n";
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error disabling rule '" << uid << "': " << e.what() << "\n";
            return false;
        }
    }

    bool runRule(const std::string& uid, const json& context = nullptr) {
        if (isRuleDisabled(uid)) {
            std::cerr << "Error: Rule '" << uid << "' is disabled.\n";
            return false;
        }
        try {
            rules_.runNow(uid, context);
            std::cout << "Rule '" << uid << "' executed successfully.\n";
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error executing rule '" << uid << "': " << e.what() << "\n";
            return false;
        }
    }

    bool testRuleExecution(const std::string& uid,
                           const std::string& expectedItem,
                           const std::string& expectedValue) {
        if (!runRule(uid)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        try {
            auto raw   = items_.getItemState(expectedItem);
            std::string state = raw.is_string() ? raw.get<std::string>() : raw.dump();
            if (state == expectedValue) {
                std::cout << "OK: item '" << expectedItem << "' = '" << state << "'.\n";
                return true;
            }
            std::cerr << "Error: '" << expectedItem << "' expected '" << expectedValue
                      << "', found '" << state << "'.\n";
        } catch (const std::exception& e) {
            std::cerr << "Error reading state of '" << expectedItem << "': " << e.what() << "\n";
        }
        return false;
    }

private:
    Rules rules_;
    Items items_;
};

}} // namespace openhab::testsuite
