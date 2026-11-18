#include "liteforge/ui/MainWindow.hpp"
#include "liteforge/core/FileUtils.hpp"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QCloseEvent>
#include <QApplication>

namespace liteforge::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_project(std::make_unique<core::Project>(this)),
      m_buildSystem(std::make_unique<build::BuildSystem>(this)),
      m_lspClient(std::make_unique<lsp::LSPClient>(this)) {
    
    setWindowTitle("LiteForge IDE");
    resize(1200, 800);

    setupUi();
    setupMenus();
    setupToolBar();
    setupShortcuts();
    setupConnections();
    setupCommandPaletteActions();

    loadSession();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    // Main horizontal splitter (Sidebar | Workspace)
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_mainSplitter);

    // Left Sidebar: File Explorer
    m_fileExplorer = new FileExplorer(this);
    m_fileExplorer->setMinimumWidth(180);
    m_mainSplitter->addWidget(m_fileExplorer);

    // Right side: Vertical splitter (TabManager | BottomPanel)
    m_rightSplitter = new QSplitter(Qt::Vertical, m_mainSplitter);
    m_mainSplitter->addWidget(m_rightSplitter);

    // Center: Tab Manager (Code Editors)
    m_tabManager = new editor::TabManager(m_rightSplitter);
    m_rightSplitter->addWidget(m_tabManager);

    // Bottom Tab Widget (Terminal, Problems/Diagnostics)
    m_bottomTabWidget = new QTabWidget(m_rightSplitter);
    m_bottomTabWidget->setDocumentMode(true);
    m_bottomTabWidget->setMinimumHeight(120);

    m_terminalPanel = new TerminalPanel(m_bottomTabWidget);
    m_diagnosticsPanel = new DiagnosticsPanel(m_bottomTabWidget);

    m_bottomTabWidget->addTab(m_terminalPanel, "Terminal & Output");
    m_bottomTabWidget->addTab(m_diagnosticsPanel, "Problems (0)");

    m_rightSplitter->addWidget(m_bottomTabWidget);

    // Splitter sizes ratio
    m_mainSplitter->setSizes(QList<int>() << 240 << 960);
    m_rightSplitter->setSizes(QList<int>() << 560 << 240);

    // Status bar
    m_statusBar = new StatusBar(this);
    setStatusBar(m_statusBar);

    // Dialogs
    m_commandPalette = new CommandPalette(this);
    m_findInFilesDialog = new FindInFilesDialog(this);
    m_settingsDialog = new SettingsDialog(this);
    m_aboutDialog = new AboutDialog(this);
}

