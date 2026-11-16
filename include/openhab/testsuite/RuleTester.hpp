#pragma once
// RuleTester – the C++ counterpart of RuleTester.py of openhab-test-suite.
#include <openhab/openhab.hpp>

#include <string>

namespace openhab {
namespace testsuite {

class RuleTester {
public:
    explicit RuleTester(OpenHABClient& client);

    /** Runs a rule (if it is not disabled); contextData may be null. */
    bool runRule(const std::string& ruleUID, const json& contextData = nullptr);
    bool enableRule(const std::string& ruleUID);
    bool disableRule(const std::string& ruleUID);
    /** Runs a rule, waits waitSeconds (2 s as in Python) and checks the item state. */
    bool testRuleExecution(const std::string& ruleUID, const std::string& expectedItem,
                           const std::string& expectedValue, double waitSeconds = 2.0);
    bool isRuleActive(const std::string& ruleUID);
    bool isRuleDisabled(const std::string& ruleUID);
    bool isRuleRunning(const std::string& ruleUID);
    bool isRuleIdle(const std::string& ruleUID);
    /** {status, statusDetail, editable, name, uid} or an empty object. */
    json getRuleStatus(const std::string& ruleUID);

private:
    json rule(const std::string& ruleUID);
    bool statusIs(const std::string& ruleUID, const char* want, bool negate);
    Rules rules_;
    Items items_;
};

}  // namespace testsuite
}  // namespace openhab
