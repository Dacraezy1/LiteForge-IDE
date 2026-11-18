#include "liteforge/core/FileUtils.hpp"
#include "liteforge/core/Document.hpp"
#include "liteforge/core/Theme.hpp"
#include <iostream>
#include <cassert>

using namespace liteforge::core;

void testFuzzyMatch() {
    std::cout << "[Test] Fuzzy Matching..." << std::endl;

    // Exact match
    auto res1 = FileUtils::fuzzyMatch("main", "main.cpp");
    assert(res1.matched);
    assert(res1.score > 0);

    // Subsequence match
    auto res2 = FileUtils::fuzzyMatch("mcp", "main.cpp");
    assert(res2.matched);

    // Case-insensitive match
    auto res3 = FileUtils::fuzzyMatch("cmk", "CMakeLists.txt");
    assert(res3.matched);

    // Non match
    auto res4 = FileUtils::fuzzyMatch("xyz", "main.cpp");
    assert(!res4.matched);

    std::cout << "  ✓ Fuzzy matching passed!" << std::endl;
}

void testFormatFileSize() {
    std::cout << "[Test] Format File Size..." << std::endl;

    assert(FileUtils::formatFileSize(500) == "500 B");
    assert(FileUtils::formatFileSize(2048) == "2.0 KB");
    assert(FileUtils::formatFileSize(5 * 1024 * 1024) == "5.0 MB");

    std::cout << "  ✓ File size formatting passed!" << std::endl;
}

void testLanguageDetection() {
    std::cout << "[Test] Language Detection..." << std::endl;

    assert(Document::detectLanguage("src/main.cpp") == Language::Cpp);
    assert(Document::detectLanguage("include/test.hpp") == Language::Header);
    assert(Document::detectLanguage("script.py") == Language::Python);
    assert(Document::detectLanguage("src/lib.rs") == Language::Rust);
    assert(Document::detectLanguage("CMakeLists.txt") == Language::CMake);
    assert(Document::detectLanguage("Makefile") == Language::Makefile);
    assert(Document::detectLanguage("README.md") == Language::Markdown);
    assert(Document::detectLanguage("package.json") == Language::Json);

    std::cout << "  ✓ Language detection passed!" << std::endl;
}

void testThemeManager() {
    std::cout << "[Test] Theme Manager..." << std::endl;

    auto& tm = ThemeManager::instance();
    auto themes = tm.availableThemes();
    assert(themes.size() >= 5);

    tm.applyTheme("One Dark Pro");
    assert(tm.currentThemeName() == "One Dark Pro");

    tm.applyTheme("LiteForge Obsidian");
    assert(tm.currentThemeName() == "LiteForge Obsidian");

    QString qss = tm.generateStyleSheet();
    assert(!qss.isEmpty());

    std::cout << "  ✓ Theme manager passed!" << std::endl;
}

int main() {
    std::cout << "Running LiteForge Core Unit Tests..." << std::endl;
    testFuzzyMatch();
    testFormatFileSize();
    testLanguageDetection();
    testThemeManager();
    std::cout << "All LiteForge tests passed successfully! 🚀" << std::endl;
    return 0;
}
