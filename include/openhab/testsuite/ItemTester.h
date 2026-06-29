#pragma once
/**
 * ItemTester — C++ openHAB Test Suite
 *
 * Tests openHAB item behaviour: type checks, command/update validation,
 * SSE-based state observation, and automatic state reset after each test.
 * Mirrors the Python ItemTester from openhab-test-suite.
 */

#include <openhab/openhab.h>
#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <chrono>
#include <atomic>

namespace openhab { namespace testsuite {

class ItemTester {
public:
    explicit ItemTester(OpenHABClient& client)
        : client_(client), items_(client), itemEvents_(client) {}

    // ── Static validators ─────────────────────────────────────────────────────

    static bool isValidSwitchValue(const std::string& v) {
        auto u = toUpper(v);
        return u == "ON" || u == "OFF";
    }
    static bool isValidContactValue(const std::string& v) {
        auto u = toUpper(v);
        return u == "OPEN" || u == "CLOSED";
    }
    static bool isValidDimmerValue(const std::string& v) {
        auto u = toUpper(v);
        if (u=="ON"||u=="OFF"||u=="INCREASE"||u=="DECREASE") return true;
        try { double d = std::stod(v); return d >= 0.0 && d <= 100.0; }
        catch (...) { return false; }
    }
    static bool isValidRollershutterValue(const std::string& v) {
        auto u = toUpper(v);
        if (u=="UP"||u=="DOWN"||u=="STOP"||u=="MOVE") return true;
        try { double d = std::stod(v); return d >= 0.0 && d <= 100.0; }
        catch (...) { return false; }
    }
    static bool isValidColorValue(const std::string& v) {
        auto u = toUpper(v);
        if (u=="ON"||u=="OFF"||u=="INCREASE"||u=="DECREASE") return true;
        // HSB: "H,S,B"
        std::istringstream ss(v); std::string tok;
        std::vector<double> parts;
        while (std::getline(ss, tok, ',')) {
            try { parts.push_back(std::stod(tok)); }
            catch (...) { return false; }
        }
        return parts.size() == 3 &&
               parts[0]>=0&&parts[0]<=360 &&
               parts[1]>=0&&parts[1]<=100 &&
               parts[2]>=0&&parts[2]<=100;
    }
    static bool isValidPlayerValue(const std::string& v) {
        auto u = toUpper(v);
        return u=="PLAY"||u=="PAUSE"||u=="NEXT"||
               u=="PREVIOUS"||u=="REWIND"||u=="FASTFORWARD";
    }
    static bool isValidNumberValue(const std::string& v) {
        return std::regex_match(v, std::regex(R"(^-?\d+(\.\d+)?(\s+\S+)?$)"));
    }
    static bool isValidDateTimeValue(const std::string& v) {
        return std::regex_match(v, std::regex(
            R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d+)?(Z|[+-]\d{2}:?\d{2})?$)"));
    }
    static bool isValidLocationValue(const std::string& v) {
        std::istringstream ss(v); std::string tok;
        std::vector<double> parts;
        while (std::getline(ss, tok, ',')) {
            try { parts.push_back(std::stod(tok)); }
            catch (...) { return false; }
        }
        if (parts.size()<2||parts.size()>3) return false;
        return parts[0]>=-90&&parts[0]<=90 && parts[1]>=-180&&parts[1]<=180;
    }
    static bool isValidImageValue(const std::string& v) {
        return v.rfind("http://",0)==0 || v.rfind("https://",0)==0 ||
               std::regex_search(v, std::regex(R"(^data:image/[a-zA-Z+]+;base64,)"));
    }

    // ── Public API ────────────────────────────────────────────────────────────

    bool doesItemExist(const std::string& itemName) {
        try {
            auto item = items_.getItem(itemName);
            if (item.contains("name") && item["name"].get<std::string>() == itemName)
                return true;
        } catch (...) {}
        std::cerr << "Error: The item '" << itemName << "' does not exist!\n";
        return false;
    }

    bool checkItemIsType(const std::string& itemName, const std::string& itemType) {
        static const std::vector<std::string> valid = {
            "Color","Contact","DateTime","Dimmer","Group","Image",
            "Location","Number","Player","Rollershutter","String","Switch"
        };
        if (std::find(valid.begin(), valid.end(), itemType) == valid.end()) {
            std::cerr << "Error: '" << itemType << "' is not a valid item type.\n";
            return false;
        }
        try {
            auto item   = items_.getItem(itemName);
            std::string actual = item.value("type", "");
            auto colon  = actual.find(':');
            std::string base = (colon != std::string::npos) ? actual.substr(0,colon) : actual;
            if (base == itemType) return true;
            std::cerr << "Error: '" << itemName << "' is type '" << actual
                      << "', expected '" << itemType << "'.\n";
        } catch (const std::exception& e) {
            std::cerr << "Error checking type of '" << itemName << "': " << e.what() << "\n";
        }
        return false;
    }

