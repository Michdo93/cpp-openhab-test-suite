# cpp-openhab-test-suite

Checks an openHAB installation from C++: do the items exist and have the right type, does a
command reach the expected state, are things online, rules active, sitemaps complete and data
persisted? Write such checks once and run them in CI, as a GoogleTest fixture, before a robot
starts its demo or in the setup routine of your own device.

The C++ counterpart of [openhab-test-suite](https://github.com/Michdo93/openhab-test-suite)
(Python) – same testers, methods and messages – built on
[cpp-openhab-rest-client](https://github.com/Michdo93/cpp-openhab-rest-client) 2.x.

```cpp
#include <openhab/testsuite.hpp>
#include <cstdlib>

int main() {
    const char* token = std::getenv("OPENHAB_TOKEN");
    openhab::OpenHABClient c("http://127.0.0.1:8080", std::nullopt, std::nullopt,
                             token ? token : "");
    openhab::testsuite::ItemTester items(c);
    openhab::testsuite::ThingTester things(c);
    bool ok = true;
    ok &= items.checkItemIsType("Werkstatt_Temperatur", "Number:Temperature");
    ok &= items.testSwitch("Wohnzimmer_Licht", "ON", "ON");
    ok &= items.testDimmer("Wohnzimmer_Dimmer", "30", {"30", "30.0"}, 5);
    ok &= things.isThingOnline("astro:sun:home");
    ok &= openhab::testsuite::RuleTester(c).isRuleActive("my-rule-1");
    return ok ? 0 : 1;
}
```

## What is new in 2.0.0

Details: [BUGS-1.0.0.md](BUGS-1.0.0.md).

- built on cpp-openhab-rest-client 2.0.0 (all classes, results as JSON, event streams with timeout)
- the event stream is opened *before* the command is sent; devices that confirm late, the value
  0 and states that do not change are handled; the timeout holds; NULL/UNDEF are restored as state
- several accepted states (`ExpectedState`), types with dimension (`Number:Temperature`)
- unknown items, things and rules return `false` (or `"UNKNOWN"`, an empty status object)
- messages through `openhab::testsuite::setLog()`
- compiled library `openhab::test_suite_cpp` instead of header-only

## Using it

```cmake
cmake_minimum_required(VERSION 3.16)
project(checks CXX)
set(CMAKE_CXX_STANDARD 17)
include(FetchContent)
FetchContent_Declare(cpp_openhab_test_suite
  GIT_REPOSITORY https://github.com/Michdo93/cpp-openhab-test-suite.git
  GIT_TAG 2.0.0)
FetchContent_MakeAvailable(cpp_openhab_test_suite)   # brings cpp-openhab-rest-client along

add_executable(checks main.cpp)
target_link_libraries(checks PRIVATE openhab::test_suite_cpp)
```

Requirements: C++17, CMake 3.16+, libcurl (`sudo apt install build-essential cmake
libcurl4-openssl-dev nlohmann-json3-dev`). The client library is taken from an existing target,
from `-DOPENHAB_REST_CLIENT_DIR=<sources>`, from an installed `OpenHABRestClientCpp` package or
downloaded.

## Testers

All classes live in `openhab::testsuite` and take the client in the constructor.

| Tester | Methods |
|---|---|
| `ItemTester` | `doesItemExist`, `checkItemIsType`, `checkItemHasState`, `isGroupItem`, `getGroupMembers`, `doesGroupContainMember`, `checkGroupMemberState`, `testColor`, `testContact`, `testDateTime`, `testDimmer`, `testImage`, `testLocation`, `testNumber`, `testPlayer`, `testRollershutter`, `testString`, `testSwitch` |
| `ThingTester` | `getThingStatus`, `isThingStatus`, `isThingOnline`, `isThingOffline`, `isThingPending`, `isThingUnknown`, `isThingUninitialized`, `isThingError`, `enableThing`, `disableThing` |
| `RuleTester` | `runRule`, `enableRule`, `disableRule`, `testRuleExecution`, `isRuleActive`, `isRuleDisabled`, `isRuleRunning`, `isRuleIdle`, `getRuleStatus` |
| `ChannelTester` | `isItemLinkedToChannel`, `getLinksForItem`, `hasOrphanedLinks`, `isItemLinkedToAnyChannel` |
| `PersistenceTester` | `isItemPersisted`, `hasDataInRange`, `checkLastPersistedState` |
| `SitemapTester` | `doesSitemapExist`, `doesSitemapContainItem` |

Checks return `bool`. `getGroupMembers`, `getLinksForItem` and `getRuleStatus` return an
`openhab::json`, `getThingStatus` a `std::string`.

The `test…` methods take `(itemName, command, expectedState = {}, timeout = 10)`. They open the
event stream, send the command (Contact and Location: an update), wait at most `timeout` seconds
for the state and restore the previous state afterwards. `expectedState` is one state
(`"ON"`), several accepted states (`{"30", "30.0"}`) or none – then the test only checks that
openHAB accepts the command. `testRuleExecution(ruleUID, item, value, waitSeconds = 2.0)`
waits like Python and then compares the item state.

Messages go to stdout (info) and stderr (errors), or to your own function:

```cpp
openhab::testsuite::setLog([](int level, const std::string& msg) { myLogger(level, msg); });
```

## Development

```bash
cmake -S . -B build && cmake --build build -j2
ctest --test-dir build --output-on-failure            # unit tests
# against openHAB with test/openhab-conf (items, things, rules, sitemap, persistence):
OPENHAB_URL=http://127.0.0.1:8080 OPENHAB_TOKEN=oh.xxx ctest --test-dir build --output-on-failure
```