void MainWindow::setupMenus() {
    // --- File Menu ---
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("New File", QKeySequence::New, this, &MainWindow::newFile);
    fileMenu->addAction("Open File...", QKeySequence::Open, this, &MainWindow::openFileDialog);
    fileMenu->addAction("Open Folder...", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), this, &MainWindow::openFolderDialog);
    fileMenu->addSeparator();
    fileMenu->addAction("Save", QKeySequence::Save, this, &MainWindow::saveFile);
    fileMenu->addAction("Save As...", QKeySequence::SaveAs, this, &MainWindow::saveFileAs);
    fileMenu->addAction("Save All", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), this, &MainWindow::saveAllFiles);
    fileMenu->addSeparator();
    fileMenu->addAction("Close Tab", QKeySequence::Close, this, &MainWindow::closeActiveTab);
    fileMenu->addAction("Exit", QKeySequence::Quit, this, &QWidget::close);

    // --- Edit Menu ---
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
    editMenu->addAction("Toggle Line Comment", QKeySequence(Qt::CTRL | Qt::Key_Slash), this, &MainWindow::toggleComment);
    editMenu->addAction("Duplicate Line", QKeySequence(Qt::CTRL | Qt::Key_D), this, &MainWindow::duplicateLine);
    editMenu->addAction("Delete Line", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K), this, &MainWindow::deleteCurrentLine);
    editMenu->addAction("Move Line Up", QKeySequence(Qt::ALT | Qt::Key_Up), this, &MainWindow::moveLineUp);
    editMenu->addAction("Move Line Down", QKeySequence(Qt::ALT | Qt::Key_Down), this, &MainWindow::moveLineDown);
    editMenu->addAction("Go to Line...", QKeySequence(Qt::CTRL | Qt::Key_G), this, &MainWindow::gotoLine);

    // --- View Menu ---
    QMenu* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Command Palette...", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P), this, &MainWindow::showCommandPalette);
    viewMenu->addAction("Quick Open File...", QKeySequence(Qt::CTRL | Qt::Key_P), this, &MainWindow::showQuickOpen);
    viewMenu->addSeparator();
    viewMenu->addAction("Toggle Sidebar", QKeySequence(Qt::CTRL | Qt::Key_B), this, &MainWindow::toggleSidebar);
    viewMenu->addAction("Toggle Terminal Panel", QKeySequence(Qt::CTRL | Qt::Key_AsciiTilde), this, &MainWindow::toggleBottomPanel);
    viewMenu->addAction("Distraction-Free Mode", QKeySequence(Qt::Key_F11), this, &MainWindow::toggleDistractionFree);
    viewMenu->addSeparator();
    viewMenu->addAction("Zoom In", QKeySequence::ZoomIn, this, &MainWindow::zoomIn);
    viewMenu->addAction("Zoom Out", QKeySequence::ZoomOut, this, &MainWindow::zoomOut);
    viewMenu->addAction("Reset Zoom", QKeySequence(Qt::CTRL | Qt::Key_0), this, &MainWindow::resetZoom);

    // --- Build Menu ---
    QMenu* buildMenu = menuBar()->addMenu("&Build");
    buildMenu->addAction("Build Project", QKeySequence(Qt::CTRL | Qt::Key_F7), this, &MainWindow::buildProject);
    buildMenu->addAction("Run", QKeySequence(Qt::Key_F5), this, &MainWindow::runProject);
    buildMenu->addAction("Clean", this, &MainWindow::cleanProject);
    buildMenu->addAction("Rebuild", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B), this, &MainWindow::rebuildProject);
    buildMenu->addAction("Stop Build", QKeySequence(Qt::SHIFT | Qt::Key_F5), this, &MainWindow::stopBuild);

    // --- Themes Menu ---
    QMenu* themeMenu = menuBar()->addMenu("&Themes");
    for (const auto& themeName : core::ThemeManager::instance().availableThemes()) {
        themeMenu->addAction(themeName, this, [this, themeName]() {
            switchTheme(themeName);
        });
    }

    // --- Help Menu ---
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("Preferences...", QKeySequence::Preferences, this, &MainWindow::showSettings);
    helpMenu->addSeparator();
    helpMenu->addAction("About LiteForge", this, &MainWindow::showAbout);
}

void MainWindow::setupToolBar() {
    auto* toolBar = addToolBar("Main Toolbar");
    toolBar->setMovable(false);

    toolBar->addAction("New", this, &MainWindow::newFile);
    toolBar->addAction("Open", this, &MainWindow::openFileDialog);
    toolBar->addAction("Save", this, &MainWindow::saveFile);
    toolBar->addSeparator();
    toolBar->addAction("▶ Run (F5)", this, &MainWindow::runProject);
    toolBar->addAction("🔨 Build", this, &MainWindow::buildProject);
    toolBar->addSeparator();
    toolBar->addAction("📁 Files (Ctrl+P)", this, &MainWindow::showQuickOpen);
    toolBar->addAction("⚡ Commands (Ctrl+Shift+P)", this, &MainWindow::showCommandPalette);
    toolBar->addSeparator();
    toolBar->addAction("Terminal", this, &MainWindow::toggleBottomPanel);
}

void MainWindow::setupShortcuts() {
    // Additional shortcuts can be handled here or via QActions
}

