// Tests of the test suite against openHAB with the exercise configuration of the book
// series (test/openhab-conf). Without OPENHAB_URL/OPENHAB_TOKEN the test is skipped (77).
#include <openhab/testsuite.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

using openhab::json;
namespace ts = openhab::testsuite;

static int failures = 0, checks = 0;
#define CHECK(cond, what)                                                        \
    do {                                                                         \
        checks++;                                                                \
        if (!(cond)) {                                                           \
            failures++;                                                          \
            std::cout << "FAIL line " << __LINE__ << ": " << what << "\n";       \
        }                                                                        \
    } while (0)

static void sleepMs(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

int main() {
    const char* url = std::getenv("OPENHAB_URL");
    const char* token = std::getenv("OPENHAB_TOKEN");
    if (!url || !*url || !token || !*token) {
        std::cout << "OPENHAB_URL/OPENHAB_TOKEN not set – skipped\n";
        return 77;
    }
    openhab::OpenHABClient c(url, std::nullopt, std::nullopt, std::string(token));
    if (!std::getenv("VERBOSE")) ts::setLog([](int, const std::string&) {});

    ts::ItemTester items(c);
    CHECK(items.doesItemExist("Wohnzimmer_Licht"), "item exists");
    CHECK(!items.doesItemExist("Gibts_nicht"), "unknown item is false");
    CHECK(items.checkItemIsType("Werkstatt_Temperatur", "Number"), "base type");
    CHECK(items.checkItemIsType("Werkstatt_Temperatur", "Number:Temperature"), "type with dimension");
    CHECK(!items.checkItemIsType("Wohnzimmer_Licht", "Dimmer"), "wrong type");
    CHECK(!items.checkItemIsType("Wohnzimmer_Licht", "Lampe"), "invalid type");
    CHECK(items.isGroupItem("gLichter"), "group");
    CHECK(items.doesGroupContainMember("gLichter", "Wohnzimmer_Licht"), "member");
    CHECK(!items.doesGroupContainMember("gLichter", "Flur_Haustuer"), "no member");

    CHECK(items.testSwitch("Wohnzimmer_Licht", "ON", "ON", 5), "testSwitch");
    CHECK(!items.testSwitch("Wohnzimmer_Licht", "HALB", "ON", 2), "invalid switch command");
    CHECK(items.testDimmer("Wohnzimmer_Dimmer", "0", {"0", "0.0"}, 5), "dimmer 0");
    CHECK(items.testDimmer("Wohnzimmer_Dimmer", "30", {"30", "30.0"}, 5), "dimmer 30");
    CHECK(items.testRollershutter("Kueche_Rollladen", "DOWN", "100", 5), "shutter");
    CHECK(items.testContact("Flur_Haustuer", "OPEN", "OPEN", 5), "contact update");
    CHECK(items.testNumber("Werkstatt_Temperatur", "23.5 °C", "23.5 °C", 5), "number with unit");
    CHECK(items.testSwitch("Wohnzimmer_Licht", "OFF", {}, 2), "without expected state");

    // a device that confirms the command late (autoupdate is off for Werkstatt_Luefter)
    std::thread device([&] {
        openhab::EventStream s = openhab::ItemEvents(c).ItemCommandEvent("Werkstatt_Luefter");
        auto ev = s.nextEvent(8000);
        if (ev) {
            sleepMs(1500);
            openhab::Items(c).postUpdate("Werkstatt_Luefter", openhab::str(*ev, "payload.value"));
        }
    });
    sleepMs(300);
    CHECK(items.testSwitch("Werkstatt_Luefter", "ON", "ON", 6), "late confirming device");
    device.join();
    openhab::Items(c).postUpdate("Werkstatt_Luefter", "OFF");

    // NULL as previous state must be restored as state, not sent as command
    openhab::Items(c).addOrUpdateItem("TS_Undef", json{{"type", "Switch"}, {"name", "TS_Undef"},
                                                       {"label", "x"}});
    CHECK(items.testSwitch("TS_Undef", "ON", "ON", 5), "switch from NULL");
    sleepMs(300);
    CHECK(items.checkItemHasState("TS_Undef", "NULL"), "NULL restored");
    openhab::Items(c).deleteItem("TS_Undef");

    // things, rules, links, sitemaps
    ts::ThingTester things(c);
    CHECK(things.isThingOnline("astro:sun:zuhause"), "thing online");
    CHECK(!things.isThingOnline("astro:sun:gibtsnicht"), "unknown thing");
    CHECK(things.isThingUnknown("astro:sun:gibtsnicht"), "unknown thing UNKNOWN");
    ts::RuleTester rules(c);
    CHECK(rules.isRuleActive("restbuch-1"), "rule active");
    CHECK(!rules.isRuleActive("gibtsnicht"), "unknown rule");
    CHECK(!rules.isRuleDisabled("gibtsnicht"), "unknown rule not disabled");
    CHECK(openhab::str(rules.getRuleStatus("restbuch-1"), "uid") == "restbuch-1", "rule status");
    CHECK(rules.getRuleStatus("gibtsnicht").empty(), "status of an unknown rule");
    ts::SitemapTester sitemaps(c);
    CHECK(sitemaps.doesSitemapExist("restbuch"), "sitemap");
    CHECK(sitemaps.doesSitemapContainItem("restbuch", "Wohnzimmer_Licht"), "sitemap item");
    CHECK(!sitemaps.doesSitemapContainItem("restbuch", "Gibts_nicht"), "sitemap without item");
    ts::ChannelTester channels(c);
    CHECK(!channels.isItemLinkedToChannel("Wohnzimmer_Licht", "x:y:z#a"), "no link");
    CHECK(channels.getLinksForItem("Gibts_nicht").is_array(), "links of an unknown item");

    // persistence with a time range
    openhab::Persistence(c).storeItemData("Werkstatt_Temperatur", "2026-03-01T10:00:00.000Z",
                                          "18.5", "inmemory");
    ts::PersistenceTester persistence(c);
    CHECK(persistence.isItemPersisted("inmemory", "Werkstatt_Temperatur"), "persisted");
    CHECK(persistence.hasDataInRange("inmemory", "Werkstatt_Temperatur",
                                     "2026-03-01T09:00:00.000Z", "2026-03-01T11:00:00.000Z"),
          "data in range");
    CHECK(!persistence.hasDataInRange("inmemory", "Werkstatt_Temperatur",
                                      "2001-01-01T00:00:00.000Z", "2001-01-02T00:00:00.000Z"),
          "empty range");
    openhab::Persistence(c).deleteItemData("Werkstatt_Temperatur", "2026-03-01T09:00:00.000Z",
                                           "2026-03-01T11:00:00.000Z", "inmemory");

    std::cout << checks << " checks, " << failures << " failed\n";
    return failures ? 1 : 0;
}
