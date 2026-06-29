#pragma once
#include <openhab/openhab.h>
#include <string>
#include <iostream>

namespace openhab { namespace testsuite {

class SitemapTester {
public:
    explicit SitemapTester(OpenHABClient& client) : sitemaps_(client) {}

    bool doesSitemapExist(const std::string& name) {
        try {
            auto all = sitemaps_.getSitemaps();
            if (!all.is_array()) return false;
            for (const auto& s : all)
                if (s.value("name","") == name) return true;
        } catch (const std::exception& e) {
            std::cerr << "Error reading sitemaps: " << e.what() << "\n";
        }
        return false;
    }

    bool doesSitemapContainItem(const std::string& sitemapName,
                                const std::string& itemName) {
        try {
            auto sitemap = sitemaps_.getSitemap(sitemapName);
            return searchForItem(sitemap, itemName);
        } catch (const std::exception& e) {
            std::cerr << "Error reading sitemap '" << sitemapName
                      << "': " << e.what() << "\n";
            return false;
        }
    }

private:
    Sitemaps sitemaps_;

    bool searchForItem(const json& node, const std::string& itemName) {
        if (node.is_object()) {
            if (node.contains("item") && node["item"].is_object())
                if (node["item"].value("name","") == itemName) return true;
            for (const auto& [key, val] : node.items())
                if (searchForItem(val, itemName)) return true;
        } else if (node.is_array()) {
            for (const auto& el : node)
                if (searchForItem(el, itemName)) return true;
        }
        return false;
    }
};

}} // namespace openhab::testsuite
