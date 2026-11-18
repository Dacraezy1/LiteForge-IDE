#include "liteforge/core/FileUtils.hpp"
#include "liteforge/core/Document.hpp"
#include "liteforge/core/Theme.hpp"
#include "liteforge/core/ProjectTemplates.hpp"
#include "liteforge/core/ProfileManager.hpp"
#include "liteforge/git/GitService.hpp"
#include "liteforge/lsp/LSPProtocol.hpp"
#include <iostream>
#include <cassert>
#include <QDir>
#include <QTemporaryDir>

using namespace liteforge::core;
using namespace liteforge::git;
using namespace liteforge::lsp;

void testFuzzyMatch() {
    std::cout << "[Test] Fuzzy Matching..." << std::endl;

    auto res1 = FileUtils::fuzzyMatch("main", "main.cpp");
    assert(res1.matched);
    assert(res1.score > 0);

    auto res2 = FileUtils::fuzzyMatch("mcp", "main.cpp");
    assert(res2.matched);

    auto res3 = FileUtils::fuzzyMatch("cmk", "CMakeLists.txt");
    assert(res3.matched);

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

void testGitIgnoreParsing() {
    std::cout << "[Test] GitIgnore Parsing & Matching..." << std::endl;

    QStringList rules = { "build/", "*.o", "node_modules/", "secret.key" };

    assert(FileUtils::isIgnoredByRule("build/app", rules));
    assert(FileUtils::isIgnoredByRule("src/main.o", rules));
    assert(FileUtils::isIgnoredByRule("node_modules/pkg/index.js", rules));
    assert(FileUtils::isIgnoredByRule("secret.key", rules));
    assert(!FileUtils::isIgnoredByRule("src/main.cpp", rules));
    assert(!FileUtils::isIgnoredByRule("CMakeLists.txt", rules));

    std::cout << "  ✓ GitIgnore rules passed!" << std::endl;
}

void testProjectTemplates() {
    std::cout << "[Test] Project Templates..." << std::endl;

    auto templates = ProjectTemplates::availableTemplates();
    assert(templates.size() >= 4);

    QTemporaryDir tempDir;
    assert(tempDir.isValid());

    QString err;
    bool ok = ProjectTemplates::createProject(TemplateType::CppCMake, tempDir.path() + "/test_cpp", "TestCpp", &err);
    assert(ok);
    (void)ok;
    assert(QFile::exists(tempDir.path() + "/test_cpp/CMakeLists.txt"));
    assert(QFile::exists(tempDir.path() + "/test_cpp/src/main.cpp"));
    assert(QFile::exists(tempDir.path() + "/test_cpp/.gitignore"));

    ok = ProjectTemplates::createProject(TemplateType::RustCargo, tempDir.path() + "/test_rust", "TestRust", &err);
    assert(ok);
    (void)ok;
    assert(QFile::exists(tempDir.path() + "/test_rust/Cargo.toml"));
    assert(QFile::exists(tempDir.path() + "/test_rust/src/main.rs"));

    std::cout << "  ✓ Project templates passed!" << std::endl;
}

void testProfileManager() {
    std::cout << "[Test] Workspace Profile Manager..." << std::endl;

    auto& pm = ProfileManager::instance();
    auto profiles = pm.availableProfiles();
    assert(profiles.contains("C/C++ Development"));
    assert(profiles.contains("Rust Development"));
    assert(profiles.contains("Python Development"));
    assert(profiles.contains("Web Development"));
    assert(profiles.contains("Minimal Writing"));

    pm.setCurrentProfile("Rust Development");
    assert(pm.currentProfile().name == "Rust Development");
    assert(pm.currentProfile().buildCommand == "cargo build");

    // Test Profile JSON Export/Import
    QTemporaryDir tempDir;
    QString exportPath = tempDir.path() + "/profile.json";
    assert(pm.exportProfile("Rust Development", exportPath));
    assert(QFile::exists(exportPath));
    assert(pm.importProfile(exportPath));

    std::cout << "  ✓ Profile manager passed!" << std::endl;
}

void testLSPProtocol() {
    std::cout << "[Test] LSP Protocol Encoding & URI conversion..." << std::endl;

    QString localPath = "/home/user/project/main.cpp";
    QString uri = LSPProtocol::pathToUri(localPath);
    assert(uri.startsWith("file://"));
    assert(LSPProtocol::uriToPath(uri) == localPath);

    QJsonObject testMsg;
    testMsg["jsonrpc"] = "2.0";
    testMsg["method"] = "test";
    QByteArray encoded = LSPProtocol::encodeMessage(testMsg);
    assert(encoded.startsWith("Content-Length: "));
    assert(encoded.contains("\r\n\r\n"));

    std::cout << "  ✓ LSP protocol passed!" << std::endl;
}

void testGitStatusHelpers() {
    std::cout << "[Test] Git Status Helpers..." << std::endl;

    assert(GitService::statusToString(FileGitStatus::Modified) == "Modified");
    assert(GitService::statusToString(FileGitStatus::Added) == "Added");
    assert(GitService::statusToString(FileGitStatus::Untracked) == "Untracked");
    assert(GitService::statusToString(FileGitStatus::Unmodified) == "Clean");

    std::cout << "  ✓ Git status helpers passed!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Running LiteForge Master Test Suite..." << std::endl;
    std::cout << "========================================" << std::endl;

    testFuzzyMatch();
    testFormatFileSize();
    testLanguageDetection();
    testThemeManager();
    testGitIgnoreParsing();
    testProjectTemplates();
    testProfileManager();
    testLSPProtocol();
    testGitStatusHelpers();

    std::cout << "========================================" << std::endl;
    std::cout << "All LiteForge tests passed successfully! 🚀" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
