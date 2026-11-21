#include "liteforge/ui/MainWindow.hpp"
#include "liteforge/core/FileUtils.hpp"
#include "liteforge/core/Keybindings.hpp"
#include "liteforge/core/ProjectTemplates.hpp"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QCloseEvent>
#include <QApplication>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QToolTip>
#include <QTextBlock>
#include <QTextCursor>
#include <QDir>
#include <QFileInfo>
#include <algorithm>

namespace liteforge::ui {

namespace {

QString lspLanguageId(core::Language l) {
    using L = core::Language;
    switch (l) {
    case L::Cpp: case L::Header: return "cpp";
    case L::C: return "c";
    case L::Python: return "python";
    case L::Rust: return "rust";
    case L::JavaScript: return "javascript";
    case L::TypeScript: return "typescript";
    case L::Json: return "json";
    case L::Markdown: return "markdown";
    case L::Html: return "html";
    case L::Css: return "css";
    case L::Shell: return "shellscript";
    case L::CMake: return "cmake";
    case L::Makefile: return "makefile";
    default: return "plaintext";
    }
}

// Applies LSP TextEdits by range, last-to-first, as ONE undoable edit.
// (The previous implementation replaced the whole document with each edit's
// text, which destroyed the file on rename/format.)
void applyTextEdits(editor::CodeEditor* ed, std::vector<lsp::TextEdit> edits) {
    if (!ed || edits.empty()) return;
    std::sort(edits.begin(), edits.end(), [](const lsp::TextEdit& a, const lsp::TextEdit& b) {
        if (a.range.start.line != b.range.start.line) return a.range.start.line > b.range.start.line;
        return a.range.start.character > b.range.start.character;
    });
    QTextDocument* doc = ed->document();
    auto toPos = [doc](const lsp::Position& p) {
        const QTextBlock b = doc->findBlockByNumber(p.line);
        if (!b.isValid()) return qMax(0, doc->characterCount() - 1);
        return b.position() + qBound(0, p.character, qMax(0, b.length() - 1));
    };
    QTextCursor c(doc);
    c.beginEditBlock();
    for (const auto& e : edits) {
        c.setPosition(toPos(e.range.start));
        c.setPosition(toPos(e.range.end), QTextCursor::KeepAnchor);
        QString text = e.newText;
        text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
        c.insertText(text);
    }
    c.endEditBlock();
}

// Visit every leaf action of the menu bar with its "Menu/Action" id.
template <typename Fn>
void forEachMenuAction(QMenuBar* bar, Fn fn) {
    for (QAction* top : bar->actions()) {
        QMenu* menu = top->menu();
        if (!menu) continue;
        for (QAction* a : menu->actions()) {
            if (a->isSeparator() || a->menu()) continue;
            fn(core::Keybindings::makeId(menu->title(), a->text()), a);
        }
    }
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_project(std::make_unique<core::Project>(this)),
      m_gitService(std::make_unique<git::GitService>(this)),
      m_buildSystem(std::make_unique<build::BuildSystem>(this)),
      m_debugger(std::make_unique<build::Debugger>(this)),
      m_lspClient(std::make_unique<lsp::LSPClient>(this)) {

    auto& cfg = core::Config::instance();
    cfg.load();
    core::ThemeManager::instance().reloadUserThemes();

    setWindowTitle("LiteForge IDE");
    resize(1200, 800);

    setupUi();
    setupMenus();
    setupToolBar();
    setupShortcuts();
    setupConnections();
    setupAutoSave();

    // Remember each action's default key so a config reload can fully reset.
    forEachMenuAction(menuBar(), [](const QString&, QAction* a) {
        a->setProperty("lf_default_key", a->shortcut().toString());
    });
    applyKeybindings();
    applyUiSettings();

    loadSession();
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------- UI setup

void MainWindow::setupUi() {
    const auto& cfg = core::Config::instance();

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_mainSplitter);

    // Sidebar: Files is real, Source Control is built on first visit.
    m_sidebarTabWidget = new QTabWidget(m_mainSplitter);
    m_sidebarTabWidget->setDocumentMode(true);
    m_sidebarTabWidget->setMinimumWidth(180);

    m_fileExplorer = new FileExplorer(m_sidebarTabWidget);
    m_gitPlaceholder = new QWidget(m_sidebarTabWidget);
    m_sidebarTabWidget->addTab(m_fileExplorer, "Files");
    m_sidebarTabWidget->addTab(m_gitPlaceholder, "Source Control");
    connect(m_sidebarTabWidget, &QTabWidget::currentChanged, this, &MainWindow::onSidebarTabChanged);

    m_rightSplitter = new QSplitter(Qt::Vertical, m_mainSplitter);

    m_centerStack = new QStackedWidget(m_rightSplitter);
    m_welcomeScreen = new WelcomeScreen(m_centerStack);
    m_editorSplitter = new editor::EditorSplitter(m_centerStack);
    m_centerStack->addWidget(m_welcomeScreen);
    m_centerStack->addWidget(m_editorSplitter);
    m_centerStack->setCurrentIndex(0);

    // Bottom: Problems is real (cheap, receives build/LSP output), Terminal and
    // Debugger are built when first shown or first needed.
    m_bottomTabWidget = new QTabWidget(m_rightSplitter);
    m_bottomTabWidget->setDocumentMode(true);
    m_bottomTabWidget->setMinimumHeight(120);

    m_terminalPlaceholder = new QWidget(m_bottomTabWidget);
    m_diagnosticsPanel = new DiagnosticsPanel(m_bottomTabWidget);
    m_debuggerPlaceholder = new QWidget(m_bottomTabWidget);
    m_bottomTabWidget->addTab(m_terminalPlaceholder, "Terminal & Output");
    m_bottomTabWidget->addTab(m_diagnosticsPanel, "Problems (0)");
    m_bottomTabWidget->addTab(m_debuggerPlaceholder, "Debugger");
    connect(m_bottomTabWidget, &QTabWidget::currentChanged, this, &MainWindow::onBottomTabChanged);

    m_mainSplitter->setSizes(QList<int>() << 240 << 960);
    m_rightSplitter->setSizes(QList<int>() << 620 << 200);
    m_rightSplitter->setStretchFactor(0, 1);
    m_rightSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setStretchFactor(1, 1);

    m_statusBar = new StatusBar(this);
    setStatusBar(m_statusBar);

    m_welcomeScreen->updateRecentProjects(cfg.recentProjects());

    // Restore layout from last session.
    const auto& w = cfg.window();
    if (!w.geometry.isEmpty()) restoreGeometry(w.geometry);
    if (!w.mainSplitterState.isEmpty()) m_mainSplitter->restoreState(w.mainSplitterState);
    if (!w.bottomSplitterState.isEmpty()) m_rightSplitter->restoreState(w.bottomSplitterState);
    m_sidebarTabWidget->setVisible(w.sidebarVisible);
    m_bottomTabWidget->setVisible(w.bottomPanelVisible);
}

GitPanel* MainWindow::ensureGitPanel() {
    if (m_gitPanel) return m_gitPanel;
    m_gitPanel = new GitPanel(m_gitService.get(), m_sidebarTabWidget);
    connect(m_gitPanel, &GitPanel::fileSelected, this, [this](const QString& relPath) {
        if (m_project->isOpen()) openFile(m_project->rootPath() + "/" + relPath);
    });
    const int idx = m_sidebarTabWidget->indexOf(m_gitPlaceholder);
    const QSignalBlocker block(m_sidebarTabWidget);
    m_sidebarTabWidget->removeTab(idx);
    m_sidebarTabWidget->insertTab(idx, m_gitPanel, "Source Control");
    m_sidebarTabWidget->setCurrentIndex(idx);
    m_gitPlaceholder->deleteLater();
    m_gitPlaceholder = nullptr;
    m_gitPanel->refresh();
    return m_gitPanel;
}

TerminalPanel* MainWindow::ensureTerminal() {
    if (m_terminalPanel) return m_terminalPanel;
    m_terminalPanel = new TerminalPanel(m_bottomTabWidget); // spawns the shell
    const int idx = m_bottomTabWidget->indexOf(m_terminalPlaceholder);
    const bool wasCurrent = m_bottomTabWidget->currentIndex() == idx;
    const QSignalBlocker block(m_bottomTabWidget);
    m_bottomTabWidget->removeTab(idx);
    m_bottomTabWidget->insertTab(idx, m_terminalPanel, "Terminal & Output");
    if (wasCurrent) m_bottomTabWidget->setCurrentIndex(idx);
    m_terminalPlaceholder->deleteLater();
    m_terminalPlaceholder = nullptr;
    return m_terminalPanel;
}

DebuggerPanel* MainWindow::ensureDebugger() {
    if (m_debuggerPanel) return m_debuggerPanel;
    m_debuggerPanel = new DebuggerPanel(m_debugger.get(), m_bottomTabWidget);
    connect(m_debuggerPanel, &DebuggerPanel::jumpToSourceRequested, this, &MainWindow::onJumpToSource);
    const int idx = m_bottomTabWidget->indexOf(m_debuggerPlaceholder);
    const bool wasCurrent = m_bottomTabWidget->currentIndex() == idx;
    const QSignalBlocker block(m_bottomTabWidget);
    m_bottomTabWidget->removeTab(idx);
    m_bottomTabWidget->insertTab(idx, m_debuggerPanel, "Debugger");
    if (wasCurrent) m_bottomTabWidget->setCurrentIndex(idx);
    m_debuggerPlaceholder->deleteLater();
    m_debuggerPlaceholder = nullptr;
    return m_debuggerPanel;
}

void MainWindow::onSidebarTabChanged(int index) {
    if (!m_gitPanel && m_sidebarTabWidget->widget(index) == m_gitPlaceholder) ensureGitPanel();
}

void MainWindow::onBottomTabChanged(int index) {
    QWidget* w = m_bottomTabWidget->widget(index);
    if (w && w == m_terminalPlaceholder) ensureTerminal();
    else if (w && w == m_debuggerPlaceholder) ensureDebugger();
}

CommandPalette* MainWindow::palette() {
    if (m_commandPalette) return m_commandPalette;
    m_commandPalette = new CommandPalette(this);
    connect(m_commandPalette, &CommandPalette::fileSelected, this, [this](const QString& path) {
        // Quick Open results were never connected before: picking a file did nothing.
        QString p = path;
        if (QFileInfo(p).isRelative() && m_project->isOpen()) p = m_project->rootPath() + "/" + p;
        openFile(p);
    });

    // The palette is generated from the menus, so it always lists every
    // command and shows the user's current (possibly remapped) shortcuts.
    forEachMenuAction(menuBar(), [this](const QString& id, QAction* a) {
        if (id == QLatin1String("Help/Command Palette")) return;
        QString title = id;
        title.replace(QLatin1Char('/'), QStringLiteral(": "));
        m_commandPalette->addActionItem(title, [a]() { a->trigger(); },
                                        a->shortcut().toString(QKeySequence::NativeText));
    });
    m_commandPalette->addActionItem("Git: Refresh Status", [this]() { ensureGitPanel()->refresh(); });
    return m_commandPalette;
}

FindInFilesDialog* MainWindow::findInFiles() {
    if (!m_findInFilesDialog) {
        m_findInFilesDialog = new FindInFilesDialog(this);
        connect(m_findInFilesDialog, &FindInFilesDialog::resultSelected, this,
                [this](const QString& file, int line) { onJumpToSource(file, line, 1); });
    }
    return m_findInFilesDialog;
}

SettingsDialog* MainWindow::settingsDialog() {
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
        connect(m_settingsDialog, &SettingsDialog::settingsApplied, this, &MainWindow::onSettingsApplied);
    }
    return m_settingsDialog;
}

AboutDialog* MainWindow::aboutDialog() {
    if (!m_aboutDialog) m_aboutDialog = new AboutDialog(this);
    return m_aboutDialog;
}

CompletionPopup* MainWindow::completionPopup() {
    if (!m_completionPopup) {
        m_completionPopup = new CompletionPopup(this);
        connect(m_completionPopup, &CompletionPopup::itemSelected, this, [this](const QString& text) {
            if (auto* ed = m_editorSplitter->currentEditor()) ed->textCursor().insertText(text);
        });
    }
    return m_completionPopup;
}

// ------------------------------------------------------------------- menus

void MainWindow::setupMenus() {
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("New File", QKeySequence::New, this, &MainWindow::newFile);
    fileMenu->addAction("New Project from Template...", this, &MainWindow::newProjectFromTemplate);
    fileMenu->addAction("Open File...", QKeySequence::Open, this, &MainWindow::openFileDialog);
    fileMenu->addAction("Open Folder...", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), this, &MainWindow::openFolderDialog);
    fileMenu->addSeparator();
    fileMenu->addAction("Save", QKeySequence::Save, this, &MainWindow::saveFile);
    fileMenu->addAction("Save As...", QKeySequence::SaveAs, this, &MainWindow::saveFileAs);
    fileMenu->addAction("Save All", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), this, &MainWindow::saveAllFiles);
    fileMenu->addSeparator();
    fileMenu->addAction("Close Tab", QKeySequence::Close, this, &MainWindow::closeActiveTab);
    fileMenu->addAction("Exit", QKeySequence::Quit, this, &QWidget::close);

