# cpp-openhab-test-suite 1.0.0 – findings and what 2.0.0 changes

Checked on 16 November 2026 against openHAB 5.2.3 and openhab-test-suite 22.0.2 (Python,
the reference).

| Finding | 1.0.0 | 2.0.0 |
|---|---|---|
| Build | header-only on cpp-openhab-rest-client 1.0.0, which implements Persistence, Links and Sitemaps not at all – PersistenceTester, ChannelTester and SitemapTester end in linker errors | compiled library on cpp-openhab-rest-client 2.0.0 (all classes) |
| Order of stream and command | the command is sent *before* the event stream is opened – a fast device changes the state before anybody listens, the test waits for the timeout | stream first, then the command |
| Timeout | checked only when data arrives – without events the test runs until the next ALIVE message or longer | `nextEvent()` with the remaining time; the timeout holds exactly |
| Several accepted states | not possible (Python: list or set) | `ExpectedState`: `"ON"`, `{"30", "30.0"}` or none |
| Rejected commands | errors were exceptions of the client and were swallowed | `{"error": ...}` of the client is reported and the test fails |
| Reset to NULL/UNDEF | sent as command – openHAB rejects it, the item keeps the test value | NULL and UNDEF are restored as state (`postUpdate`) |
| Unknown things/rules | depend on exceptions of the client | `false` / `"UNKNOWN"` / empty status object, like Python |
| Types with dimension | `Number:Temperature` not accepted | base type and full type accepted (Python 22.0.2) |
| Messages | fixed to stdout/stderr | `openhab::testsuite::setLog()`; texts as in Python |

Tests: 23 unit checks (value checks, ExpectedState), 36 checks against openHAB with the
exercise configuration of the book series (late confirming device, value 0, unit, NULL
restore, persistence time range).