void MainWindow::setupConnections() {
    // TabManager connections
    connect(m_tabManager, &editor::TabManager::activeEditorChanged, this, &MainWindow::onActiveEditorChanged);
    connect(m_tabManager, &editor::TabManager::fileOpened, this, [this](const QString& path) {
        core::Config::instance().setActiveFile(path);
    });

    // FileExplorer connection
    connect(m_fileExplorer, &FileExplorer::fileSelected, this, [this](const QString& path) {
        openFile(path);
    });

    // CommandPalette connections
    connect(m_commandPalette, &CommandPalette::fileSelected, this, [this](const QString& path) {
        openFile(path);
    });

    // FindInFiles connection
    connect(m_findInFilesDialog, &FindInFilesDialog::resultSelected, this, [this](const QString& file, int line) {
        openFile(file);
        if (auto* ed = m_tabManager->currentEditor()) {
            ed->gotoLine(line);
        }
    });

    // TerminalPanel error line click
    connect(m_terminalPanel, &TerminalPanel::errorLineClicked, this, [this](const QString& file, int line, int col) {
        onJumpToSource(file, line, col);
    });

    // DiagnosticsPanel item selected
    connect(m_diagnosticsPanel, &DiagnosticsPanel::itemSelected, this, [this](const QString& file, int line, int col) {
        onJumpToSource(file, line, col);
    });

    // BuildSystem connections
    connect(m_buildSystem.get(), &build::BuildSystem::buildStarted, this, &MainWindow::onBuildStarted);
    connect(m_buildSystem.get(), &build::BuildSystem::buildFinished, this, &MainWindow::onBuildFinished);
    connect(m_buildSystem.get(), &build::BuildSystem::outputReceived, this, &MainWindow::onBuildOutput);
    connect(m_buildSystem.get(), &build::BuildSystem::diagnosticFound, this, &MainWindow::onDiagnosticFound);

    // SettingsApplied connection
    connect(m_settingsDialog, &SettingsDialog::settingsApplied, this, &MainWindow::onSettingsApplied);
}

void MainWindow::setupCommandPaletteActions() {
    m_commandPalette->registerCommand("File", "New File", "Ctrl+N", [this]() { newFile(); });
    m_commandPalette->registerCommand("File", "Open File", "Ctrl+O", [this]() { openFileDialog(); });
    m_commandPalette->registerCommand("File", "Open Folder", "Ctrl+Shift+O", [this]() { openFolderDialog(); });
    m_commandPalette->registerCommand("File", "Save File", "Ctrl+S", [this]() { saveFile(); });
    m_commandPalette->registerCommand("File", "Save All", "Ctrl+Shift+S", [this]() { saveAllFiles(); });
    m_commandPalette->registerCommand("File", "Close Active Tab", "Ctrl+W", [this]() { closeActiveTab(); });

    m_commandPalette->registerCommand("Edit", "Find in Project Files", "Ctrl+Shift+F", [this]() { showFindInFiles(); });
    m_commandPalette->registerCommand("Edit", "Toggle Line Comment", "Ctrl+/", [this]() { toggleComment(); });
    m_commandPalette->registerCommand("Edit", "Duplicate Line", "Ctrl+D", [this]() { duplicateLine(); });
    m_commandPalette->registerCommand("Edit", "Delete Line", "Ctrl+Shift+K", [this]() { deleteCurrentLine(); });
    m_commandPalette->registerCommand("Edit", "Go to Line", "Ctrl+G", [this]() { gotoLine(); });

    m_commandPalette->registerCommand("Build", "Run Project", "F5", [this]() { runProject(); });
    m_commandPalette->registerCommand("Build", "Build Project", "Ctrl+F7", [this]() { buildProject(); });
    m_commandPalette->registerCommand("Build", "Clean Project", "", [this]() { cleanProject(); });
    m_commandPalette->registerCommand("Build", "Rebuild Project", "Ctrl+Shift+B", [this]() { rebuildProject(); });

    m_commandPalette->registerCommand("View", "Toggle Sidebar", "Ctrl+B", [this]() { toggleSidebar(); });
    m_commandPalette->registerCommand("View", "Toggle Terminal", "Ctrl+~", [this]() { toggleBottomPanel(); });
    m_commandPalette->registerCommand("View", "Distraction-Free Mode", "F11", [this]() { toggleDistractionFree(); });

    m_commandPalette->registerCommand("Preferences", "Open Settings", "Ctrl+,", [this]() { showSettings(); });
    m_commandPalette->registerCommand("Help", "About LiteForge", "", [this]() { showAbout(); });

    for (const auto& theme : core::ThemeManager::instance().availableThemes()) {
        m_commandPalette->registerCommand("Theme", "Switch Theme: " + theme, "", [this, theme]() {
            switchTheme(theme);
        });
    }
}

void MainWindow::applyTheme(const QString& themeName) {
    core::ThemeManager::instance().applyTheme(themeName);
    qApp->setStyleSheet(core::ThemeManager::instance().generateStyleSheet());
    m_tabManager->applyTheme(core::ThemeManager::instance().currentTheme());
}

void MainWindow::switchTheme(const QString& themeName) {
    core::Config::instance().setActiveTheme(themeName);
    applyTheme(themeName);
}