    bool checkItemHasState(const std::string& itemName, const std::string& expected) {
        try {
            auto raw = items_.getItemState(itemName);
            std::string state = raw.is_string() ? raw.get<std::string>() : raw.dump();
            return state == expected;
        } catch (...) { return false; }
    }

    bool isGroupItem(const std::string& itemName) {
        return checkItemIsType(itemName, "Group");
    }

    json getGroupMembers(const std::string& groupName) {
        try {
            auto item = items_.getItem(groupName, ".*", true);
            if (item.contains("members")) return item["members"];
        } catch (const std::exception& e) {
            std::cerr << "Error reading group '" << groupName << "': " << e.what() << "\n";
        }
        return json::array();
    }

    bool doesGroupContainMember(const std::string& groupName, const std::string& memberName) {
        auto members = getGroupMembers(groupName);
        for (const auto& m : members)
            if (m.value("name","") == memberName) return true;
        return false;
    }

    bool checkGroupMemberState(const std::string& groupName,
                               const std::string& memberName,
                               const std::string& expectedState) {
        auto members = getGroupMembers(groupName);
        for (const auto& m : members)
            if (m.value("name","") == memberName)
                return m.value("state","") == expectedState;
        return false;
    }

    // ── Per-type test methods ─────────────────────────────────────────────────

    bool testSwitch(const std::string& n, const std::string& cmd,
                    const std::string& exp = "", int timeoutSec = 10) {
        if (!checkItemIsType(n,"Switch")) return false;
        if (!isValidSwitchValue(cmd)) { std::cerr<<"Invalid Switch command '"<<cmd<<"'. Use ON or OFF.\n"; return false; }
        return runTest(n,"Switch",cmd,exp,timeoutSec);
    }
    bool testContact(const std::string& n, const std::string& upd = "",
                     const std::string& exp = "", int timeoutSec = 10) {
        if (!checkItemIsType(n,"Contact")) return false;
        if (!upd.empty() && !isValidContactValue(upd)) { std::cerr<<"Invalid Contact update '"<<upd<<"'. Use OPEN or CLOSED.\n"; return false; }
        return runTest(n,"Contact",upd,exp,timeoutSec);
    }
    bool testColor(const std::string& n, const std::string& cmd,
                   const std::string& exp = "", int timeoutSec = 10) {
        if (!checkItemIsType(n,"Color")) return false;
        if (!isValidColorValue(cmd)) { std::cerr<<"Invalid Color command '"<<cmd<<"'.\n"; return false; }
        return runTest(n,"Color",cmd,exp,timeoutSec);
    }
    bool testDimmer(const std::string& n, const std::string& cmd,
                    const std::string& exp = "", int timeoutSec = 10) {
        if (!checkItemIsType(n,"Dimmer")) return false;
        if (!isValidDimmerValue(cmd)) { std::cerr<<"Invalid Dimmer command '"<<cmd<<"'.\n"; return false; }
        return runTest(n,"Dimmer",cmd,exp,timeoutSec);
    }
    bool testRollershutter(const std::string& n, const std::string& cmd,
                           const std::string& exp = "", int timeoutSec = 10) {
        if (!checkItemIsType(n,"Rollershutter")) return false;
        if (!isValidRollershutterValue(cmd)) { std::cerr<<"Invalid Rollershutter command '"<<cmd<<"'.\n"; return false; }
        return runTest(n,"Rollershutter",cmd,exp,timeoutSec);
    }
    bool testNumber(const std::string& n, const std::string& cmd,
                    const std::string& exp = "", int timeoutSec = 10) {
        if (!checkItemIsType(n,"Number")) return false;
        if (!isValidNumberValue(cmd)) { std::cerr<<"Invalid Number command '"<<cmd<<"'.\n"; return false; }
        return runTest(n,"Number",cmd,exp,timeoutSec);
    }
    bool testPlayer(const std::string& n, const std::string& cmd,
                    const std::string& exp = "", int timeoutSec = 10) {
        if (!checkItemIsType(n,"Player")) return false;
        if (!isValidPlayerValue(cmd)) { std::cerr<<"Invalid Player command '"<<cmd<<"'.\n"; return false; }
        return runTest(n,"Player",cmd,exp,timeoutSec);
    }
    bool testDateTime(const std::string& n, const std::string& cmd,
                      const std::string& exp = "", int timeoutSec = 10) {
        if (!checkItemIsType(n,"DateTime")) return false;
        if (!isValidDateTimeValue(cmd)) { std::cerr<<"Invalid DateTime command '"<<cmd<<"'. Use ISO-8601.\n"; return false; }
        return runTest(n,"DateTime",cmd,exp,timeoutSec);
    }
    bool testLocation(const std::string& n, const std::string& upd,
                      const std::string& exp = "", int timeoutSec = 10) {
        if (!checkItemIsType(n,"Location")) return false;
        if (!isValidLocationValue(upd)) { std::cerr<<"Invalid Location update '"<<upd<<"'.\n"; return false; }
        return runTest(n,"Location",upd,exp,timeoutSec);
    }
    bool testImage(const std::string& n, const std::string& cmd,
                   const std::string& exp = "", int timeoutSec = 10) {
        if (!checkItemIsType(n,"Image")) return false;
        if (!isValidImageValue(cmd)) { std::cerr<<"Invalid Image command '"<<cmd<<"'.\n"; return false; }
        return runTest(n,"Image",cmd,exp,timeoutSec);
    }
    bool testString(const std::string& n, const std::string& cmd,
                    const std::string& exp = "", int timeoutSec = 10) {
        if (!checkItemIsType(n,"String")) return false;
        return runTest(n,"String",cmd,exp,timeoutSec);
    }

private:
    OpenHABClient& client_;
    Items          items_;
    ItemEvents     itemEvents_;

