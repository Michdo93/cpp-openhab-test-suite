#pragma once
/**
 * cpp-openhab-test-suite 2.0.0 – checks an openHAB installation from C++.
 *
 * The C++ counterpart of openhab-test-suite (Python): the same testers, methods and
 * messages, built on cpp-openhab-rest-client 2.x.
 *
 *   openhab::testsuite::ItemTester items(client);
 *   bool ok = items.testSwitch("Wohnzimmer_Licht", "ON", "ON");
 */
#include "testsuite/Log.hpp"
#include "testsuite/ItemTester.hpp"
#include "testsuite/ThingTester.hpp"
#include "testsuite/RuleTester.hpp"
#include "testsuite/ChannelTester.hpp"
#include "testsuite/PersistenceTester.hpp"
#include "testsuite/SitemapTester.hpp"

namespace openhab {
namespace testsuite {
inline constexpr const char* VERSION = "2.0.0";
}
}  // namespace openhab
