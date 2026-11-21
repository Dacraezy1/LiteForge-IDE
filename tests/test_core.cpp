// Tests must keep assert() active even in Release (NDEBUG) builds.
#undef NDEBUG
#include "liteforge/core/FileUtils.hpp"
#include "liteforge/core/Document.hpp"
#include "liteforge/core/Theme.hpp"
#include "liteforge/core/Config.hpp"
#include "liteforge/core/Keybindings.hpp"
#include "liteforge/core/ProjectTemplates.hpp"
#include "liteforge/core/ProfileManager.hpp"
#include "liteforge/git/GitService.hpp"
#include "liteforge/lsp/LSPProtocol.hpp"
#include <iostream>
#include <cassert>
#include <QDir>
#include <QFile>
#include <QColor>
#include <QCoreApplication>
#include <QStandardPaths>
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


void testFileRoundTrip() {
    std::cout << "[Test] CRLF / BOM preserving atomic file IO..." << std::endl;
    QTemporaryDir dir;
    const QString path = dir.path() + "/crlf.txt";

    // CRLF + BOM file: read normalises to LF, write restores both.
    {
        QFile f(path);
        assert(f.open(QIODevice::WriteOnly));
        f.write("\xEF\xBB\xBF" "one\r\ntwo\r\n");
    }
    bool ok = false, crlf = false, bom = false;
    QString text = FileUtils::readFileText(path, &ok, &crlf, &bom);
    assert(ok && crlf && bom);
    assert(text == "one\ntwo\n");

    assert(FileUtils::writeFileText(path, "one\ntwo\nthree\n", crlf, bom));
    QFile f(path);
    assert(f.open(QIODevice::ReadOnly));
    const QByteArray raw = f.readAll();
    assert(raw == QByteArray("\xEF\xBB\xBF" "one\r\ntwo\r\nthree\r\n"));

    // Plain LF file stays LF.
    assert(FileUtils::writeFileUtf8(dir.path() + "/lf.txt", "a\nb\n"));
    text = FileUtils::readFileText(dir.path() + "/lf.txt", &ok, &crlf, &bom);
    assert(ok && !crlf && !bom && text == "a\nb\n");

    std::cout << "  ✓ File round-trip passed!" << std::endl;
}

