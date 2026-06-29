#pragma once
/**
 * cpp-openhab-test-suite
 * ──────────────────────
 * Single include for all openHAB test suite classes.
 *
 *   #include <openhab/testsuite.h>
 *
 *   openhab::OpenHABClient client("http://127.0.0.1:8080","openhab","habopen");
 *   openhab::testsuite::ItemTester it(client);
 *   bool ok = it.testSwitch("MySwitch","ON","ON",10);
 */
#include "testsuite/ItemTester.h"
#include "testsuite/ThingTester.h"
#include "testsuite/RuleTester.h"
#include "testsuite/ChannelTester.h"
#include "testsuite/PersistenceTester.h"
#include "testsuite/SitemapTester.h"