    QMenu* editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("Undo", QKeySequence::Undo, this, &MainWindow::undo);
    editMenu->addAction("Redo", QKeySequence::Redo, this, &MainWindow::redo);
    editMenu->addSeparator();
    editMenu->addAction("Cut", QKeySequence::Cut, this, &MainWindow::cut);
    editMenu->addAction("Copy", QKeySequence::Copy, this, &MainWindow::copy);
    editMenu->addAction("Paste", QKeySequence::Paste, this, &MainWindow::paste);
    editMenu->addAction("Select All", QKeySequence::SelectAll, this, &MainWindow::selectAll);
    editMenu->addSeparator();
    editMenu->addAction("Find...", QKeySequence::Find, this, &MainWindow::showFind);
    editMenu->addAction("Replace...", QKeySequence::Replace, this, &MainWindow::showReplace);
    editMenu->addAction("Find in Files...", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), this, &MainWindow::showFindInFiles);
    editMenu->addSeparator();
    editMenu->addAction("Select Next Occurrence", QKeySequence(Qt::CTRL | Qt::Key_D), this, &MainWindow::selectNextOccurrence);
    editMenu->addAction("Toggle Comment", QKeySequence(Qt::CTRL | Qt::Key_Slash), this, &MainWindow::toggleComment);
    editMenu->addAction("Duplicate Line", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D), this, &MainWindow::duplicateLine);
    editMenu->addAction("Delete Line", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K), this, &MainWindow::deleteCurrentLine);
    editMenu->addAction("Move Line Up", QKeySequence(Qt::ALT | Qt::Key_Up), this, &MainWindow::moveLineUp);
    editMenu->addAction("Move Line Down", QKeySequence(Qt::ALT | Qt::Key_Down), this, &MainWindow::moveLineDown);
    editMenu->addAction("Go to Line...", QKeySequence(Qt::CTRL | Qt::Key_G), this, &MainWindow::gotoLine);

    QMenu* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Toggle Sidebar", QKeySequence(Qt::CTRL | Qt::Key_B), this, &MainWindow::toggleSidebar);
    viewMenu->addAction("Toggle Bottom Panel", QKeySequence(Qt::CTRL | Qt::Key_AsciiTilde), this, &MainWindow::toggleBottomPanel);
    viewMenu->addAction("Toggle Toolbar", this, &MainWindow::toggleToolbar);
    viewMenu->addAction("Toggle Distraction-Free", QKeySequence(Qt::Key_F11), this, &MainWindow::toggleDistractionFree);
    viewMenu->addSeparator();
    viewMenu->addAction("Split Right (Vertical)", QKeySequence(Qt::CTRL | Qt::Key_Backslash), this, &MainWindow::splitVertical);
    viewMenu->addAction("Split Down (Horizontal)", this, &MainWindow::splitHorizontal);
    viewMenu->addSeparator();
    viewMenu->addAction("Toggle Minimap", this, &MainWindow::toggleMinimap);
    viewMenu->addAction("Toggle Sticky Scroll", this, &MainWindow::toggleStickyScroll);
    viewMenu->addAction("Toggle Word Wrap", this, &MainWindow::toggleWordWrap);
    viewMenu->addSeparator();
    viewMenu->addAction("Zoom In", QKeySequence::ZoomIn, this, &MainWindow::zoomIn);
    viewMenu->addAction("Zoom Out", QKeySequence::ZoomOut, this, &MainWindow::zoomOut);
    viewMenu->addAction("Reset Zoom", QKeySequence(Qt::CTRL | Qt::Key_0), this, &MainWindow::resetZoom);

    QMenu* buildMenu = menuBar()->addMenu("&Build");
    buildMenu->addAction("Build Project", QKeySequence(Qt::CTRL | Qt::Key_F7), this, &MainWindow::buildProject);
    buildMenu->addAction("Run Project", QKeySequence(Qt::Key_F5), this, &MainWindow::runProject);
    buildMenu->addAction("Clean Project", this, &MainWindow::cleanProject);
    buildMenu->addAction("Rebuild Project", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B), this, &MainWindow::rebuildProject);
    buildMenu->addAction("Stop Build", this, &MainWindow::stopBuild);
    buildMenu->addSeparator();
    buildMenu->addAction("Start Debugging (GDB)", QKeySequence(Qt::Key_F9), this, &MainWindow::startDebugging);
    buildMenu->addAction("Stop Debugging", QKeySequence(Qt::SHIFT | Qt::Key_F5), this, &MainWindow::stopDebugging);

    m_profileMenu = menuBar()->addMenu("&Profiles");
    rebuildProfileMenu();

    m_themeMenu = menuBar()->addMenu("&Theme");
    rebuildThemeMenu();

    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("Command Palette...", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P), this, &MainWindow::showCommandPalette);
    helpMenu->addAction("Quick Open...", QKeySequence(Qt::CTRL | Qt::Key_P), this, &MainWindow::showQuickOpen);
    helpMenu->addSeparator();
    helpMenu->addAction("Settings / Preferences...", QKeySequence(Qt::CTRL | Qt::Key_Comma), this, &MainWindow::showSettings);
    helpMenu->addAction("Open Config File (JSON)", this, &MainWindow::openConfigFile);
    helpMenu->addAction("Open Keybindings (JSON)", this, &MainWindow::openKeybindingsFile);
    helpMenu->addAction("Open Themes Folder", this, &MainWindow::openThemesFolder);
    helpMenu->addAction("Export Current Theme as Template...", this, &MainWindow::exportThemeTemplate);
    helpMenu->addAction("Reload Config, Themes && Keys", this, &MainWindow::reloadConfig);
    helpMenu->addSeparator();
    helpMenu->addAction("About LiteForge", this, &MainWindow::showAbout);
}