void testThemeCustomisation() {
    std::cout << "[Test] User themes, derived palettes, accent override..." << std::endl;
    auto& tm = ThemeManager::instance();
    auto& cfg = Config::instance();

    // All built-ins resolve and produce valid colors.
    for (const auto& name : tm.builtinThemes()) {
        tm.applyTheme(name);
        assert(tm.currentThemeName() == name);
        assert(tm.currentTheme().editorBg.isValid() && tm.currentTheme().keyword.isValid());
    }
    assert(tm.builtinThemes().size() >= 9);

    // Light theme really is light.
    tm.applyTheme("LiteForge Daylight");
    assert(tm.currentTheme().editorBg.lightness() > 200);

    // Unknown name falls back instead of leaving a broken theme.
    tm.applyTheme("No Such Theme");
    assert(tm.currentThemeName() == "LiteForge Obsidian");

    // User theme from JSON: overrides on top of a base.
    QTemporaryDir cfgDir;
    QStandardPaths::setTestModeEnabled(true);
    QDir().mkpath(cfg.themesDir());
    {
        QFile f(cfg.themesDir() + "/unit-test.json");
        assert(f.open(QIODevice::WriteOnly));
        f.write(R"({"name":"Unit Test Theme","base":"Nord","colors":{"accent":"#ff0000","bogus":"x","keyword":"notacolor"}})");
    }
    tm.reloadUserThemes();
    bool found = false;
    for (const auto& n : tm.availableThemes()) found |= (n == "Unit Test Theme");
    assert(found);
    tm.applyTheme("Unit Test Theme");
    assert(tm.currentTheme().accent == QColor("#ff0000"));
    // invalid color ignored -> keeps Nord's keyword
    tm.applyTheme("Nord");
    const QColor nordKeyword = tm.currentTheme().keyword;
    tm.applyTheme("Unit Test Theme");
    assert(tm.currentTheme().keyword == nordKeyword);

    // Export template round-trips.
    const QString exported = cfg.themesDir() + "/exported.json";
    assert(tm.exportThemeTemplate(exported));
    ThemeColors loaded;
    assert(tm.loadThemeFile(exported, loaded));
    assert(loaded.accent == tm.currentTheme().accent);

    // Accent override applies on top of any theme and is stripped when cleared.
    cfg.ui().accentOverride = "#12ab34";
    tm.applyTheme("Gruvbox Dark");
    assert(tm.currentTheme().accent == QColor("#12ab34"));
    assert(tm.currentTheme().cursor == QColor("#12ab34"));
    cfg.ui().accentOverride.clear();
    tm.applyTheme("Obsidian-missing");

    // Density changes the stylesheet.
    cfg.ui().density = "compact";
    const QString compact = tm.generateStyleSheet();
    cfg.ui().density = "comfortable";
    const QString roomy = tm.generateStyleSheet();
    cfg.ui().density = "normal";
    assert(compact != roomy);
    assert(!compact.contains("{{"));   // every template token was substituted

    QFile::remove(cfg.themesDir() + "/unit-test.json");
    QFile::remove(exported);
    tm.reloadUserThemes();
    tm.applyTheme("LiteForge Obsidian");
    std::cout << "  ✓ Theme customisation passed!" << std::endl;
}

void testConfigAndKeybindings() {
    std::cout << "[Test] Config persistence & keybinding overrides..." << std::endl;
    QStandardPaths::setTestModeEnabled(true);
    auto& cfg = Config::instance();

    cfg.editor().fontSize = 17;
    cfg.editor().autoSaveDelayMs = 2500;
    cfg.editor().minimap = true;
    cfg.ui().density = "compact";
    cfg.ui().cornerRadius = 9;
    cfg.general().lspEnabled = false;
    cfg.setActiveTheme("Nord");
    cfg.save();

    cfg.editor() = EditorConfig{};
    cfg.ui() = UiConfig{};
    cfg.general() = GeneralConfig{};
    cfg.load();
    assert(cfg.editor().fontSize == 17);
    assert(cfg.editor().autoSaveDelayMs == 2500);
    assert(cfg.editor().minimap);
    assert(cfg.ui().density == "compact" && cfg.ui().cornerRadius == 9);
    assert(!cfg.general().lspEnabled);
    assert(cfg.activeTheme() == "Nord");

    // Out-of-range values are clamped, not trusted.
    {
        QFile f(cfg.configFilePath());
        assert(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write(R"({"editor":{"fontSize":9999,"tabWidth":-4},"ui":{"cornerRadius":500}})");
    }
    cfg.editor() = EditorConfig{};
    cfg.ui() = UiConfig{};
    cfg.load();
    assert(cfg.editor().fontSize == 72 && cfg.editor().tabWidth == 1 && cfg.ui().cornerRadius == 16);

    // Corrupt JSON must leave defaults intact.
    {
        QFile f(cfg.configFilePath());
        assert(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write("{ not json");
    }
    cfg.editor() = EditorConfig{};
    cfg.load();
    assert(cfg.editor().fontSize == EditorConfig{}.fontSize);

    // Keybinding ids + file round trip.
    assert(Keybindings::makeId("&File", "Open File...") == "File/Open File");
    assert(Keybindings::makeId("&Build", "Start Debugging (GDB)") == "Build/Start Debugging (GDB)");
    QTemporaryDir dir;
    const QString kb = dir.path() + "/kb.json";
    QMap<QString, QString> m{{"File/Save", "Ctrl+Alt+S"}, {"Edit/Find", ""}};
    assert(Keybindings::save(kb, m));
    assert(Keybindings::load(kb) == m);
    assert(Keybindings::load(dir.path() + "/missing.json").isEmpty());

    cfg.editor() = EditorConfig{};
    cfg.ui() = UiConfig{};
    cfg.general() = GeneralConfig{};
    cfg.setActiveTheme("LiteForge Obsidian");
    cfg.save();
    std::cout << "  ✓ Config & keybindings passed!" << std::endl;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("LiteForgeTest");
    app.setOrganizationName("LiteForgeTest");
    QStandardPaths::setTestModeEnabled(true); // never touch the real ~/.config
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
    testFileRoundTrip();
    testThemeCustomisation();
    testConfigAndKeybindings();

    std::cout << "========================================" << std::endl;
    std::cout << "All LiteForge tests passed successfully! 🚀" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