void MainWindow::loadSession() {
    auto& cfg = core::Config::instance();
    cfg.load();

    applyTheme(cfg.activeTheme());

    // Restore window geometry
    if (!cfg.window().geometry.isEmpty()) {
        restoreGeometry(cfg.window().geometry);
    }
    if (!cfg.window().mainSplitterState.isEmpty()) {
        m_mainSplitter->restoreState(cfg.window().mainSplitterState);
    }
    if (!cfg.window().bottomSplitterState.isEmpty()) {
        m_rightSplitter->restoreState(cfg.window().bottomSplitterState);
    }

    m_fileExplorer->setVisible(cfg.window().sidebarVisible);
    m_bottomTabWidget->setVisible(cfg.window().bottomPanelVisible);

    // Open last project if existed, or current workspace
    QString proj = cfg.lastProjectPath();
    if (proj.isEmpty() || !QDir(proj).exists()) {
        proj = QDir::currentPath();
    }
    openProject(proj);

    // Restore opened files
    for (const QString& file : cfg.openFiles()) {
        if (QFile::exists(file)) {
            m_tabManager->openFile(file);
        }
    }

    if (m_tabManager->count() == 0) {
        newFile();
    }
}

void MainWindow::saveSession() {
    auto& cfg = core::Config::instance();
    cfg.window().geometry = saveGeometry();
    cfg.window().mainSplitterState = m_mainSplitter->saveState();
    cfg.window().bottomSplitterState = m_rightSplitter->saveState();
    cfg.window().sidebarVisible = m_fileExplorer->isVisible();
    cfg.window().bottomPanelVisible = m_bottomTabWidget->isVisible();

    cfg.setOpenFiles(m_tabManager->openFilePaths());
    if (m_project->isOpen()) {
        cfg.setLastProjectPath(m_project->rootPath());
    }

    cfg.save();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!m_tabManager->closeAllFiles()) {
        event->ignore();
        return;
    }
    saveSession();
    event->accept();
}

void MainWindow::openProject(const QString& path) {
    if (m_project->open(path)) {
        m_fileExplorer->setRootPath(path);
        m_findInFilesDialog->setProjectRoot(path);
        m_terminalPanel->setWorkingDirectory(path);
        m_commandPalette->setProjectFiles(m_project->projectFiles());

        m_statusBar->setGitBranch(m_project->gitBranch());
        setWindowTitle(QString("LiteForge IDE - %1 [%2]").arg(m_project->projectName(), path));
        core::Config::instance().setLastProjectPath(path);
        core::Config::instance().addRecentProject(path);
    }
}

void MainWindow::openFile(const QString& path) {
    auto* editor = m_tabManager->openFile(path);
    if (editor) {
        editor->setFocus();
    }
}

void MainWindow::newFile() {
    auto* editor = m_tabManager->newUntitledDocument();
    if (editor) {
        editor->setFocus();
    }
}

void MainWindow::openFileDialog() {
    QString startDir = m_project->isOpen() ? m_project->rootPath() : QDir::homePath();
    QString path = QFileDialog::getOpenFileName(this, "Open File", startDir);
    if (!path.isEmpty()) {
        openFile(path);
    }
}

void MainWindow::openFolderDialog() {
    QString startDir = m_project->isOpen() ? m_project->rootPath() : QDir::homePath();
    QString path = QFileDialog::getExistingDirectory(this, "Open Project Folder", startDir);
    if (!path.isEmpty()) {
        openProject(path);
    }
}

void MainWindow::saveFile() {
    auto* editor = m_tabManager->currentEditor();
    if (!editor) return;

    auto doc = editor->documentModel();
    if (doc && doc->isUntitled()) {
        saveFileAs();
    } else {
        m_tabManager->saveCurrentFile();
    }
}

void MainWindow::saveFileAs() {
    auto* editor = m_tabManager->currentEditor();
    if (!editor) return;

    QString startPath = m_project->isOpen() ? m_project->rootPath() : QDir::homePath();
    QString path = QFileDialog::getSaveFileName(this, "Save As", startPath);
    if (!path.isEmpty()) {
        if (core::FileUtils::writeFileUtf8(path, editor->toPlainText())) {
            editor->documentModel()->setFilePath(path);
            editor->documentModel()->setModified(false);
            m_tabManager->setTabText(m_tabManager->currentIndex(), editor->documentModel()->fileName());
        }
    }
}

void MainWindow::saveAllFiles() {
    m_tabManager->saveAllFiles();
}