void MainWindow::rebuildProfileMenu() {
    m_profileMenu->clear();
    const QStringList profiles = core::ProfileManager::instance().availableProfiles();
    for (const QString& profName : profiles)
        m_profileMenu->addAction(profName, [this, profName]() { switchProfile(profName); });
    m_profileMenu->addSeparator();
    m_profileMenu->addAction("Export Current Profile...", this, &MainWindow::exportProfile);
    m_profileMenu->addAction("Import Profile...", this, &MainWindow::importProfile);
}

void MainWindow::rebuildThemeMenu() {
    m_themeMenu->clear();
    for (const auto& name : core::ThemeManager::instance().availableThemes())
        m_themeMenu->addAction(name, [this, name]() { switchTheme(name); });
}

void MainWindow::setupToolBar() {
    m_toolBar = addToolBar("Main Controls");
    m_toolBar->setObjectName("mainToolBar");
    m_toolBar->setMovable(false);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    m_toolBar->addAction("New", this, &MainWindow::newFile);
    m_toolBar->addAction("Open", this, &MainWindow::openFileDialog);
    m_toolBar->addAction("Save", this, &MainWindow::saveFile);
    m_toolBar->addSeparator();
    m_toolBar->addAction("Run", this, &MainWindow::runProject);
    m_toolBar->addAction("Build", this, &MainWindow::buildProject);
    m_toolBar->addAction("Debug", this, &MainWindow::startDebugging);
    m_toolBar->addSeparator();
    m_toolBar->addAction("Commands", this, &MainWindow::showCommandPalette);
}

