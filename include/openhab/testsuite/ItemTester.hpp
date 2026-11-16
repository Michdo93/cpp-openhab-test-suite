#pragma once
// ItemTester – the C++ counterpart of ItemTester.py of openhab-test-suite.
#include <openhab/openhab.hpp>

#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

namespace openhab {
namespace testsuite {

/**
 * The state a test must reach: one state, several accepted states or none (then the test
 * only checks that openHAB accepts the command).
 *
 *   tester.testSwitch("Licht", "ON", "ON");
 *   tester.testDimmer("Dimmer", "30", {"30", "30.0"}, 5);
 *   tester.testSwitch("Licht", "OFF");
 */
class ExpectedState {
public:
    ExpectedState() = default;
    ExpectedState(std::nullopt_t) {}
    ExpectedState(const char* s) { if (s) states_.emplace_back(s); }
    ExpectedState(std::string s) { states_.push_back(std::move(s)); }
    ExpectedState(std::initializer_list<std::string> l) : states_(l), list_(true) {}
    ExpectedState(std::vector<std::string> v) : states_(std::move(v)), list_(true) {}

    bool empty() const { return states_.empty() && !list_; }
    bool matches(const std::string& state) const;
    /** 'ON' or ['30', '30.0'] – as Python prints it. */
    std::string text() const;

private:
    std::vector<std::string> states_;
    bool list_ = false;
};

class ItemTester {
public:
    explicit ItemTester(OpenHABClient& client);

    bool doesItemExist(const std::string& itemName);
    /** itemType: "Number" (base type) or "Number:Temperature" (with dimension). */
    bool checkItemIsType(const std::string& itemName, const std::string& itemType);
    bool checkItemHasState(const std::string& itemName, const std::string& state);
    bool isGroupItem(const std::string& itemName);
    /** Members of a group with their states (array, empty on errors). */
    json getGroupMembers(const std::string& groupName);
    bool doesGroupContainMember(const std::string& groupName, const std::string& memberName);
    bool checkGroupMemberState(const std::string& groupName, const std::string& memberName,
                               const std::string& expectedState);

    // A test opens the event stream first, sends the command (Contact and Location: an
    // update), waits at most timeout seconds for the expected state and afterwards restores
    // the previous state.
    bool testColor(const std::string& itemName, const std::string& command,
                   const ExpectedState& expectedState = {}, int timeout = 10);
    bool testContact(const std::string& itemName,
                     const std::optional<std::string>& update = std::nullopt,
                     const ExpectedState& expectedState = {}, int timeout = 10);
    bool testDateTime(const std::string& itemName, const std::string& command,
                      const ExpectedState& expectedState = {}, int timeout = 10);
    bool testDimmer(const std::string& itemName, const std::string& command,
                    const ExpectedState& expectedState = {}, int timeout = 10);
    bool testImage(const std::string& itemName, const std::string& command,
                   const ExpectedState& expectedState = {}, int timeout = 10);
    bool testLocation(const std::string& itemName, const std::string& update,
                      const ExpectedState& expectedState = {}, int timeout = 10);
    bool testNumber(const std::string& itemName, const std::string& command,
                    const ExpectedState& expectedState = {}, int timeout = 10);
    bool testPlayer(const std::string& itemName, const std::string& command,
                    const ExpectedState& expectedState = {}, int timeout = 10);
    bool testRollershutter(const std::string& itemName, const std::string& command,
                           const ExpectedState& expectedState = {}, int timeout = 10);
    bool testString(const std::string& itemName, const std::string& command,
                    const ExpectedState& expectedState = {}, int timeout = 10);
    bool testSwitch(const std::string& itemName, const std::string& command,
                    const ExpectedState& expectedState = {}, int timeout = 10);

    // Value checks (Python: _isValid…Value)
    static bool isValidSwitchValue(const std::string& v);
    static bool isValidContactValue(const std::string& v);
    static bool isValidDimmerValue(const std::string& v);
    static bool isValidRollershutterValue(const std::string& v);
    static bool isValidColorValue(const std::string& v);
    static bool isValidPlayerValue(const std::string& v);
    static bool isValidNumberValue(const std::string& v);
    static bool isValidDateTimeValue(const std::string& v);
    static bool isValidLocationValue(const std::string& v);
    static bool isValidImageValue(const std::string& v);

private:
    bool testItem(const std::string& itemName, const std::string& itemType,
                  const std::optional<std::string>& command, const ExpectedState& expected,
                  int timeout);
    void resetItem(const std::string& itemName, bool isUpdate,
                   const std::optional<std::string>& initial);
    bool checkFinalState(const std::string& itemName, const ExpectedState& expected);

    OpenHABClient* client_;
    Items items_;
    ItemEvents itemEvents_;
};

}  // namespace testsuite
}  // namespace openhab