void MainWindow::closeActiveTab() {
    m_tabManager->closeActiveTab();
}

void MainWindow::undo() {
    if (auto* ed = m_tabManager->currentEditor()) ed->undo();
}

void MainWindow::redo() {
    if (auto* ed = m_tabManager->currentEditor()) ed->redo();
}

void MainWindow::cut() {
    if (auto* ed = m_tabManager->currentEditor()) ed->cut();
}

void MainWindow::copy() {
    if (auto* ed = m_tabManager->currentEditor()) ed->copy();
}

void MainWindow::paste() {
    if (auto* ed = m_tabManager->currentEditor()) ed->paste();
}

void MainWindow::selectAll() {
    if (auto* ed = m_tabManager->currentEditor()) ed->selectAll();
}

void MainWindow::showFind() {
    if (auto* ed = m_tabManager->currentEditor()) ed->showFindBar(false);
}

void MainWindow::showReplace() {
    if (auto* ed = m_tabManager->currentEditor()) ed->showFindBar(true);
}

void MainWindow::showFindInFiles() {
    QString initial;
    if (auto* ed = m_tabManager->currentEditor()) {
        if (ed->textCursor().hasSelection()) {
            initial = ed->textCursor().selectedText();
        }
    }
    m_findInFilesDialog->showDialog(initial);
}

void MainWindow::toggleComment() {
    if (auto* ed = m_tabManager->currentEditor()) ed->toggleComment();
}

void MainWindow::duplicateLine() {
    if (auto* ed = m_tabManager->currentEditor()) ed->duplicateLine();
}

void MainWindow::deleteCurrentLine() {
    if (auto* ed = m_tabManager->currentEditor()) ed->deleteCurrentLine();
}

void MainWindow::moveLineUp() {
    if (auto* ed = m_tabManager->currentEditor()) ed->moveLineUp();
}

void MainWindow::moveLineDown() {
    if (auto* ed = m_tabManager->currentEditor()) ed->moveLineDown();
}

void MainWindow::gotoLine() {
    auto* ed = m_tabManager->currentEditor();
    if (!ed) return;

    bool ok = false;
    int line = QInputDialog::getInt(this, "Go to Line", "Line number:", 1, 1, ed->blockCount(), 1, &ok);
    if (ok) {
        ed->gotoLine(line);
    }
}

void MainWindow::toggleSidebar() {
    m_fileExplorer->setVisible(!m_fileExplorer->isVisible());
}

void MainWindow::toggleBottomPanel() {
    m_bottomTabWidget->setVisible(!m_bottomTabWidget->isVisible());
}

void MainWindow::toggleDistractionFree() {
    m_distractionFree = !m_distractionFree;
    if (m_distractionFree) {
        m_fileExplorer->hide();
        m_bottomTabWidget->hide();
        menuBar()->hide();
        m_statusBar->hide();
        showFullScreen();
    } else {
        m_fileExplorer->show();
        m_bottomTabWidget->show();
        menuBar()->show();
        m_statusBar->show();
        showNormal();
    }
}

void MainWindow::zoomIn() {
    if (auto* ed = m_tabManager->currentEditor()) ed->zoomInStep();
}

void MainWindow::zoomOut() {
    if (auto* ed = m_tabManager->currentEditor()) ed->zoomOutStep();
}

void MainWindow::resetZoom() {
    if (auto* ed = m_tabManager->currentEditor()) ed->resetZoom();
}

void MainWindow::buildProject() {
    if (core::Config::instance().build().autoSaveBeforeBuild) {
        saveAllFiles();
    }
    m_bottomTabWidget->show();
    m_terminalPanel->switchToBuildTab();
    m_terminalPanel->clearBuildOutput();
    m_diagnosticsPanel->clearDiagnostics();

    QString activeFile;
    if (auto* ed = m_tabManager->currentEditor()) {
        if (ed->documentModel()) {
            activeFile = ed->documentModel()->filePath();
        }
    }

    m_buildSystem->startBuild(m_project->rootPath(), activeFile, build::BuildTarget::Build);
}

void MainWindow::runProject() {
    if (core::Config::instance().build().autoSaveBeforeBuild) {
        saveAllFiles();
    }
    m_bottomTabWidget->show();
    m_terminalPanel->switchToBuildTab();
    m_terminalPanel->clearBuildOutput();
    m_diagnosticsPanel->clearDiagnostics();

    QString activeFile;
    if (auto* ed = m_tabManager->currentEditor()) {
        if (ed->documentModel()) {
            activeFile = ed->documentModel()->filePath();
        }
    }

    m_buildSystem->startBuild(m_project->rootPath(), activeFile, build::BuildTarget::Run);
}