    static std::string toUpper(std::string s) {
        for (auto& c : s) c = static_cast<char>(::toupper(c)); return s;
    }

    bool isUpdateOnly(const std::string& t) {
        return t == "Contact" || t == "Location";
    }

    bool runTest(const std::string& itemName, const std::string& itemType,
                 const std::string& commandOrUpdate, const std::string& expectedState,
                 int timeoutSec) {
        std::string initialState;
        bool result = false;

        // Save initial state
        if (!commandOrUpdate.empty()) {
            try {
                auto raw   = items_.getItemState(itemName);
                initialState = raw.is_string() ? raw.get<std::string>() : raw.dump();
            } catch (...) {
                std::cerr << "Warning: could not read initial state of '" << itemName << "'.\n";
            }
        }

        // Send command / update
        if (!commandOrUpdate.empty()) {
            try {
                if (isUpdateOnly(itemType))
                    items_.postUpdate(itemName, commandOrUpdate);
                else
                    items_.sendCommand(itemName, commandOrUpdate);
            } catch (const std::exception& e) {
                std::cerr << "Error sending command to '" << itemName << "': " << e.what() << "\n";
                resetItem(itemName, itemType, initialState);
                return false;
            }
        }

        if (expectedState.empty()) {
            result = true;
        } else {
            // SSE observation with timeout
            auto deadline = std::chrono::steady_clock::now() +
                            std::chrono::seconds(timeoutSec);
            std::atomic<bool> found{false};

            try {
                auto sse = itemEvents_.ItemStateChangedEvent(itemName);
                sse.forEach([&](const std::string& data) -> bool {
                    if (std::chrono::steady_clock::now() >= deadline) return false;
                    try {
                        auto event   = json::parse(data);
                        if (event.value("type","") != "ItemStateChangedEvent") return true;
                        std::string payloadStr = event.value("payload","{}");
                        auto payload = json::parse(payloadStr);
                        std::string val = payload.value("value","");
                        if (val == expectedState) {
                            std::cout << "OK: '" << itemName << "' reached state '"
                                      << val << "'.\n";
                            found = true;
                            return false;  // stop
                        }
                    } catch (...) {}
                    return true;  // continue
                });
            } catch (const std::exception& e) {
                std::cerr << "SSE error for '" << itemName << "': " << e.what() << "\n";
            }

            result = found;
            if (!result) {
                // Fallback: direct read
                result = checkItemHasState(itemName, expectedState);
                if (!result)
                    std::cerr << "Error: state of '" << itemName << "' is not '"
                              << expectedState << "' after " << timeoutSec << "s.\n";
            }
        }

        resetItem(itemName, itemType, initialState);
        return result;
    }

    void resetItem(const std::string& itemName, const std::string& itemType,
                   const std::string& initialState) {
        if (initialState.empty()) return;
        try {
            if (isUpdateOnly(itemType))
                items_.postUpdate(itemName, initialState);
            else
                items_.sendCommand(itemName, initialState);
        } catch (const std::exception& e) {
            std::cerr << "Warning: could not reset '" << itemName
                      << "' to '" << initialState << "': " << e.what() << "\n";
        }
    }
};

}} // namespace openhab::testsuite
