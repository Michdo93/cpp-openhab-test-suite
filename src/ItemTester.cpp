// ItemTester – the C++ counterpart of ItemTester.py of openhab-test-suite.
#include "support.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <regex>

namespace openhab {
namespace testsuite {

using detail::error;
using detail::info;

// ── ExpectedState ─────────────────────────────────────────────────────────────

bool ExpectedState::matches(const std::string& state) const {
    return std::find(states_.begin(), states_.end(), state) != states_.end();
}

std::string ExpectedState::text() const {
    if (!list_) return states_.empty() ? "None" : "'" + states_[0] + "'";
    std::string t = "[";
    for (size_t i = 0; i < states_.size(); i++) t += (i ? ", '" : "'") + states_[i] + "'";
    return t + "]";
}

// ── value checks (Python: _isValid…Value) ─────────────────────────────────────

static std::string trimUpper(const std::string& v) {
    size_t a = v.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = v.find_last_not_of(" \t\r\n");
    std::string t = v.substr(a, b - a + 1);
    for (char& ch : t) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    return t;
}

static bool oneOf(const std::string& v, std::initializer_list<const char*> words) {
    std::string t = trimUpper(v);
    for (const char* w : words)
        if (t == w) return true;
    return false;
}

// float(v): number with optional surrounding spaces
static bool toNumber(const std::string& v, double& d) {
    const char* s = v.c_str();
    while (std::isspace(static_cast<unsigned char>(*s))) s++;
    if (!*s) return false;
    char* end = nullptr;
    d = std::strtod(s, &end);
    if (end == s) return false;
    while (std::isspace(static_cast<unsigned char>(*end))) end++;
    return *end == '\0';
}

static bool percentOr(const std::string& v, std::initializer_list<const char*> words) {
    double d;
    if (oneOf(v, words)) return true;
    return toNumber(v, d) && d >= 0 && d <= 100;
}

static std::vector<double> numbers(const std::string& v, bool& ok) {
    std::vector<double> out;
    size_t p = 0;
    ok = true;
    for (;;) {
        size_t q = v.find(',', p);
        double d;
        if (!toNumber(v.substr(p, q == std::string::npos ? std::string::npos : q - p), d)) {
            ok = false;
            return out;
        }
        out.push_back(d);
        if (q == std::string::npos) return out;
        p = q + 1;
    }
}

bool ItemTester::isValidSwitchValue(const std::string& v) { return oneOf(v, {"ON", "OFF"}); }
bool ItemTester::isValidContactValue(const std::string& v) { return oneOf(v, {"OPEN", "CLOSED"}); }
bool ItemTester::isValidDimmerValue(const std::string& v) {
    return percentOr(v, {"ON", "OFF", "INCREASE", "DECREASE"});
}
bool ItemTester::isValidRollershutterValue(const std::string& v) {
    return percentOr(v, {"UP", "DOWN", "STOP", "MOVE"});
}
bool ItemTester::isValidColorValue(const std::string& v) {
    if (oneOf(v, {"ON", "OFF", "INCREASE", "DECREASE"})) return true;
    bool ok;
    auto x = numbers(v, ok);
    return ok && x.size() == 3 && x[0] >= 0 && x[0] <= 360 && x[1] >= 0 && x[1] <= 100 &&
           x[2] >= 0 && x[2] <= 100;
}
bool ItemTester::isValidPlayerValue(const std::string& v) {
    return oneOf(v, {"PLAY", "PAUSE", "NEXT", "PREVIOUS", "REWIND", "FASTFORWARD"});
}
bool ItemTester::isValidNumberValue(const std::string& v) {
    static const std::regex re(R"(^-?\d+(\.\d+)?(\s+\S+)?$)");
    std::string t = v;
    t.erase(0, t.find_first_not_of(" \t\r\n"));
    t.erase(t.find_last_not_of(" \t\r\n") + 1);
    return std::regex_match(t, re);
}
bool ItemTester::isValidDateTimeValue(const std::string& v) {
    static const std::regex re(
        R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d+)?(Z|[+-]\d{2}:?\d{2})?$)");
    std::string t = v;
    t.erase(0, t.find_first_not_of(" \t\r\n"));
    t.erase(t.find_last_not_of(" \t\r\n") + 1);
    return std::regex_match(t, re);
}
bool ItemTester::isValidLocationValue(const std::string& v) {
    bool ok;
    auto x = numbers(v, ok);
    return ok && (x.size() == 2 || x.size() == 3) && x[0] >= -90 && x[0] <= 90 &&
           x[1] >= -180 && x[1] <= 180;
}
bool ItemTester::isValidImageValue(const std::string& v) {
    std::string t = v.substr(std::min(v.size(), v.find_first_not_of(" \t\r\n")));
    if (t.rfind("http://", 0) == 0 || t.rfind("https://", 0) == 0) return true;
    static const std::regex re(R"(^data:image/[a-zA-Z+]+;base64,)");
    return std::regex_search(t, re);
}

// ── public API ────────────────────────────────────────────────────────────────

ItemTester::ItemTester(OpenHABClient& client)
    : client_(&client), items_(client), itemEvents_(client) {}

bool ItemTester::doesItemExist(const std::string& itemName) {
    json r = items_.getItem(itemName);
    if (!isError(r) && str(r, "name") == itemName) return true;
    error("Error: The item '" + itemName + "' does not exist!");
    return false;
}

bool ItemTester::checkItemIsType(const std::string& itemName, const std::string& itemType) {
    static const char* const validTypes[] = {"Color", "Contact", "DateTime", "Dimmer", "Group",
                                             "Image", "Location", "Number", "Player",
                                             "Rollershutter", "String", "Switch"};
    // "Number:Temperature" etc.: the base type is checked, the dimension must match too
    std::string base = itemType.substr(0, itemType.find(':'));
    if (std::none_of(std::begin(validTypes), std::end(validTypes),
                     [&](const char* t) { return base == t; })) {
        error("Error: '" + itemType + "' is not a valid item type.");
        return false;
    }
    json r = items_.getItem(itemName);
    if (isError(r)) {
        error("Error: The item '" + itemName + "' could not be found.");
        return false;
    }
    std::string actual = str(r, "type");
    if (actual.substr(0, actual.find(':')) == itemType || actual == itemType) return true;
    error("Error: The item '" + itemName + "' is not of type '" + itemType +
          "'! Found type: '" + actual + "'");
    return false;
}

bool ItemTester::checkItemHasState(const std::string& itemName, const std::string& state) {
    json r = items_.getItemState(itemName);
    if (isError(r)) {
        error("Error: Could not retrieve the state for item '" + itemName + "'.");
        return false;
    }
    return detail::text(r) == state;
}

bool ItemTester::isGroupItem(const std::string& itemName) {
    return checkItemIsType(itemName, "Group");
}

json ItemTester::getGroupMembers(const std::string& groupName) {
    Items::GetItemArgs a;
    a.recursive = true;
    json r = items_.getItem(groupName, a);
    if (isError(r) || !r.is_object() || !r.contains("members") || !r["members"].is_array())
        return json::array();
    return r["members"];
}

bool ItemTester::doesGroupContainMember(const std::string& groupName,
                                        const std::string& memberName) {
    for (const json& m : getGroupMembers(groupName))
        if (str(m, "name") == memberName) return true;
    return false;
}

bool ItemTester::checkGroupMemberState(const std::string& groupName,
                                       const std::string& memberName,
                                       const std::string& expectedState) {
    for (const json& m : getGroupMembers(groupName))
        if (str(m, "name") == memberName)
            return detail::text(m.contains("state") ? m["state"] : json()) == expectedState;
    return false;
}

// ── the test run (Python: __testItem, __resetItem, __checkFinalState) ─────────

void ItemTester::resetItem(const std::string& itemName, bool isUpdate,
                           const std::optional<std::string>& initial) {
    if (!initial) return;
    // NULL and UNDEF are no valid commands – they are restored as state
    bool asUpdate = isUpdate || *initial == "NULL" || *initial == "UNDEF";
    json r = asUpdate ? items_.postUpdate(itemName, *initial)
                      : items_.sendCommand(itemName, *initial);
    if (isError(r))
        error("Warning: Could not reset item '" + itemName + "' to '" + *initial + "': " +
              errorMessage(r));
}

bool ItemTester::checkFinalState(const std::string& itemName, const ExpectedState& expected) {
    json r = items_.getItemState(itemName);
    if (isError(r)) error("Error: Could not retrieve the state for item '" + itemName + "'.");
    bool ok = !isError(r) && expected.matches(detail::text(r));
    if (!ok)
        error("Error: After fallback, state of '" + itemName + "' is not " +
              (expected.text().front() == '[' ? "in " : "") + expected.text() + ".");
    return ok;
}

bool ItemTester::testItem(const std::string& itemName, const std::string& itemType,
                          const std::optional<std::string>& command,
                          const ExpectedState& expected, int timeout) {
    bool isUpdate = itemType == "Contact" || itemType == "Location";
    if (timeout <= 0) timeout = 10;
    std::optional<std::string> initial;
    if (command) {
        json s = items_.getItemState(itemName);
        if (isError(s)) error("Warning: Could not retrieve initial state for item '" + itemName + "'.");
        else initial = detail::text(s);
    }
    bool result = false;
    // open the stream before sending, so that no event is missed
    EventStream stream = itemEvents_.ItemStateChangedEvent(itemName);
    if (stream.isError()) {
        error("Error: No event stream for item '" + itemName + "': " + stream.error());
    } else {
        bool rejected = false;
        if (command) {
            json sent = isUpdate ? items_.postUpdate(itemName, *command)
                                 : items_.sendCommand(itemName, *command);
            rejected = isError(sent);
            if (rejected)
                error("Error: '" + *command + "' was rejected for '" + itemName + "': " +
                      errorMessage(sent));
        }
        if (!rejected && expected.empty()) {
            result = true;
        } else if (!rejected) {
            // if the state does not change at all, the check after the timeout decides
            long long end = detail::nowMs() + static_cast<long long>(timeout) * 1000;
            bool reached = false;
            for (;;) {
                long long left = end - detail::nowMs();
                if (left <= 0) break;
                auto ev = stream.nextEvent(static_cast<int>(left));
                if (!ev) {
                    if (!stream.isOpen()) break;
                    continue;
                }
                std::string value = str(*ev, "payload.value");
                if (str(*ev, "type") == "ItemStateChangedEvent" &&
                    !at(*ev, "payload.value").is_null() && expected.matches(value)) {
                    info("Success: '" + itemName + "' reached state '" + value + "'.");
                    reached = true;
                    break;
                }
            }
            result = reached || checkFinalState(itemName, expected);
        }
    }
    stream.close();
    resetItem(itemName, isUpdate, initial);
    return result;
}

#define TYPE_CHECK(type)                                                                  \
    if (!checkItemIsType(itemName, type)) {                                               \
        error("Test failed: '" + itemName + "' is not of type '" type "'.");             \
        return false;                                                                     \
    }

bool ItemTester::testColor(const std::string& itemName, const std::string& command,
                           const ExpectedState& e, int timeout) {
    TYPE_CHECK("Color")
    if (!isValidColorValue(command)) {
        error("Test failed: '" + command + "' is not a valid Color command. Use ON, OFF, "
              "INCREASE, DECREASE, or 'H,S,B' (e.g. '240,100,100').");
        return false;
    }
    return testItem(itemName, "Color", command, e, timeout);
}

bool ItemTester::testContact(const std::string& itemName,
                             const std::optional<std::string>& update, const ExpectedState& e,
                             int timeout) {
    TYPE_CHECK("Contact")
    if (update && !isValidContactValue(*update)) {
        error("Test failed: '" + *update + "' is not a valid Contact update. Use OPEN or CLOSED.");
        return false;
    }
    return testItem(itemName, "Contact", update, e, timeout);
}

bool ItemTester::testDateTime(const std::string& itemName, const std::string& command,
                              const ExpectedState& e, int timeout) {
    TYPE_CHECK("DateTime")
    if (!isValidDateTimeValue(command)) {
        error("Test failed: '" + command + "' is not a valid DateTime command. Use ISO-8601 "
              "format, e.g. '2024-01-15T08:30:00+0000'.");
        return false;
    }
    return testItem(itemName, "DateTime", command, e, timeout);
}

bool ItemTester::testDimmer(const std::string& itemName, const std::string& command,
                            const ExpectedState& e, int timeout) {
    TYPE_CHECK("Dimmer")
    if (!isValidDimmerValue(command)) {
        error("Test failed: '" + command + "' is not a valid Dimmer command. Use ON, OFF, "
              "INCREASE, DECREASE, or a percentage value between 0 and 100.");
        return false;
    }
    return testItem(itemName, "Dimmer", command, e, timeout);
}

bool ItemTester::testImage(const std::string& itemName, const std::string& command,
                           const ExpectedState& e, int timeout) {
    TYPE_CHECK("Image")
    if (!isValidImageValue(command)) {
        error("Test failed: '" + command + "' is not a valid Image command. Use an http/https "
              "URL or a Base64 data URI (data:image/...;base64,...).");
        return false;
    }
    return testItem(itemName, "Image", command, e, timeout);
}

bool ItemTester::testLocation(const std::string& itemName, const std::string& update,
                              const ExpectedState& e, int timeout) {
    TYPE_CHECK("Location")
    if (!isValidLocationValue(update)) {
        error("Test failed: '" + update + "' is not a valid Location update. Use 'lat,lon' or "
              "'lat,lon,alt', e.g. '48.7758,9.1829' or '48.7758,9.1829,300.0'.");
        return false;
    }
    return testItem(itemName, "Location", update, e, timeout);
}

bool ItemTester::testNumber(const std::string& itemName, const std::string& command,
                            const ExpectedState& e, int timeout) {
    TYPE_CHECK("Number")
    if (!isValidNumberValue(command)) {
        error("Test failed: '" + command + "' is not a valid Number command. Use a numeric "
              "value, optionally with a unit (e.g. '20', '20.5', '20 °C').");
        return false;
    }
    return testItem(itemName, "Number", command, e, timeout);
}

bool ItemTester::testPlayer(const std::string& itemName, const std::string& command,
                            const ExpectedState& e, int timeout) {
    TYPE_CHECK("Player")
    if (!isValidPlayerValue(command)) {
        error("Test failed: '" + command + "' is not a valid Player command. Use PLAY, PAUSE, "
              "NEXT, PREVIOUS, REWIND, or FASTFORWARD.");
        return false;
    }
    return testItem(itemName, "Player", command, e, timeout);
}

bool ItemTester::testRollershutter(const std::string& itemName, const std::string& command,
                                   const ExpectedState& e, int timeout) {
    TYPE_CHECK("Rollershutter")
    if (!isValidRollershutterValue(command)) {
        error("Test failed: '" + command + "' is not a valid Rollershutter command. Use UP, "
              "DOWN, STOP, MOVE, or a percentage value between 0 and 100.");
        return false;
    }
    return testItem(itemName, "Rollershutter", command, e, timeout);
}

bool ItemTester::testString(const std::string& itemName, const std::string& command,
                            const ExpectedState& e, int timeout) {
    TYPE_CHECK("String")
    return testItem(itemName, "String", command, e, timeout);
}

bool ItemTester::testSwitch(const std::string& itemName, const std::string& command,
                            const ExpectedState& e, int timeout) {
    TYPE_CHECK("Switch")
    if (!isValidSwitchValue(command)) {
        error("Test failed: '" + command + "' is not a valid Switch command. Use ON or OFF.");
        return false;
    }
    return testItem(itemName, "Switch", command, e, timeout);
}

}  // namespace testsuite
}  // namespace openhab