void MainWindow::setupShortcuts() {}

void MainWindow::setupConnections() {
    connect(m_editorSplitter, &editor::EditorSplitter::activeEditorChanged, this, &MainWindow::onActiveEditorChanged);
    connect(m_editorSplitter, &editor::EditorSplitter::fileOpened, this, [this](const QString& path) {
        m_statusBar->showMessage("Opened: " + QFileInfo(path).fileName(), 3000);
        updateCenterView();
    });
    connect(m_editorSplitter, &editor::EditorSplitter::allFilesClosed, this, &MainWindow::updateCenterView);

    // Saving config.json / keybindings.json / a theme file applies it immediately.
    connect(m_editorSplitter, &editor::EditorSplitter::fileSaved, this, [this](const QString& path) {
        const QString dir = QFileInfo(core::Config::instance().configFilePath()).absolutePath();
        if (QFileInfo(path).absoluteFilePath().startsWith(dir + "/")) reloadConfig();
    });

    connect(m_welcomeScreen, &WelcomeScreen::newFileRequested, this, &MainWindow::newFile);
    connect(m_welcomeScreen, &WelcomeScreen::openFileRequested, this, &MainWindow::openFileDialog);
    connect(m_welcomeScreen, &WelcomeScreen::openProjectRequested, this, &MainWindow::openFolderDialog);
    connect(m_welcomeScreen, &WelcomeScreen::projectSelected, this, &MainWindow::openProject);

    connect(m_fileExplorer, &FileExplorer::fileSelected, this, [this](const QString& path) { openFilePreview(path); });

    connect(m_gitService.get(), &git::GitService::branchChanged, this,
            [this](const QString& branch) { m_statusBar->setGitBranch(branch); });

    connect(m_buildSystem.get(), &build::BuildSystem::buildStarted, this, &MainWindow::onBuildStarted);
    connect(m_buildSystem.get(), &build::BuildSystem::buildFinished, this, &MainWindow::onBuildFinished);
    connect(m_buildSystem.get(), &build::BuildSystem::outputReceived, this, &MainWindow::onBuildOutput);
    connect(m_buildSystem.get(), &build::BuildSystem::diagnosticFound, this, &MainWindow::onDiagnosticFound);

    connect(m_diagnosticsPanel, &DiagnosticsPanel::itemSelected, this, &MainWindow::onJumpToSource);

    connect(m_lspClient.get(), &lsp::LSPClient::diagnosticsReceived, this,
            [this](const QString& file, const std::vector<lsp::Diagnostic>& diags) {
        m_diagnosticsPanel->removeForFile(file); // each publish replaces the file's previous list
        for (const auto& d : diags) {
            DiagnosticSeverity sev = DiagnosticSeverity::Error;
            if (d.severity == 2) sev = DiagnosticSeverity::Warning;
            else if (d.severity >= 3) sev = DiagnosticSeverity::Information;
            m_diagnosticsPanel->addDiagnostic(sev, d.message, file, d.range.start.line + 1, d.range.start.character + 1);
        }
        m_diagnosticsPanel->updateTabTitle(m_bottomTabWidget, 1);
    });
    connect(m_lspClient.get(), &lsp::LSPClient::initialized, this, &MainWindow::announceOpenDocumentsToLsp);
}

void MainWindow::setupAutoSave() {
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);
    connect(m_autoSaveTimer, &QTimer::timeout, this, [this]() { m_editorSplitter->autoSaveModified(); });

    m_lspSyncTimer = new QTimer(this);
    m_lspSyncTimer->setSingleShot(true);
    m_lspSyncTimer->setInterval(400);
    connect(m_lspSyncTimer, &QTimer::timeout, this, &MainWindow::flushLspChanges);

    connect(m_editorSplitter, &editor::EditorSplitter::documentEdited, this, [this](editor::CodeEditor* ed) {
        const int delay = core::Config::instance().editor().autoSaveDelayMs;
        if (delay > 0) m_autoSaveTimer->start(delay);
        queueLspChange(ed);
    });

    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState st) {
        if (st == Qt::ApplicationInactive && core::Config::instance().editor().autoSaveOnFocusLoss) {
            m_editorSplitter->autoSaveModified();
        } else if (st == Qt::ApplicationActive) {
            QTimer::singleShot(0, this, [this]() { m_editorSplitter->checkExternalChanges(); });
        }
    });
}

// ------------------------------------------------------- look & customisation

void MainWindow::applyTheme(const QString& themeName) {
    auto& tm = core::ThemeManager::instance();
    tm.applyTheme(themeName);
    setStyleSheet(tm.generateStyleSheet());
    m_editorSplitter->applyTheme(tm.currentTheme());
}

void MainWindow::applyUiSettings() {
    const auto& ui = core::Config::instance().ui();

    static const QFont baseFont = QApplication::font();
    QFont f = baseFont;
    if (!ui.fontFamily.isEmpty()) f.setFamily(ui.fontFamily);
    if (ui.fontSize > 0) f.setPointSize(ui.fontSize);
    if (f != QApplication::font()) QApplication::setFont(f);

    applyTheme(core::Config::instance().activeTheme());

    if (m_toolBar) m_toolBar->setVisible(ui.showToolbar);
    if (m_statusBar) m_statusBar->setVisible(ui.showStatusBar);
}

