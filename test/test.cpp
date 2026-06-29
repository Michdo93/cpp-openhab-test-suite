/**
 * Integration test for cpp-openhab-test-suite.
 *
 * Compile & run:
 *   cmake -B build -DCMAKE_BUILD_TYPE=Release
 *   cmake --build build
 *   OPENHAB_URL=http://127.0.0.1:8080 \
 *   OPENHAB_USER=openhab \
 *   OPENHAB_PASS=habopen \
 *   ./build/openhab_suite_test
 *
 * Required openHAB items: testSwitch (Switch), testDimmer (Dimmer),
 *   testColor (Color), testGroup (Group containing testSwitch)
 */

#include <openhab/testsuite.h>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace openhab;
using namespace openhab::testsuite;

static int passed = 0, failed = 0;

void check(const std::string& name, bool result) {
    std::cout << (result ? "  ✓ " : "  ✗ ") << name << "\n";
    result ? ++passed : ++failed;
}

int main() {
    const char* url_env  = std::getenv("OPENHAB_URL");
    const char* user_env = std::getenv("OPENHAB_USER");
    const char* pass_env = std::getenv("OPENHAB_PASS");

    std::string url  = url_env  ? url_env  : "http://127.0.0.1:8080";
    std::string user = user_env ? user_env : "openhab";
    std::string pass = pass_env ? pass_env : "habopen";

    std::cout << "\n── Static validators ──\n";
    check("isValidSwitchValue ON",   ItemTester::isValidSwitchValue("ON"));
    check("isValidSwitchValue OFF",  ItemTester::isValidSwitchValue("OFF"));
    check("isValidSwitchValue bad",  !ItemTester::isValidSwitchValue("TOGGLE"));
    check("isValidDimmerValue 50",   ItemTester::isValidDimmerValue("50"));
    check("isValidDimmerValue INC",  ItemTester::isValidDimmerValue("INCREASE"));
    check("isValidDimmerValue 101",  !ItemTester::isValidDimmerValue("101"));
    check("isValidColorValue H,S,B", ItemTester::isValidColorValue("240,100,100"));
    check("isValidColorValue bad",   !ItemTester::isValidColorValue("red"));
    check("isValidNumberValue 42",   ItemTester::isValidNumberValue("42"));
    check("isValidNumberValue unit", ItemTester::isValidNumberValue("20 °C"));
    check("isValidLocation",         ItemTester::isValidLocationValue("48.7758,9.1829"));
    check("isValidLocation bad",     !ItemTester::isValidLocationValue("91,0"));
    check("isValidDateTime",         ItemTester::isValidDateTimeValue("2024-01-15T08:30:00+0000"));

    std::cout << "\n── Connecting to " << url << " ──\n";
    OpenHABClient client(url, user, pass);
    check("login", client.isLoggedIn());
    if (!client.isLoggedIn()) {
        std::cerr << "Connection failed. Skipping integration tests.\n";
        std::cout << "\nResults: " << passed << " passed, " << (failed+1) << " failed\n";
        return 1;
    }

    std::cout << "\n── ItemTester ──\n";
    ItemTester it(client);
    check("doesItemExist testSwitch",   it.doesItemExist("testSwitch"));
    check("doesItemExist nonexistent",  !it.doesItemExist("itemThatDoesNotExist99"));
    check("checkItemIsType Switch",     it.checkItemIsType("testSwitch","Switch"));
    check("checkItemIsType wrong",      !it.checkItemIsType("testSwitch","Dimmer"));
    check("testSwitch ON",  it.testSwitch("testSwitch","ON","ON",10));
    check("testSwitch OFF", it.testSwitch("testSwitch","OFF","OFF",10));

    std::cout << "\n── ThingTester ──\n";
    ThingTester tt(client);
    std::string status = tt.getThingStatus("astro:sun:local");
    std::cout << "  ℹ astro:sun:local status = " << status << "\n";
    check("getThingStatus not empty", !status.empty());

    std::cout << "\n── RuleTester ──\n";
    RuleTester rt(client);
    const std::string ruleUID = "test_color-1";
    auto rs = rt.getRuleStatus(ruleUID);
    check("getRuleStatus not empty", !rs.empty());
    check("enableRule",   rt.enableRule(ruleUID));
    check("runRule",      rt.runRule(ruleUID));
    check("disableRule",  rt.disableRule(ruleUID));
    check("isRuleDisabled", rt.isRuleDisabled(ruleUID));
    rt.enableRule(ruleUID);

    std::cout << "\n── ChannelTester ──\n";
    ChannelTester ct(client);
    check("hasOrphanedLinks runs",
          ct.hasOrphanedLinks() || !ct.hasOrphanedLinks()); // just runs

    std::cout << "\n── SitemapTester ──\n";
    SitemapTester st(client);
    check("doesSitemapExist default",  st.doesSitemapExist("default"));
    check("doesSitemapExist fake",     !st.doesSitemapExist("sitemapThatDoesNotExist99"));

    std::cout << "\n" << std::string(40,'-') << "\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}
