// Headless GUI tests (QT_QPA_PLATFORM=offscreen). They drive the real
// TabManager / CodeEditor / MainWindow and guard the bugs fixed in 1.2.0.
#undef NDEBUG
#include "liteforge/editor/TabManager.hpp"
#include "liteforge/core/Config.hpp"
#include "liteforge/core/FileUtils.hpp"
#include "liteforge/ui/MainWindow.hpp"
#include <QApplication>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QFile>
#include <QProcess>
#include <QTimer>
#include <QDateTime>
#include <cassert>
#include <iostream>

using namespace liteforge;

static QString writeFile(const QString& path, const QByteArray& data) {
    QFile f(path);
    bool ok = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    assert(ok);
    (void)ok;
    f.write(data);
    return path;
}

static QByteArray readRaw(const QString& path) {
    QFile f(path);
    bool ok = f.open(QIODevice::ReadOnly);
    assert(ok);
    (void)ok;
    return f.readAll();
}

static void testPreviewTabKeepsDirtyState() {
    std::cout << "[GUI] Editing a reused preview tab marks the NEW document dirty..." << std::endl;
    QTemporaryDir dir;
    const QString a = writeFile(dir.path() + "/a.cpp", "int a;\n");
    const QString b = writeFile(dir.path() + "/b.cpp", "int b;\n");

    editor::TabManager tm;
    tm.openFilePreview(a);
    auto* ed = tm.openFilePreview(b);   // reuses the clean preview editor
    assert(ed && tm.count() == 1);
    assert(ed->documentModel()->filePath() == b);

    ed->insertPlainText("// edit\n");
    // Regression: the edit used to flag the stale document, so this stayed false
    // and closing the tab silently discarded the user's work.
    assert(ed->documentModel()->isModified());
    assert(tm.tabText(0).contains("*"));
    std::cout << "  ok" << std::endl;
}

static void testSavePreservesFormatting() {
    std::cout << "[GUI] Save keeps CRLF + BOM, trims whitespace, adds final newline..." << std::endl;
    QTemporaryDir dir;
    const QString p = writeFile(dir.path() + "/win.cpp", QByteArray("\xEF\xBB\xBF" "int x;   \r\nint y;\r\nint z;"));

    editor::TabManager tm;
    auto* ed = tm.openFile(p);
    assert(ed);
    assert(ed->documentModel()->lineEnding() == core::LineEnding::CRLF);
    assert(ed->toPlainText() == "int x;   \nint y;\nint z;");

    ed->insertPlainText("// hi\n");
    assert(tm.saveCurrentFile());

    const QByteArray raw = readRaw(p);
    assert(raw.startsWith("\xEF\xBB\xBF"));
    assert(raw.contains("\r\n"));
    assert(!raw.contains("\r\r"));
    assert(!raw.contains("int x;   "));          // trailing whitespace trimmed
    assert(raw.endsWith("\r\n"));                 // final newline added, as CRLF
    assert(!ed->documentModel()->isModified());
    std::cout << "  ok" << std::endl;
}

static void testMarkdownKeepsTrailingSpaces() {
    std::cout << "[GUI] Markdown hard line breaks (two trailing spaces) survive save..." << std::endl;
    QTemporaryDir dir;
    const QString p = writeFile(dir.path() + "/n.md", "line one  \nline two\n");
    editor::TabManager tm;
    auto* ed = tm.openFile(p);
    ed->insertPlainText("x");
    assert(tm.saveCurrentFile());
    assert(readRaw(p).contains("one  \n"));
    std::cout << "  ok" << std::endl;
}

static void testPinsSurviveTabChanges() {
    std::cout << "[GUI] Pinned state follows the tab, not its index..." << std::endl;
    QTemporaryDir dir;
    editor::TabManager tm;
    tm.openFile(writeFile(dir.path() + "/1.txt", "1"));
    tm.openFile(writeFile(dir.path() + "/2.txt", "2"));
    auto* third = tm.openFile(writeFile(dir.path() + "/3.txt", "3"));
    tm.togglePinTab(tm.indexOf(third));
    assert(tm.isTabPinned(tm.indexOf(third)));

    assert(tm.closeFile(0));                      // everything shifts left
    assert(tm.isTabPinned(tm.indexOf(third)));    // pin must still be on file 3
    assert(!tm.isTabPinned(0));                   // and NOT on whatever moved into its old slot
    std::cout << "  ok" << std::endl;
}

