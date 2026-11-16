// Unit tests of the value checks and ExpectedState – no openHAB needed.
#include <openhab/testsuite.hpp>

#include <iostream>

using openhab::testsuite::ExpectedState;
using openhab::testsuite::ItemTester;

static int failures = 0, checks = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        checks++;                                                                \
        if (!(cond)) {                                                           \
            failures++;                                                          \
            std::cout << "FAIL line " << __LINE__ << ": " #cond "\n";            \
        }                                                                        \
    } while (0)

int main() {
    CHECK(ItemTester::isValidSwitchValue(" on "));
    CHECK(!ItemTester::isValidSwitchValue("TOGGLE"));
    CHECK(ItemTester::isValidContactValue("closed"));
    CHECK(ItemTester::isValidDimmerValue("50") && ItemTester::isValidDimmerValue("0"));
    CHECK(!ItemTester::isValidDimmerValue("101") && !ItemTester::isValidDimmerValue("viel"));
    CHECK(ItemTester::isValidRollershutterValue("MOVE") && ItemTester::isValidRollershutterValue("100"));
    CHECK(ItemTester::isValidColorValue("240,100,100") && !ItemTester::isValidColorValue("red"));
    CHECK(!ItemTester::isValidColorValue("400,1,1"));
    CHECK(ItemTester::isValidPlayerValue("FASTFORWARD"));
    CHECK(ItemTester::isValidNumberValue("42") && ItemTester::isValidNumberValue("-20.5 °C"));
    CHECK(!ItemTester::isValidNumberValue("20,5"));
    CHECK(ItemTester::isValidDateTimeValue("2024-01-15T08:30:00+0000"));
    CHECK(ItemTester::isValidDateTimeValue("2024-01-15T08:30:00.000Z"));
    CHECK(!ItemTester::isValidDateTimeValue("15.01.2024"));
    CHECK(ItemTester::isValidLocationValue("48.7758,9.1829") &&
          ItemTester::isValidLocationValue("48.7758,9.1829,300.0"));
    CHECK(!ItemTester::isValidLocationValue("91,0"));
    CHECK(ItemTester::isValidImageValue("https://example.org/a.png"));
    CHECK(ItemTester::isValidImageValue("data:image/svg+xml;base64,PHN2Zz4="));
    CHECK(!ItemTester::isValidImageValue("bild.png"));

    ExpectedState none;
    ExpectedState one = "ON";
    ExpectedState two = {"30", "30.0"};
    CHECK(none.empty() && !one.empty() && !two.empty());
    CHECK(one.matches("ON") && !one.matches("OFF"));
    CHECK(two.matches("30.0") && !two.matches("31"));
    CHECK(one.text() == "'ON'" && two.text() == "['30', '30.0']");

    std::cout << checks << " checks, " << failures << " failed\n";
    return failures ? 1 : 0;
}
