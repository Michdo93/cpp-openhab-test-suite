#pragma once
// SitemapTester – the C++ counterpart of SitemapTester.py of openhab-test-suite.
#include <openhab/openhab.hpp>

#include <string>

namespace openhab {
namespace testsuite {

class SitemapTester {
public:
    explicit SitemapTester(OpenHABClient& client);

    bool doesSitemapExist(const std::string& sitemapName);
    bool doesSitemapContainItem(const std::string& sitemapName, const std::string& itemName);

private:
    Sitemaps sitemaps_;
};

}  // namespace testsuite
}  // namespace openhab
