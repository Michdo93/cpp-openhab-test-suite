# cpp-openhab-test-suite

A C++ testing library for validating openHAB installations.
Mirrors the Python [openhab-test-suite](https://github.com/Michdo93/openhab-test-suite)
with identical class names and method names.

**Header-only** — no separate compilation step needed for the testers themselves.

Powered by [cpp-openhab-rest-client](https://github.com/Michdo93/cpp-openhab-rest-client).

## Classes

| Class | Header |
|---|---|
| `ItemTester` | `<openhab/testsuite/ItemTester.h>` |
| `ThingTester` | `<openhab/testsuite/ThingTester.h>` |
| `RuleTester` | `<openhab/testsuite/RuleTester.h>` |
| `ChannelTester` | `<openhab/testsuite/ChannelTester.h>` |
| `PersistenceTester` | `<openhab/testsuite/PersistenceTester.h>` |
| `SitemapTester` | `<openhab/testsuite/SitemapTester.h>` |

Or include everything at once: `#include <openhab/testsuite.h>`

## Adding to your CMake project

### Option 1: add_subdirectory

```bash
git submodule add https://github.com/Michdo93/cpp-openhab-rest-client.git \
    extern/cpp-openhab-test-suite/extern/cpp-openhab-rest-client
git submodule add https://github.com/Michdo93/cpp-openhab-test-suite.git \
    extern/cpp-openhab-test-suite
```

```cmake
add_subdirectory(extern/cpp-openhab-test-suite)

target_link_libraries(myapp PRIVATE
  OpenHABTestSuite::Headers
  ${CURL_LIBRARIES}
  nlohmann_json::nlohmann_json)
```

### Option 2: FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(cpp_openhab_rest_client
  GIT_REPOSITORY https://github.com/Michdo93/cpp-openhab-rest-client.git
  GIT_TAG main)
FetchContent_Declare(cpp_openhab_test_suite
  GIT_REPOSITORY https://github.com/Michdo93/cpp-openhab-test-suite.git
  GIT_TAG main)
FetchContent_MakeAvailable(cpp_openhab_rest_client cpp_openhab_test_suite)

target_link_libraries(myapp PRIVATE
  OpenHABTestSuite::Headers ${CURL_LIBRARIES} nlohmann_json::nlohmann_json)
```

## Build & Test

```bash
# Clone with submodule
git clone --recurse-submodules https://github.com/Michdo93/cpp-openhab-test-suite.git
cd cpp-openhab-test-suite
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Run integration tests
OPENHAB_URL=http://127.0.0.1:8080 OPENHAB_USER=openhab OPENHAB_PASS=habopen \
  ./openhab_suite_test
```

## Usage

```cpp
#include <openhab/testsuite.h>
using namespace openhab;
using namespace openhab::testsuite;

OpenHABClient client("http://127.0.0.1:8080", "openhab", "habopen");

ItemTester it(client);
bool ok = it.testSwitch("MySwitch", "ON", "ON", 10);

ThingTester tt(client);
bool online = tt.isThingOnline("astro:sun:local");

RuleTester rt(client);
rt.enableRule("my-rule");
rt.runRule("my-rule");
```

## License

MIT