void MainWindow::applyKeybindings() {
    const auto overrides = core::Keybindings::load(core::Config::instance().keybindingsFilePath());
    forEachMenuAction(menuBar(), [&](const QString& id, QAction* a) {
        const QString def = a->property("lf_default_key").toString();
        const auto it = overrides.constFind(id);
        a->setShortcut(QKeySequence(it != overrides.constEnd() ? it.value() : def));
    });
}

QString MainWindow::keyFor(const QString& actionId) const {
    QString out;
    forEachMenuAction(menuBar(), [&](const QString& id, QAction* a) {
        if (id == actionId) out = a->shortcut().toString(QKeySequence::NativeText);
    });
    return out;
}

void MainWindow::reloadConfig() {
    auto& cfg = core::Config::instance();
    cfg.load();
    core::ThemeManager::instance().reloadUserThemes();
    rebuildThemeMenu();

    // Dialogs and the palette snapshot theme lists / shortcuts: rebuild on next use.
    if (m_commandPalette) { m_commandPalette->deleteLater(); m_commandPalette = nullptr; }
    if (m_settingsDialog) { m_settingsDialog->deleteLater(); m_settingsDialog = nullptr; }

    applyKeybindings();
    applyUiSettings();
    m_editorSplitter->refreshEditorSettings();
    m_statusBar->showMessage("Config, themes and keybindings reloaded", 3000);
}

void MainWindow::openConfigTextFile(const QString& path, const QString& templateContent) {
    if (!QFileInfo::exists(path)) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        core::FileUtils::writeFileUtf8(path, templateContent);
    }
    openFile(path);
}

void MainWindow::openConfigFile() {
    auto& cfg = core::Config::instance();
    cfg.save(); // make sure the file reflects the live state before editing
    openConfigTextFile(cfg.configFilePath(), "{}\n");
}

void MainWindow::openKeybindingsFile() {
    const QString path = core::Config::instance().keybindingsFilePath();
    if (!QFileInfo::exists(path)) {
        // Seed with every action's current binding: edit a value, or set "" to unbind.
        QMap<QString, QString> all;
        forEachMenuAction(menuBar(), [&](const QString& id, QAction* a) {
            all.insert(id, a->shortcut().toString());
        });
        core::Keybindings::save(path, all);
    }
    openFile(path);
}