static void testUntitledAndAutosave() {
    std::cout << "[GUI] Untitled tabs keep their names; autosave never prompts..." << std::endl;
    QTemporaryDir dir;
    editor::TabManager tm;
    auto* u1 = tm.newUntitledDocument();
    auto* u2 = tm.newUntitledDocument();
    u1->insertPlainText("a");
    u2->insertPlainText("b");
    assert(tm.tabText(0).contains("Untitled-") && tm.tabText(1).contains("Untitled-"));
    assert(tm.tabText(0) != tm.tabText(1));

    const QString p = writeFile(dir.path() + "/auto.txt", "hello\n");
    auto* ed = tm.openFile(p);
    ed->insertPlainText("typed ");
    const int saved = tm.autoSaveModified();     // must skip the two untitled docs
    assert(saved == 1);
    assert(readRaw(p).startsWith("typed hello"));
    std::cout << "  ok" << std::endl;
}

static void testExternalChangeReload() {
    std::cout << "[GUI] Clean documents reload when changed on disk..." << std::endl;
    QTemporaryDir dir;
    const QString p = writeFile(dir.path() + "/ext.txt", "old\n");
    editor::TabManager tm;
    auto* ed = tm.openFile(p);
    assert(ed->toPlainText() == "old\n");

    writeFile(p, "new content\n");
    QFile f(p);
    bool ok = f.open(QIODevice::ReadWrite);
    assert(ok);
    (void)ok;
    f.setFileTime(QDateTime::currentDateTime().addSecs(5), QFileDevice::FileModificationTime);
    f.close();

    tm.checkExternalChanges();
    assert(ed->toPlainText() == "new content\n");
    assert(!ed->documentModel()->isModified());
    std::cout << "  ok" << std::endl;
}

static void testLargeFileGuard() {
    std::cout << "[GUI] Large files open without highlighter/minimap..." << std::endl;
    QTemporaryDir dir;
    auto& cfg = core::Config::instance();
    cfg.editor().largeFileKiB = 1;
    cfg.editor().minimap = true;
    const QString p = writeFile(dir.path() + "/big.cpp", QByteArray(4096, 'x') + "\n");
    editor::TabManager tm;
    auto* ed = tm.openFile(p);
    assert(ed->isLargeFile());
    assert(!ed->isMinimapEnabled());
    cfg.editor() = core::EditorConfig{};
    std::cout << "  ok" << std::endl;
}

static void testMainWindowIsLazy() {
    std::cout << "[GUI] MainWindow starts no shell and builds no panels until needed..." << std::endl;
    ui::MainWindow w;
    w.show();
    QApplication::processEvents();
    // No terminal shell, no git, no gdb, no LSP process at startup.
    assert(w.findChildren<QProcess*>().isEmpty());
    // Heavy dialogs are not instantiated yet.
    assert(w.findChildren<ui::SettingsDialog*>().isEmpty());
    assert(w.findChildren<ui::CommandPalette*>().isEmpty());
    assert(w.findChildren<ui::AboutDialog*>().isEmpty());
    assert(w.findChildren<ui::TerminalPanel*>().isEmpty());
    assert(w.findChildren<ui::GitPanel*>().isEmpty());
    assert(w.findChildren<ui::DebuggerPanel*>().isEmpty());

    // Opening a file doesn't create them either.
    QTemporaryDir dir;
    w.openFile(writeFile(dir.path() + "/t.cpp", "int main() {}\n"));
    QApplication::processEvents();
    assert(w.findChildren<ui::TerminalPanel*>().isEmpty());
    std::cout << "  ok" << std::endl;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("LiteForgeGuiTest");
    app.setOrganizationName("LiteForgeGuiTest");
    QStandardPaths::setTestModeEnabled(true);

    testPreviewTabKeepsDirtyState();
    testSavePreservesFormatting();
    testMarkdownKeepsTrailingSpaces();
    testPinsSurviveTabChanges();
    testUntitledAndAutosave();
    testExternalChangeReload();
    testLargeFileGuard();
    testMainWindowIsLazy();

    std::cout << "All GUI tests passed." << std::endl;
    return 0;
}