void MainWindow::cleanProject() {
    m_bottomTabWidget->show();
    m_terminalPanel->switchToBuildTab();
    m_buildSystem->startBuild(m_project->rootPath(), QString(), build::BuildTarget::Clean);
}

void MainWindow::rebuildProject() {
    if (core::Config::instance().build().autoSaveBeforeBuild) {
        saveAllFiles();
    }
    m_bottomTabWidget->show();
    m_terminalPanel->switchToBuildTab();
    m_buildSystem->startBuild(m_project->rootPath(), QString(), build::BuildTarget::Rebuild);
}

void MainWindow::stopBuild() {
    m_buildSystem->stopBuild();
}

void MainWindow::showQuickOpen() {
    if (m_project->isOpen()) {
        m_commandPalette->setProjectFiles(m_project->projectFiles());
    }
    m_commandPalette->showPalette(CommandPalette::Mode::Files);
}

void MainWindow::showCommandPalette() {
    m_commandPalette->showPalette(CommandPalette::Mode::Commands);
}

void MainWindow::showSettings() {
    m_settingsDialog->show();
}

void MainWindow::showAbout() {
    m_aboutDialog->show();
}

void MainWindow::onActiveEditorChanged(editor::CodeEditor* editor) {
    if (!editor) {
        m_statusBar->setLanguage("Plain Text");
        m_statusBar->setCursorPosition(1, 1, 0, 0);
        return;
    }

    connect(editor, &editor::CodeEditor::cursorPositionChangedInfo, this, &MainWindow::onCursorPositionChanged, Qt::UniqueConnection);

    if (auto doc = editor->documentModel()) {
        m_statusBar->setLanguage(doc->languageName());
        m_statusBar->setEncoding(doc->encoding());
        m_statusBar->setLineEnding(doc->lineEndingString());
    }

    m_statusBar->setTabInfo(core::Config::instance().editor().tabWidth, core::Config::instance().editor().useSpacesForTabs);
    m_statusBar->updatePerformanceStats();
}

void MainWindow::onCursorPositionChanged(int line, int col, int totalLines, int totalChars) {
    m_statusBar->setCursorPosition(line, col, totalLines, totalChars);
    m_statusBar->updatePerformanceStats();
}

void MainWindow::onBuildStarted(const QString& target) {
    m_statusBar->setBuildStatus("Building (" + target + ")...", false, true);
}

void MainWindow::onBuildFinished(bool success, int exitCode) {
    if (success) {
        m_statusBar->setBuildStatus("Build Succeeded", false, false);
    } else {
        m_statusBar->setBuildStatus(QString("Build Failed (%1)").arg(exitCode), true, false);
    }
    m_bottomTabWidget->setTabText(1, QString("Problems (%1)").arg(m_diagnosticsPanel->errorCount() + m_diagnosticsPanel->warningCount()));
}

void MainWindow::onBuildOutput(const QString& text, bool isError) {
    m_terminalPanel->appendBuildOutput(text, isError);
}

void MainWindow::onDiagnosticFound(DiagnosticSeverity severity, const QString& message, const QString& file, int line, int column) {
    m_diagnosticsPanel->addDiagnostic(severity, message, file, line, column);
    m_bottomTabWidget->setTabText(1, QString("Problems (%1)").arg(m_diagnosticsPanel->errorCount() + m_diagnosticsPanel->warningCount()));
}

void MainWindow::onJumpToSource(const QString& file, int line, int column) {
    QString fullPath = file;
    if (!QDir::isAbsolutePath(fullPath) && m_project->isOpen()) {
        fullPath = m_project->rootPath() + "/" + file;
    }

    if (QFile::exists(fullPath)) {
        openFile(fullPath);
        if (auto* ed = m_tabManager->currentEditor()) {
            ed->gotoLine(line, column);
        }
    }
}

void MainWindow::onSettingsApplied() {
    const auto& cfg = core::Config::instance().editor();
    m_tabManager->updateFontSettings(cfg.fontFamily, cfg.fontSize);
    applyTheme(core::Config::instance().activeTheme());
}

} // namespace liteforge::ui
