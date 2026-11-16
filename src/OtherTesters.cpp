// ThingTester, RuleTester, ChannelTester, PersistenceTester, SitemapTester.
#include "support.hpp"

namespace openhab {
namespace testsuite {

using detail::error;
using detail::info;

// ── ThingTester ───────────────────────────────────────────────────────────────

ThingTester::ThingTester(OpenHABClient& client) : things_(client) {}

std::string ThingTester::getThingStatus(const std::string& thingUID) {
    json r = things_.getThing(thingUID);
    std::string s = isError(r) ? "" : str(r, "statusInfo.status");
    return s.empty() ? "UNKNOWN" : s;
}

bool ThingTester::isThingStatus(const std::string& uid, const std::string& statusToCheck) {
    return getThingStatus(uid) == statusToCheck;
}
bool ThingTester::isThingOnline(const std::string& uid) { return isThingStatus(uid, "ONLINE"); }
bool ThingTester::isThingOffline(const std::string& uid) { return isThingStatus(uid, "OFFLINE"); }
bool ThingTester::isThingPending(const std::string& uid) { return isThingStatus(uid, "PENDING"); }
bool ThingTester::isThingUnknown(const std::string& uid) { return isThingStatus(uid, "UNKNOWN"); }
bool ThingTester::isThingUninitialized(const std::string& uid) {
    return isThingStatus(uid, "UNINITIALIZED");
}
bool ThingTester::isThingError(const std::string& uid) { return isThingStatus(uid, "ERROR"); }

bool ThingTester::setEnabled(const std::string& uid, bool enabled) {
    json r = things_.setThingStatus(uid, enabled);
    const char* what = enabled ? "enabl" : "disabl";
    if (!isError(r)) {
        info("Thing " + uid + " was successfully " + what + "ed.");
        return true;
    }
    error(std::string("Error ") + what + "ing thing " + uid + ": " + errorMessage(r));
    return false;
}
bool ThingTester::enableThing(const std::string& uid) { return setEnabled(uid, true); }
bool ThingTester::disableThing(const std::string& uid) { return setEnabled(uid, false); }

// ── RuleTester ────────────────────────────────────────────────────────────────

RuleTester::RuleTester(OpenHABClient& client) : rules_(client), items_(client) {}

// the rule, or null (with message) if its status cannot be read
json RuleTester::rule(const std::string& ruleUID) {
    json r = rules_.getRule(ruleUID);
    if (isError(r) || !r.is_object() || !r.contains("status") || !r["status"].is_object()) {
        error("Error retrieving the status of rule " + ruleUID + ".");
        return nullptr;
    }
    return r;
}

bool RuleTester::isRuleDisabled(const std::string& ruleUID) {
    json r = rule(ruleUID);
    if (r.is_null()) return false;
    std::string st = str(r, "status.status"), de = str(r, "status.statusDetail");
    info("Rule status: " + st + ", Detail: " + de);
    return st == "UNINITIALIZED" && de == "DISABLED";
}

bool RuleTester::statusIs(const std::string& ruleUID, const char* want, bool negate) {
    json r = rule(ruleUID);
    if (r.is_null()) return false;
    std::string st = str(r, "status.status");
    info("Rule status: " + st);
    return (st == want) != negate;
}

bool RuleTester::isRuleActive(const std::string& uid) { return statusIs(uid, "UNINITIALIZED", true); }
bool RuleTester::isRuleRunning(const std::string& uid) { return statusIs(uid, "RUNNING", false); }
bool RuleTester::isRuleIdle(const std::string& uid) { return statusIs(uid, "IDLE", false); }

bool RuleTester::runRule(const std::string& ruleUID, const json& contextData) {
    if (isRuleDisabled(ruleUID)) {
        error("Error: Rule " + ruleUID + " could not be executed because it is disabled.");
        return false;
    }
    json r = rules_.runNow(ruleUID, contextData);
    if (!isError(r)) {
        info("Rule " + ruleUID + " executed successfully.");
        return true;
    }
    error("Error executing rule " + ruleUID + ": " + errorMessage(r));
    return false;
}

bool RuleTester::enableRule(const std::string& ruleUID) {
    json r = rules_.enable(ruleUID);
    if (!isError(r)) {
        info("Rule " + ruleUID + " enabled successfully.");
        return true;
    }
    error("Error enabling rule " + ruleUID + ": " + errorMessage(r));
    return false;
}

bool RuleTester::disableRule(const std::string& ruleUID) {
    json r = rules_.disable(ruleUID);
    if (!isError(r)) {
        info("Rule " + ruleUID + " disabled successfully.");
        return true;
    }
    error("Error disabling rule " + ruleUID + ": " + errorMessage(r));
    return false;
}

bool RuleTester::testRuleExecution(const std::string& ruleUID, const std::string& expectedItem,
                                   const std::string& expectedValue, double waitSeconds) {
    if (!runRule(ruleUID)) {
        error("Error: Rule " + ruleUID + " could not be executed.");
        return false;
    }
    detail::sleepMs(static_cast<int>((waitSeconds > 0 ? waitSeconds : 2.0) * 1000));
    json s = items_.getItemState(expectedItem);
    std::string state = isError(s) ? errorMessage(s) : detail::text(s);
    if (!isError(s) && state == expectedValue) {
        info(expectedItem + " state after rule execution: " + state);
        return true;
    }
    error("Error: State of item " + expectedItem + " after rule execution does not match. "
          "Expected: " + expectedValue + ", Found: " + state);
    return false;
}

json RuleTester::getRuleStatus(const std::string& ruleUID) {
    json r = rule(ruleUID);
    if (r.is_null()) return json::object();
    std::string st = str(r, "status.status"), de = str(r, "status.statusDetail");
    return json{{"status", st.empty() ? "UNKNOWN" : st},
                {"statusDetail", de.empty() ? "UNKNOWN" : de},
                {"editable", r.value("editable", false)},
                {"name", str(r, "name")},
                {"uid", str(r, "uid")}};
}

// ── ChannelTester ─────────────────────────────────────────────────────────────

ChannelTester::ChannelTester(OpenHABClient& client) : links_(client) {}

bool ChannelTester::isItemLinkedToChannel(const std::string& itemName,
                                          const std::string& channelUID) {
    json r = links_.getLink(itemName, channelUID);
    return !isError(r) && str(r, "itemName") == itemName;
}

json ChannelTester::getLinksForItem(const std::string& itemName) {
    Links::GetLinksArgs a;
    a.itemName = itemName;
    json r = links_.getLinks(a);
    return r.is_array() ? r : json::array();
}

bool ChannelTester::hasOrphanedLinks() {
    json r = links_.getOrphanLinks();
    return r.is_array() && !r.empty();
}

bool ChannelTester::isItemLinkedToAnyChannel(const std::string& itemName) {
    return !getLinksForItem(itemName).empty();
}

// ── PersistenceTester ─────────────────────────────────────────────────────────

PersistenceTester::PersistenceTester(OpenHABClient& client) : persistence_(client) {}

bool PersistenceTester::isItemPersisted(const std::string& serviceID, const std::string& itemName) {
    json r = persistence_.getItemsFromService(serviceID);
    if (!r.is_array()) return false;
    for (const json& e : r)
        if ((e.is_string() && e.get<std::string>() == itemName) || str(e, "name") == itemName)
            return true;
    return false;
}

bool PersistenceTester::hasDataInRange(const std::string& serviceID, const std::string& itemName,
                                       const std::string& startTime, const std::string& endTime) {
    Persistence::GetItemPersistenceDataArgs a;
    a.serviceID = serviceID;
    a.startTime = startTime;
    a.endTime = endTime;
    json r = persistence_.getItemPersistenceData(itemName, a);
    json data = isError(r) ? json() : at(r, "data");
    return data.is_array() && !data.empty();
}

bool PersistenceTester::checkLastPersistedState(const std::string& serviceID,
                                                const std::string& itemName,
                                                const std::string& expectedState) {
    Persistence::GetItemPersistenceDataArgs a;
    a.serviceID = serviceID;
    json r = persistence_.getItemPersistenceData(itemName, a);
    json data = isError(r) ? json() : at(r, "data");
    if (!data.is_array() || data.empty()) return false;
    const json& last = data.back();
    return detail::text(last.contains("state") ? last["state"] : json()) == expectedState;
}

// ── SitemapTester ─────────────────────────────────────────────────────────────

SitemapTester::SitemapTester(OpenHABClient& client) : sitemaps_(client) {}

bool SitemapTester::doesSitemapExist(const std::string& sitemapName) {
    json r = sitemaps_.getSitemaps();
    if (!r.is_array()) return false;
    for (const json& e : r)
        if (str(e, "name") == sitemapName) return true;
    return false;
}

static bool searchForItem(const json& node, const std::string& itemName) {
    if (node.is_object() && str(node, "item.name") == itemName) return true;
    if (node.is_object() || node.is_array())
        for (const json& e : node)
            if (searchForItem(e, itemName)) return true;
    return false;
}

bool SitemapTester::doesSitemapContainItem(const std::string& sitemapName,
                                           const std::string& itemName) {
    json r = sitemaps_.getSitemap(sitemapName);
    return !isError(r) && searchForItem(r, itemName);
}

}  // namespace testsuite
}  // namespace openhab