void MainWindow::openThemesFolder() {
    const QString dir = core::Config::instance().themesDir();
    QDir().mkpath(dir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void MainWindow::exportThemeTemplate() {
    const QString dir = core::Config::instance().themesDir();
    QDir().mkpath(dir);
    const QString path = QFileDialog::getSaveFileName(this, "Export Theme Template", dir + "/my-theme.json", "JSON Files (*.json)");
    if (path.isEmpty()) return;
    if (core::ThemeManager::instance().exportThemeTemplate(path)) {
        openFile(path);
        m_statusBar->showMessage("Edit the colors, save, and pick it from the Theme menu", 5000);
    }
}

void MainWindow::toggleToolbar() {
    auto& ui = core::Config::instance().ui();
    ui.showToolbar = !ui.showToolbar;
    m_toolBar->setVisible(ui.showToolbar);
}

// ------------------------------------------------------------------ session

void MainWindow::updateCenterView() {
    if (m_editorSplitter->countAllTabs() == 0) {
        m_centerStack->setCurrentIndex(0);
        m_welcomeScreen->updateRecentProjects(core::Config::instance().recentProjects());
    } else {
        m_centerStack->setCurrentIndex(1);
    }
}

void MainWindow::loadSession() {
    auto& cfg = core::Config::instance();
    if (!cfg.general().restoreSession) { updateCenterView(); return; }

    if (!cfg.lastProjectPath().isEmpty() && QDir(cfg.lastProjectPath()).exists())
        openProject(cfg.lastProjectPath());

    // Restore tabs after the first paint so the window appears immediately.
    QTimer::singleShot(0, this, &MainWindow::restoreOpenFiles);
    updateCenterView();
}

void MainWindow::restoreOpenFiles() {
    const auto& cfg = core::Config::instance();
    for (const QString& f : cfg.openFiles()) {
        if (QFile::exists(f) && f != cfg.activeFile()) openFile(f);
    }
    // Active file last so it ends up focused.
    if (!cfg.activeFile().isEmpty() && QFile::exists(cfg.activeFile())) openFile(cfg.activeFile());
    updateCenterView();
}

void MainWindow::saveSession() {
    auto& cfg = core::Config::instance();
    cfg.setOpenFiles(m_editorSplitter->openFilePaths());
    if (auto* ed = m_editorSplitter->currentEditor()) {
        if (ed->documentModel() && !ed->documentModel()->isUntitled())
            cfg.setActiveFile(ed->documentModel()->filePath());
    }
    auto& w = cfg.window();
    if (!m_distractionFree) w.geometry = saveGeometry();
    w.mainSplitterState = m_mainSplitter->saveState();
    w.bottomSplitterState = m_rightSplitter->saveState();
    w.sidebarVisible = m_sidebarTabWidget->isVisible() || m_distractionFree;
    w.bottomPanelVisible = m_bottomTabWidget->isVisible();
    cfg.save();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Ask about unsaved work first; only then persist the session.
    if (!m_editorSplitter->closeAllFiles()) {
        event->ignore();
        return;
    }
    saveSession();
    event->accept();
}

// ----------------------------------------------------------- project / files

void MainWindow::startLspIfConfigured(const QString& rootPath) {
    if (!core::Config::instance().general().lspEnabled) return;
    const auto& prof = core::ProfileManager::instance().currentProfile();
    if (prof.lspExecutable.isEmpty()) return;
    m_lspVersions.clear();
    m_lspClient->startServer(prof.lspExecutable, prof.lspArgs, rootPath);
}

void MainWindow::openProject(const QString& path) {
    if (!m_project->open(path)) return;

    m_fileExplorer->setRootPath(path);
    m_gitService->setRepositoryPath(path);
    m_statusBar->setGitBranch(m_project->gitBranch());

    core::Config::instance().setLastProjectPath(path);
    core::Config::instance().addRecentProject(path);

    setWindowTitle(QString("LiteForge IDE — %1 [%2]").arg(m_project->projectName(), m_project->buildTypeString()));
    startLspIfConfigured(path);
}

void MainWindow::openFile(const QString& path) {
    auto* editor = m_editorSplitter->openFile(path);
    if (!editor) return;
    updateCenterView();

    if (m_lspClient->isInitialized() && editor->documentModel() && !m_lspVersions.contains(path)) {
        m_lspVersions.insert(path, 1);
        m_lspClient->didOpen(path, lspLanguageId(editor->documentModel()->language()), 1, editor->toPlainText());
    }
}

void MainWindow::openFilePreview(const QString& path) {
    if (m_editorSplitter->openFilePreview(path)) updateCenterView();
}

void MainWindow::announceOpenDocumentsToLsp() {
    // Files restored before the server finished its handshake were never announced.
    for (const QString& path : m_editorSplitter->openFilePaths()) {
        auto* ed = m_editorSplitter->editorForPath(path);
        if (!ed || !ed->documentModel() || m_lspVersions.contains(path)) continue;
        m_lspVersions.insert(path, 1);
        m_lspClient->didOpen(path, lspLanguageId(ed->documentModel()->language()), 1, ed->toPlainText());
    }
}

void MainWindow::queueLspChange(editor::CodeEditor* editor) {
    if (!editor || !m_lspClient->isInitialized()) return;
    for (const auto& p : m_lspPending) if (p == editor) { m_lspSyncTimer->start(); return; }
    m_lspPending.emplace_back(editor);
    m_lspSyncTimer->start();
}

void MainWindow::flushLspChanges() {
    auto pending = std::move(m_lspPending);
    m_lspPending.clear();
    if (!m_lspClient->isInitialized()) return;
    for (const auto& p : pending) {
        if (!p || !p->documentModel()) continue;
        const QString path = p->documentModel()->filePath();
        auto it = m_lspVersions.find(path);
        if (path.isEmpty() || it == m_lspVersions.end()) continue;
        m_lspClient->didChange(path, ++it.value(), p->toPlainText());
    }
}

void MainWindow::newFile() {
    m_editorSplitter->newUntitledDocument();
    updateCenterView();
}

void MainWindow::newProjectFromTemplate() {
    const QStringList templates = core::ProjectTemplates::availableTemplates();
    bool ok = false;
    const QString chosen = QInputDialog::getItem(this, "New Project", "Choose Project Template:", templates, 0, false, &ok);
    if (!ok || chosen.isEmpty()) return;

    const QString dir = QFileDialog::getExistingDirectory(this, "Select Project Destination Directory");
    if (dir.isEmpty()) return;

    const QString name = QInputDialog::getText(this, "Project Name", "Enter project name:", QLineEdit::Normal, "MyProject", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    core::TemplateType type = core::TemplateType::CppCMake;
    if (chosen.contains("Rust")) type = core::TemplateType::RustCargo;
    else if (chosen.contains("Python")) type = core::TemplateType::PythonApp;
    else if (chosen.contains("Web")) type = core::TemplateType::WebMinimal;

    QString err;
    const QString fullPath = dir + "/" + name.trimmed();
    if (core::ProjectTemplates::createProject(type, fullPath, name.trimmed(), &err)) openProject(fullPath);
    else QMessageBox::warning(this, "Template Error", err);
}

void MainWindow::openFileDialog() {
    const QStringList paths = QFileDialog::getOpenFileNames(this, "Open File");
    for (const QString& p : paths) openFile(p);
}

void MainWindow::openFolderDialog() {
    const QString path = QFileDialog::getExistingDirectory(this, "Open Folder");
    if (!path.isEmpty()) openProject(path);
}

void MainWindow::saveFile() { m_editorSplitter->saveCurrentFile(); }   // untitled -> Save As dialog
void MainWindow::saveFileAs() { m_editorSplitter->saveCurrentFileAs(); } // renames the tab in place
void MainWindow::saveAllFiles() { m_editorSplitter->saveAllFiles(); }

void MainWindow::closeActiveTab() {
    m_editorSplitter->closeActiveTab();
    updateCenterView();
}

// -------------------------------------------------------------- edit actions

void MainWindow::undo() { if (auto* ed = m_editorSplitter->currentEditor()) ed->undo(); }
void MainWindow::redo() { if (auto* ed = m_editorSplitter->currentEditor()) ed->redo(); }
void MainWindow::cut() { if (auto* ed = m_editorSplitter->currentEditor()) ed->cut(); }
void MainWindow::copy() { if (auto* ed = m_editorSplitter->currentEditor()) ed->copy(); }
void MainWindow::paste() { if (auto* ed = m_editorSplitter->currentEditor()) ed->paste(); }
void MainWindow::selectAll() { if (auto* ed = m_editorSplitter->currentEditor()) ed->selectAll(); }
void MainWindow::showFind() { if (auto* ed = m_editorSplitter->currentEditor()) ed->showFindBar(false); }
void MainWindow::showReplace() { if (auto* ed = m_editorSplitter->currentEditor()) ed->showFindBar(true); }

void MainWindow::showFindInFiles() {
    if (m_project->isOpen()) {
        findInFiles()->setProjectRoot(m_project->rootPath());
        findInFiles()->showDialog();
    } else {
        showFind();
    }
}

void MainWindow::selectNextOccurrence() { if (auto* ed = m_editorSplitter->currentEditor()) ed->selectNextOccurrence(); }
void MainWindow::toggleComment() { if (auto* ed = m_editorSplitter->currentEditor()) ed->toggleComment(); }
void MainWindow::duplicateLine() { if (auto* ed = m_editorSplitter->currentEditor()) ed->duplicateLine(); }
void MainWindow::deleteCurrentLine() { if (auto* ed = m_editorSplitter->currentEditor()) ed->deleteCurrentLine(); }
void MainWindow::moveLineUp() { if (auto* ed = m_editorSplitter->currentEditor()) ed->moveLineUp(); }
void MainWindow::moveLineDown() { if (auto* ed = m_editorSplitter->currentEditor()) ed->moveLineDown(); }

void MainWindow::gotoLine() {
    bool ok = false;
    const int line = QInputDialog::getInt(this, "Go to Line", "Line number:", 1, 1, 10000000, 1, &ok);
    if (ok) if (auto* ed = m_editorSplitter->currentEditor()) ed->gotoLine(line);
}

// -------------------------------------------------------------- view actions

void MainWindow::toggleSidebar() { m_sidebarTabWidget->setVisible(!m_sidebarTabWidget->isVisible()); }

void MainWindow::toggleBottomPanel() {
    m_bottomTabWidget->setVisible(!m_bottomTabWidget->isVisible());
    if (m_bottomTabWidget->isVisible()) onBottomTabChanged(m_bottomTabWidget->currentIndex());
}

void MainWindow::toggleDistractionFree() {
    m_distractionFree = !m_distractionFree;
    if (m_distractionFree) {
        m_savedGeometry = saveGeometry();
        m_sidebarTabWidget->hide();
        m_bottomTabWidget->hide();
        menuBar()->hide();
        m_toolBar->hide();
        showFullScreen();
    } else {
        showNormal();
        restoreGeometry(m_savedGeometry);
        m_sidebarTabWidget->show();
        menuBar()->show();
        m_toolBar->setVisible(core::Config::instance().ui().showToolbar);
    }
}

void MainWindow::toggleMinimap() {
    if (auto* ed = m_editorSplitter->currentEditor()) if (!ed->isLargeFile()) ed->setMinimapEnabled(!ed->isMinimapEnabled());
}
void MainWindow::toggleStickyScroll() {
    if (auto* ed = m_editorSplitter->currentEditor()) ed->setStickyScrollEnabled(!ed->isStickyScrollEnabled());
}
void MainWindow::toggleWordWrap() {
    if (auto* ed = m_editorSplitter->currentEditor()) ed->setWordWrap(!ed->isWordWrap());
}
void MainWindow::splitVertical() { m_editorSplitter->split(Qt::Horizontal); }
void MainWindow::splitHorizontal() { m_editorSplitter->split(Qt::Vertical); }
void MainWindow::zoomIn() { if (auto* ed = m_editorSplitter->currentEditor()) ed->zoomInStep(); }
void MainWindow::zoomOut() { if (auto* ed = m_editorSplitter->currentEditor()) ed->zoomOutStep(); }
void MainWindow::resetZoom() { if (auto* ed = m_editorSplitter->currentEditor()) ed->resetZoom(); }

// ------------------------------------------------------------- build & debug

void MainWindow::buildProject() {
    if (core::Config::instance().build().autoSaveBeforeBuild) saveAllFiles();
    const QString root = m_project->isOpen() ? m_project->rootPath() : QString();
    auto* ed = m_editorSplitter->currentEditor();
    const QString activeFile = ed && ed->documentModel() ? ed->documentModel()->filePath() : QString();
    m_diagnosticsPanel->clearDiagnostics();
    m_buildSystem->startBuild(root, activeFile, build::BuildTarget::Build);
}

void MainWindow::runProject() {
    if (core::Config::instance().build().autoSaveBeforeBuild) saveAllFiles();
    const QString root = m_project->isOpen() ? m_project->rootPath() : QString();
    auto* ed = m_editorSplitter->currentEditor();
    const QString activeFile = ed && ed->documentModel() ? ed->documentModel()->filePath() : QString();
    m_buildSystem->startBuild(root, activeFile, build::BuildTarget::Run);
}

void MainWindow::cleanProject() {
    const QString root = m_project->isOpen() ? m_project->rootPath() : QString();
    m_buildSystem->startBuild(root, QString(), build::BuildTarget::Clean);
}

void MainWindow::rebuildProject() {
    // Clean and build used to be started back-to-back, so the build was
    // rejected ("already running"). Now the build starts when the clean ends.
    if (m_buildSystem->isRunning()) return;
    m_pendingRebuild = true;
    cleanProject();
}

void MainWindow::stopBuild() {
    m_pendingRebuild = false;
    m_buildSystem->stopBuild();
}

void MainWindow::startDebugging() {
    QString target;
    if (m_project->isOpen()) target = m_project->rootPath() + "/build/" + m_project->projectName();
    if (!QFileInfo::exists(target)) target = QFileDialog::getOpenFileName(this, "Select Executable to Debug");
    if (target.isEmpty()) return;

    m_bottomTabWidget->setCurrentWidget(ensureDebugger());
    m_bottomTabWidget->show();
    m_debugger->startDebugging(target, m_project->isOpen() ? m_project->rootPath() : QString());
}

void MainWindow::stopDebugging() { m_debugger->stopDebugging(); }

// ------------------------------------------------------------- palettes etc.

void MainWindow::showQuickOpen() { palette()->showQuickOpen(m_project->projectFiles()); }
void MainWindow::showCommandPalette() { palette()->showPalette(); }
void MainWindow::showSettings() { settingsDialog()->show(); settingsDialog()->raise(); }
void MainWindow::showAbout() { aboutDialog()->show(); }

void MainWindow::switchProfile(const QString& profileName) {
    auto& pm = core::ProfileManager::instance();
    pm.setCurrentProfile(profileName);
    const auto& prof = pm.currentProfile();

    switchTheme(prof.theme);
    const auto& ed = core::Config::instance().editor();
    m_editorSplitter->updateFontSettings(ed.fontFamily, ed.fontSize);

    if (m_project->isOpen()) startLspIfConfigured(m_project->rootPath());
    m_statusBar->showMessage("Switched to Profile: " + profileName, 3000);
}

void MainWindow::switchTheme(const QString& themeName) {
    core::Config::instance().setActiveTheme(themeName);
    applyTheme(themeName);
}

void MainWindow::exportProfile() {
    const QString path = QFileDialog::getSaveFileName(this, "Export Profile", "profile.json", "JSON Files (*.json)");
    if (!path.isEmpty())
        core::ProfileManager::instance().exportProfile(core::ProfileManager::instance().currentProfile().name, path);
}

void MainWindow::importProfile() {
    const QString path = QFileDialog::getOpenFileName(this, "Import Profile", "", "JSON Files (*.json)");
    if (path.isEmpty()) return;
    if (core::ProfileManager::instance().importProfile(path)) {
        rebuildProfileMenu();
        m_statusBar->showMessage("Profile imported successfully.", 3000);
    }
}

// ----------------------------------------------------------- event handlers

void MainWindow::onActiveEditorChanged(editor::CodeEditor* editor) {
    if (!editor) return;

    // Wire each editor exactly once. This used to connect on EVERY tab switch,
    // so handlers (cursor updates, LSP requests, breakpoints) multiplied.
    if (editor->property("lf_wired").toBool()) return;
    editor->setProperty("lf_wired", true);

    connect(editor, &editor::CodeEditor::cursorPositionChangedInfo, this, &MainWindow::onCursorPositionChanged);
    connect(editor, &editor::CodeEditor::definitionRequested, this, &MainWindow::onLspDefinitionRequested);
    connect(editor, &editor::CodeEditor::referencesRequested, this, &MainWindow::onLspReferencesRequested);
    connect(editor, &editor::CodeEditor::hoverRequested, this, &MainWindow::onLspHoverRequested);
    connect(editor, &editor::CodeEditor::completionRequested, this, &MainWindow::onLspCompletionRequested);
    connect(editor, &editor::CodeEditor::formatRequested, this, &MainWindow::onLspFormatRequested);
    connect(editor, &editor::CodeEditor::renameRequested, this, &MainWindow::onLspRenameRequested);
    connect(editor, &editor::CodeEditor::breakpointToggled, this, [this, editor](int line, bool active) {
        if (!editor->documentModel()) return;
        if (active) m_debugger->addBreakpoint(editor->documentModel()->filePath(), line);
        else m_debugger->removeBreakpoint(editor->documentModel()->filePath(), line);
    });
}

void MainWindow::onCursorPositionChanged(int line, int col, int, int) { m_statusBar->setCursorPosition(line, col); }

void MainWindow::onBuildStarted(const QString& target) {
    m_bottomTabWidget->show();
    auto* term = ensureTerminal();
    m_bottomTabWidget->setCurrentWidget(term);
    term->appendBuildOutput("=== Build Started: " + target + " ===\n");
}

void MainWindow::onBuildFinished(bool success, int exitCode) {
    ensureTerminal()->appendBuildOutput(QString("\n=== Build %1 (exit code: %2) ===\n")
                                            .arg(success ? "SUCCESSFUL" : "FAILED").arg(exitCode));
    m_diagnosticsPanel->updateTabTitle(m_bottomTabWidget, 1);

    if (m_pendingRebuild) {
        m_pendingRebuild = false;
        if (success) QTimer::singleShot(0, this, &MainWindow::buildProject);
    }
}

void MainWindow::onBuildOutput(const QString& text, bool isError) { ensureTerminal()->appendBuildOutput(text, isError); }

void MainWindow::onDiagnosticFound(DiagnosticSeverity severity, const QString& message, const QString& file, int line, int column) {
    m_diagnosticsPanel->addDiagnostic(severity, message, file, line, column);
    m_diagnosticsPanel->updateTabTitle(m_bottomTabWidget, 1);
}

void MainWindow::onJumpToSource(const QString& file, int line, int column) {
    openFile(file);
    if (auto* ed = m_editorSplitter->currentEditor()) ed->gotoLine(line, column);
}

void MainWindow::onSettingsApplied() {
    // Theme, UI density, font, minimap etc. now take effect immediately
    // (previously only the editor font did; the rest needed a restart).
    core::Config::instance().save();
    applyKeybindings();
    applyUiSettings();
    m_editorSplitter->refreshEditorSettings();
}

// ----------------------------------------------------------------------- LSP

void MainWindow::onLspDefinitionRequested(int line, int col) {
    auto* ed = m_editorSplitter->currentEditor();
    if (!ed || !ed->documentModel()) return;
    m_lspClient->requestDefinition(ed->documentModel()->filePath(), line, col, [this](const std::vector<lsp::Location>& locs) {
        if (!locs.empty()) {
            const auto& loc = locs.front();
            onJumpToSource(loc.localPath(), loc.range.start.line + 1, loc.range.start.character + 1);
        }
    });
}

void MainWindow::onLspReferencesRequested(int line, int col) {
    auto* ed = m_editorSplitter->currentEditor();
    if (!ed || !ed->documentModel()) return;
    m_lspClient->requestReferences(ed->documentModel()->filePath(), line, col, [this](const std::vector<lsp::Location>& locs) {
        m_diagnosticsPanel->clearDiagnostics();
        for (const auto& loc : locs)
            m_diagnosticsPanel->addDiagnostic(DiagnosticSeverity::Information, "Reference", loc.localPath(),
                                              loc.range.start.line + 1, loc.range.start.character + 1);
        m_bottomTabWidget->show();
        m_bottomTabWidget->setCurrentWidget(m_diagnosticsPanel);
    });
}

void MainWindow::onLspHoverRequested(int line, int col, const QPoint& globalPos) {
    auto* ed = m_editorSplitter->currentEditor();
    if (!ed || !ed->documentModel()) return;
    m_lspClient->requestHover(ed->documentModel()->filePath(), line, col, [globalPos](const lsp::HoverInfo& hover) {
        if (hover.valid && !hover.contents.isEmpty()) QToolTip::showText(globalPos, hover.contents);
    });
}

void MainWindow::onLspCompletionRequested(int line, int col, const QString&) {
    auto* ed = m_editorSplitter->currentEditor();
    if (!ed || !ed->documentModel()) return;
    QPointer<editor::CodeEditor> guard(ed);
    m_lspClient->requestCompletion(ed->documentModel()->filePath(), line, col, [this, guard](const std::vector<lsp::CompletionItem>& items) {
        if (items.empty() || !guard) return;
        completionPopup()->showCompletions(items, guard->mapToGlobal(guard->cursorRect().bottomLeft()));
    });
}

void MainWindow::onLspFormatRequested() {
    auto* ed = m_editorSplitter->currentEditor();
    if (!ed || !ed->documentModel()) return;
    QPointer<editor::CodeEditor> guard(ed);
    m_lspClient->requestFormatting(ed->documentModel()->filePath(), [guard](const std::vector<lsp::TextEdit>& edits) {
        if (guard) applyTextEdits(guard, edits);
    });
}

void MainWindow::onLspRenameRequested(int line, int col) {
    auto* ed = m_editorSplitter->currentEditor();
    if (!ed || !ed->documentModel()) return;

    bool ok = false;
    const QString newName = QInputDialog::getText(this, "Rename Symbol", "New symbol name:", QLineEdit::Normal, "", &ok);
    if (!ok || newName.trimmed().isEmpty()) return;

    QPointer<editor::CodeEditor> guard(ed);
    m_lspClient->requestRename(ed->documentModel()->filePath(), line, col, newName.trimmed(),
                               [guard](const std::vector<lsp::TextEdit>& edits) {
        if (guard) applyTextEdits(guard, edits);
    });
}

} // namespace liteforge::ui
