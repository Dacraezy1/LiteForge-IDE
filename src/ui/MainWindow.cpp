#include "liteforge/ui/MainWindow.hpp"
#include "liteforge/core/FileUtils.hpp"
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
#include <QToolTip>

namespace liteforge::ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_project(std::make_unique<core::Project>(this)),
      m_gitService(std::make_unique<git::GitService>(this)),
      m_buildSystem(std::make_unique<build::BuildSystem>(this)),
      m_debugger(std::make_unique<build::Debugger>(this)),
      m_lspClient(std::make_unique<lsp::LSPClient>(this)) {

    setWindowTitle("LiteForge IDE");
    resize(1260, 840);

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

    // Left Sidebar: TabWidget (Files | Git)
    m_sidebarTabWidget = new QTabWidget(m_mainSplitter);
    m_sidebarTabWidget->setDocumentMode(true);
    m_sidebarTabWidget->setMinimumWidth(200);

    m_fileExplorer = new FileExplorer(m_sidebarTabWidget);
    m_gitPanel = new GitPanel(m_gitService.get(), m_sidebarTabWidget);

    m_sidebarTabWidget->addTab(m_fileExplorer, "Files");
    m_sidebarTabWidget->addTab(m_gitPanel, "Source Control");
    m_mainSplitter->addWidget(m_sidebarTabWidget);

    // Right side: Vertical splitter (Center Editor/Welcome | Bottom Panel)
    m_rightSplitter = new QSplitter(Qt::Vertical, m_mainSplitter);
    m_mainSplitter->addWidget(m_rightSplitter);

    // Center Stack (Welcome Screen [0] | Editor Splitter [1])
    m_centerStack = new QStackedWidget(m_rightSplitter);
    m_welcomeScreen = new WelcomeScreen(m_centerStack);
    m_editorSplitter = new editor::EditorSplitter(m_centerStack);

    m_centerStack->addWidget(m_welcomeScreen);
    m_centerStack->addWidget(m_editorSplitter);
    m_centerStack->setCurrentIndex(0); // Show WelcomeScreen by default

    m_rightSplitter->addWidget(m_centerStack);

    // Bottom Tab Widget (Terminal, Problems, Debugger)
    m_bottomTabWidget = new QTabWidget(m_rightSplitter);
    m_bottomTabWidget->setDocumentMode(true);
    m_bottomTabWidget->setMinimumHeight(140);

    m_terminalPanel = new TerminalPanel(m_bottomTabWidget);
    m_diagnosticsPanel = new DiagnosticsPanel(m_bottomTabWidget);
    m_debuggerPanel = new DebuggerPanel(m_debugger.get(), m_bottomTabWidget);

    m_bottomTabWidget->addTab(m_terminalPanel, "Terminal & Output");
    m_bottomTabWidget->addTab(m_diagnosticsPanel, "Problems (0)");
    m_bottomTabWidget->addTab(m_debuggerPanel, "Debugger");

    m_rightSplitter->addWidget(m_bottomTabWidget);

    // Splitter sizes ratio
    m_mainSplitter->setSizes(QList<int>() << 250 << 1010);
    m_rightSplitter->setSizes(QList<int>() << 620 << 220);

    // Status bar
    m_statusBar = new StatusBar(this);
    setStatusBar(m_statusBar);

    // Dialogs & Popups
    m_completionPopup = new CompletionPopup(this);
    m_commandPalette = new CommandPalette(this);
    m_findInFilesDialog = new FindInFilesDialog(this);
    m_settingsDialog = new SettingsDialog(this);
    m_aboutDialog = new AboutDialog(this);

    // Update Welcome screen recent projects list
    m_welcomeScreen->updateRecentProjects(core::Config::instance().recentProjects());
}

void MainWindow::setupMenus() {
    // --- File Menu ---
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
    editMenu->addAction("Select Next Occurrence", QKeySequence(Qt::CTRL | Qt::Key_D), this, &MainWindow::selectNextOccurrence);
    editMenu->addAction("Toggle Comment", QKeySequence(Qt::CTRL | Qt::Key_Slash), this, &MainWindow::toggleComment);
    editMenu->addAction("Duplicate Line", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D), this, &MainWindow::duplicateLine);
    editMenu->addAction("Delete Line", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_K), this, &MainWindow::deleteCurrentLine);
    editMenu->addAction("Move Line Up", QKeySequence(Qt::ALT | Qt::Key_Up), this, &MainWindow::moveLineUp);
    editMenu->addAction("Move Line Down", QKeySequence(Qt::ALT | Qt::Key_Down), this, &MainWindow::moveLineDown);
    editMenu->addAction("Go to Line...", QKeySequence(Qt::CTRL | Qt::Key_G), this, &MainWindow::gotoLine);

    // --- View Menu ---
    QMenu* viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Toggle Sidebar", QKeySequence(Qt::CTRL | Qt::Key_B), this, &MainWindow::toggleSidebar);
    viewMenu->addAction("Toggle Bottom Panel", QKeySequence(Qt::CTRL | Qt::Key_AsciiTilde), this, &MainWindow::toggleBottomPanel);
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

    // --- Build & Debug Menu ---
    QMenu* buildMenu = menuBar()->addMenu("&Build");
    buildMenu->addAction("Build Project", QKeySequence(Qt::CTRL | Qt::Key_F7), this, &MainWindow::buildProject);
    buildMenu->addAction("Run Project", QKeySequence(Qt::Key_F5), this, &MainWindow::runProject);
    buildMenu->addAction("Clean Project", this, &MainWindow::cleanProject);
    buildMenu->addAction("Rebuild Project", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B), this, &MainWindow::rebuildProject);
    buildMenu->addAction("Stop Build", this, &MainWindow::stopBuild);
    buildMenu->addSeparator();
    buildMenu->addAction("Start Debugging (GDB)", QKeySequence(Qt::Key_F9), this, &MainWindow::startDebugging);
    buildMenu->addAction("Stop Debugging", QKeySequence(Qt::SHIFT | Qt::Key_F5), this, &MainWindow::stopDebugging);

    // --- Profiles Menu ---
    QMenu* profileMenu = menuBar()->addMenu("&Profiles");
    const QStringList profiles = core::ProfileManager::instance().availableProfiles();
    for (const QString& profName : profiles) {
        profileMenu->addAction(profName, [this, profName]() {
            switchProfile(profName);
        });
    }
    profileMenu->addSeparator();
    profileMenu->addAction("Export Current Profile...", this, &MainWindow::exportProfile);
    profileMenu->addAction("Import Profile...", this, &MainWindow::importProfile);

    // --- Theme Menu ---
    QMenu* themeMenu = menuBar()->addMenu("&Theme");
    const auto themes = core::ThemeManager::instance().availableThemes();
    for (const auto& tName : themes) {
        themeMenu->addAction(tName, [this, tName]() {
            switchTheme(tName);
        });
    }

    // --- Help Menu ---
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("Command Palette...", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P), this, &MainWindow::showCommandPalette);
    helpMenu->addAction("Quick Open...", QKeySequence(Qt::CTRL | Qt::Key_P), this, &MainWindow::showQuickOpen);
    helpMenu->addSeparator();
    helpMenu->addAction("Settings / Preferences...", QKeySequence(Qt::CTRL | Qt::Key_Comma), this, &MainWindow::showSettings);
    helpMenu->addAction("About LiteForge", this, &MainWindow::showAbout);
}

void MainWindow::setupToolBar() {
    auto* toolbar = addToolBar("Main Controls");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));

    toolbar->addAction("New", this, &MainWindow::newFile);
    toolbar->addAction("Open", this, &MainWindow::openFileDialog);
    toolbar->addAction("Save", this, &MainWindow::saveFile);
    toolbar->addSeparator();
    toolbar->addAction("▶ Run", this, &MainWindow::runProject);
    toolbar->addAction("⚙ Build", this, &MainWindow::buildProject);
    toolbar->addAction("🐞 Debug", this, &MainWindow::startDebugging);
    toolbar->addSeparator();
    toolbar->addAction("⚡ Command Palette", this, &MainWindow::showCommandPalette);
}

void MainWindow::setupShortcuts() {
    // Additional direct action shortcuts
}

void MainWindow::setupConnections() {
    // EditorSplitter connections
    connect(m_editorSplitter, &editor::EditorSplitter::activeEditorChanged, this, &MainWindow::onActiveEditorChanged);
    connect(m_editorSplitter, &editor::EditorSplitter::fileOpened, this, [this](const QString& path) {
        m_statusBar->showMessage("Opened: " + QFileInfo(path).fileName(), 3000);
        updateCenterView();
    });
    connect(m_editorSplitter, &editor::EditorSplitter::allFilesClosed, this, &MainWindow::updateCenterView);

    // WelcomeScreen connections
    connect(m_welcomeScreen, &WelcomeScreen::newFileRequested, this, &MainWindow::newFile);
    connect(m_welcomeScreen, &WelcomeScreen::openFileRequested, this, &MainWindow::openFileDialog);
    connect(m_welcomeScreen, &WelcomeScreen::openProjectRequested, this, &MainWindow::openFolderDialog);
    connect(m_welcomeScreen, &WelcomeScreen::projectSelected, this, &MainWindow::openProject);

    // FileExplorer connections
    connect(m_fileExplorer, &FileExplorer::fileSelected, this, [this](const QString& path) {
        openFilePreview(path);
    });

    // GitService connections
    connect(m_gitService.get(), &git::GitService::branchChanged, this, [this](const QString& branch) {
        m_statusBar->setGitBranch(branch);
    });
    connect(m_gitPanel, &GitPanel::fileSelected, this, [this](const QString& relPath) {
        if (m_project->isOpen()) {
            openFile(m_project->rootPath() + "/" + relPath);
        }
    });

    // BuildSystem connections
    connect(m_buildSystem.get(), &build::BuildSystem::buildStarted, this, &MainWindow::onBuildStarted);
    connect(m_buildSystem.get(), &build::BuildSystem::buildFinished, this, &MainWindow::onBuildFinished);
    connect(m_buildSystem.get(), &build::BuildSystem::outputReceived, this, &MainWindow::onBuildOutput);
    connect(m_buildSystem.get(), &build::BuildSystem::diagnosticFound, this, &MainWindow::onDiagnosticFound);

    // DiagnosticsPanel connections
    connect(m_diagnosticsPanel, &DiagnosticsPanel::itemSelected, this, &MainWindow::onJumpToSource);
    connect(m_debuggerPanel, &DebuggerPanel::jumpToSourceRequested, this, &MainWindow::onJumpToSource);

    // FindInFilesDialog connections
    connect(m_findInFilesDialog, &FindInFilesDialog::resultSelected, this, [this](const QString& file, int line) {
        onJumpToSource(file, line, 1);
    });

    // LSP Diagnostics
    connect(m_lspClient.get(), &lsp::LSPClient::diagnosticsReceived, this, [this](const QString& file, const std::vector<lsp::Diagnostic>& diags) {
        for (const auto& d : diags) {
            DiagnosticSeverity sev = DiagnosticSeverity::Error;
            if (d.severity == 2) sev = DiagnosticSeverity::Warning;
            else if (d.severity >= 3) sev = DiagnosticSeverity::Information;
            m_diagnosticsPanel->addDiagnostic(sev, d.message, file, d.range.start.line + 1, d.range.start.character + 1);
        }
    });

    // CompletionPopup connection
    connect(m_completionPopup, &CompletionPopup::itemSelected, this, [this](const QString& text) {
        auto* ed = m_editorSplitter->currentEditor();
        if (ed) {
            ed->textCursor().insertText(text);
        }
    });

    // SettingsDialog
    connect(m_settingsDialog, &SettingsDialog::settingsApplied, this, &MainWindow::onSettingsApplied);
}

void MainWindow::updateCenterView() {
    if (m_editorSplitter->countAllTabs() == 0) {
        m_centerStack->setCurrentIndex(0); // Show WelcomeScreen
        m_welcomeScreen->updateRecentProjects(core::Config::instance().recentProjects());
    } else {
        m_centerStack->setCurrentIndex(1); // Show EditorSplitter
    }
}

void MainWindow::openProject(const QString& path) {
    if (m_project->open(path)) {
        m_fileExplorer->setRootPath(path);
        m_gitService->setRepositoryPath(path);
        m_statusBar->setGitBranch(m_project->gitBranch());

        core::Config::instance().setLastProjectPath(path);
        core::Config::instance().addRecentProject(path);

        setWindowTitle(QString("LiteForge IDE — %1 [%2]").arg(m_project->projectName()).arg(m_project->buildTypeString()));

        // Start LSP server if configured
        const auto& prof = core::ProfileManager::instance().currentProfile();
        if (!prof.lspExecutable.isEmpty()) {
            m_lspClient->startServer(prof.lspExecutable, prof.lspArgs, path);
        }
    }
}

void MainWindow::openFile(const QString& path) {
    auto* editor = m_editorSplitter->openFile(path);
    if (editor) {
        updateCenterView();
        if (m_lspClient->isInitialized()) {
            m_lspClient->didOpen(path, "cpp", 1, editor->toPlainText());
        }
    }
}

void MainWindow::openFilePreview(const QString& path) {
    auto* editor = m_editorSplitter->openFilePreview(path);
    if (editor) {
        updateCenterView();
    }
}

void MainWindow::newFile() {
    m_editorSplitter->newUntitledDocument();
    updateCenterView();
}

void MainWindow::newProjectFromTemplate() {
    QStringList templates = core::ProjectTemplates::availableTemplates();
    bool ok = false;
    QString chosen = QInputDialog::getItem(this, "New Project", "Choose Project Template:", templates, 0, false, &ok);
    if (!ok || chosen.isEmpty()) return;\n
    QString dir = QFileDialog::getExistingDirectory(this, "Select Project Destination Directory");
    if (dir.isEmpty()) return;

    QString name = QInputDialog::getText(this, "Project Name", "Enter project name:", QLineEdit::Normal, "MyProject", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    core::TemplateType type = core::TemplateType::CppCMake;
    if (chosen.contains("Rust")) type = core::TemplateType::RustCargo;
    else if (chosen.contains("Python")) type = core::TemplateType::PythonApp;
    else if (chosen.contains("Web")) type = core::TemplateType::WebMinimal;

    QString err;
    QString fullPath = dir + "/" + name.trimmed();
    if (core::ProjectTemplates::createProject(type, fullPath, name.trimmed(), &err)) {
        openProject(fullPath);
    } else {
        QMessageBox::warning(this, "Template Error", err);
    }
}

void MainWindow::openFileDialog() {
    QString path = QFileDialog::getOpenFileName(this, "Open File");
    if (!path.isEmpty()) openFile(path);
}

void MainWindow::openFolderDialog() {
    QString path = QFileDialog::getExistingDirectory(this, "Open Folder");
    if (!path.isEmpty()) openProject(path);
}

void MainWindow::saveFile() {
    m_editorSplitter->saveCurrentFile();
}

void MainWindow::saveFileAs() {
    auto* ed = m_editorSplitter->currentEditor();
    if (!ed) return;
    QString path = QFileDialog::getSaveFileName(this, "Save File As");
    if (!path.isEmpty()) {
        core::FileUtils::writeFileUtf8(path, ed->toPlainText());
        openFile(path);
    }
}

void MainWindow::saveAllFiles() {
    m_editorSplitter->saveAllFiles();
}

void MainWindow::closeActiveTab() {
    m_editorSplitter->closeActiveTab();
    updateCenterView();
}

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
        m_findInFilesDialog->setProjectRoot(m_project->rootPath());
        m_findInFilesDialog->showDialog();
    } else {
        showFind();
    }
}

void MainWindow::selectNextOccurrence() {
    if (auto* ed = m_editorSplitter->currentEditor()) ed->selectNextOccurrence();
}

void MainWindow::toggleComment() { if (auto* ed = m_editorSplitter->currentEditor()) ed->toggleComment(); }
void MainWindow::duplicateLine() { if (auto* ed = m_editorSplitter->currentEditor()) ed->duplicateLine(); }
void MainWindow::deleteCurrentLine() { if (auto* ed = m_editorSplitter->currentEditor()) ed->deleteCurrentLine(); }
void MainWindow::moveLineUp() { if (auto* ed = m_editorSplitter->currentEditor()) ed->moveLineUp(); }
void MainWindow::moveLineDown() { if (auto* ed = m_editorSplitter->currentEditor()) ed->moveLineDown(); }

void MainWindow::gotoLine() {
    bool ok = false;
    int line = QInputDialog::getInt(this, "Go to Line", "Line number:", 1, 1, 1000000, 1, &ok);
    if (ok) {
        if (auto* ed = m_editorSplitter->currentEditor()) ed->gotoLine(line);
    }
}

void MainWindow::toggleSidebar() {
    m_sidebarTabWidget->setVisible(!m_sidebarTabWidget->isVisible());
}

void MainWindow::toggleBottomPanel() {
    m_bottomTabWidget->setVisible(!m_bottomTabWidget->isVisible());
}

void MainWindow::toggleDistractionFree() {
    m_distractionFree = !m_distractionFree;
    if (m_distractionFree) {
        m_savedGeometry = saveGeometry();
        m_sidebarTabWidget->hide();
        m_bottomTabWidget->hide();
        menuBar()->hide();
        showFullScreen();
    } else {
        showNormal();
        restoreGeometry(m_savedGeometry);
        m_sidebarTabWidget->show();
        menuBar()->show();
    }
}

void MainWindow::toggleMinimap() {
    if (auto* ed = m_editorSplitter->currentEditor()) {
        ed->setMinimapEnabled(!ed->isMinimapEnabled());
    }
}

void MainWindow::toggleStickyScroll() {
    if (auto* ed = m_editorSplitter->currentEditor()) {
        ed->setStickyScrollEnabled(!ed->isStickyScrollEnabled());
    }
}

void MainWindow::toggleWordWrap() {
    if (auto* ed = m_editorSplitter->currentEditor()) {
        ed->setWordWrap(!ed->isWordWrap());
    }
}

void MainWindow::splitVertical() {
    m_editorSplitter->split(Qt::Horizontal);
}

void MainWindow::splitHorizontal() {
    m_editorSplitter->split(Qt::Vertical);
}

void MainWindow::zoomIn() { if (auto* ed = m_editorSplitter->currentEditor()) ed->zoomInStep(); }
void MainWindow::zoomOut() { if (auto* ed = m_editorSplitter->currentEditor()) ed->zoomOutStep(); }
void MainWindow::resetZoom() { if (auto* ed = m_editorSplitter->currentEditor()) ed->resetZoom(); }

void MainWindow::buildProject() {
    if (core::Config::instance().build().autoSaveBeforeBuild) saveAllFiles();
    QString root = m_project->isOpen() ? m_project->rootPath() : QString();
    QString activeFile = m_editorSplitter->currentEditor() && m_editorSplitter->currentEditor()->documentModel() ?
                         m_editorSplitter->currentEditor()->documentModel()->filePath() : QString();
    m_diagnosticsPanel->clearDiagnostics();
    m_buildSystem->startBuild(root, activeFile, build::BuildTarget::Build);
}

void MainWindow::runProject() {
    if (core::Config::instance().build().autoSaveBeforeBuild) saveAllFiles();
    QString root = m_project->isOpen() ? m_project->rootPath() : QString();
    QString activeFile = m_editorSplitter->currentEditor() && m_editorSplitter->currentEditor()->documentModel() ?
                         m_editorSplitter->currentEditor()->documentModel()->filePath() : QString();
    m_buildSystem->startBuild(root, activeFile, build::BuildTarget::Run);
}

void MainWindow::cleanProject() {
    QString root = m_project->isOpen() ? m_project->rootPath() : QString();
    m_buildSystem->startBuild(root, QString(), build::BuildTarget::Clean);
}

void MainWindow::rebuildProject() {
    cleanProject();
    buildProject();
}

void MainWindow::stopBuild() {
    m_buildSystem->stopBuild();
}

void MainWindow::startDebugging() {
    QString target;
    if (m_project->isOpen()) {
        target = m_project->rootPath() + "/build/" + m_project->projectName();
    }
    if (!QFileInfo::exists(target)) {
        target = QFileDialog::getOpenFileName(this, "Select Executable to Debug");
    }
    if (!target.isEmpty()) {
        m_bottomTabWidget->setCurrentWidget(m_debuggerPanel);
        m_bottomTabWidget->show();
        m_debugger->startDebugging(target, m_project->isOpen() ? m_project->rootPath() : QString());
    }
}

void MainWindow::stopDebugging() {
    m_debugger->stopDebugging();
}

void MainWindow::showQuickOpen() {
    m_commandPalette->showQuickOpen(m_project->projectFiles());
}

void MainWindow::showCommandPalette() {
    m_commandPalette->showPalette();
}

void MainWindow::showSettings() {
    m_settingsDialog->show();
}

void MainWindow::showAbout() {
    m_aboutDialog->show();
}

void MainWindow::switchProfile(const QString& profileName) {
    auto& pm = core::ProfileManager::instance();
    pm.setCurrentProfile(profileName);
    const auto& prof = pm.currentProfile();

    switchTheme(prof.theme);
    m_editorSplitter->updateFontSettings(core::Config::instance().editor().fontFamily, core::Config::instance().editor().fontSize);

    if (m_project->isOpen() && !prof.lspExecutable.isEmpty()) {
        m_lspClient->startServer(prof.lspExecutable, prof.lspArgs, m_project->rootPath());
    }

    m_statusBar->showMessage("Switched to Profile: " + profileName, 3000);
}

void MainWindow::switchTheme(const QString& themeName) {
    applyTheme(themeName);
    core::Config::instance().setActiveTheme(themeName);
}

void MainWindow::exportProfile() {
    QString path = QFileDialog::getSaveFileName(this, "Export Profile", "profile.json", "JSON Files (*.json)");
    if (!path.isEmpty()) {
        core::ProfileManager::instance().exportProfile(core::ProfileManager::instance().currentProfile().name, path);
    }
}

void MainWindow::importProfile() {
    QString path = QFileDialog::getOpenFileName(this, "Import Profile", "", "JSON Files (*.json)");
    if (!path.isEmpty()) {
        if (core::ProfileManager::instance().importProfile(path)) {
            m_statusBar->showMessage("Profile imported successfully.", 3000);
        }
    }
}

void MainWindow::onActiveEditorChanged(editor::CodeEditor* editor) {
    if (!editor) return;

    // Connect cursor position updates
    connect(editor, &editor::CodeEditor::cursorPositionChangedInfo, this, &MainWindow::onCursorPositionChanged);

    // Connect editor LSP signals
    connect(editor, &editor::CodeEditor::definitionRequested, this, &MainWindow::onLspDefinitionRequested);
    connect(editor, &editor::CodeEditor::referencesRequested, this, &MainWindow::onLspReferencesRequested);
    connect(editor, &editor::CodeEditor::hoverRequested, this, &MainWindow::onLspHoverRequested);
    connect(editor, &editor::CodeEditor::completionRequested, this, &MainWindow::onLspCompletionRequested);
    connect(editor, &editor::CodeEditor::formatRequested, this, &MainWindow::onLspFormatRequested);
    connect(editor, &editor::CodeEditor::renameRequested, this, &MainWindow::onLspRenameRequested);

    // Connect Breakpoint changes
    connect(editor, &editor::CodeEditor::breakpointToggled, this, [this, editor](int line, bool active) {\n        if (editor && editor->documentModel()) {
            if (active) m_debugger->addBreakpoint(editor->documentModel()->filePath(), line);
            else m_debugger->removeBreakpoint(editor->documentModel()->filePath(), line);
        }
    });
}

void MainWindow::onCursorPositionChanged(int line, int col, int, int) {
    m_statusBar->setCursorPosition(line, col);
}

void MainWindow::onBuildStarted(const QString& target) {
    m_bottomTabWidget->show();
    m_bottomTabWidget->setCurrentWidget(m_terminalPanel);
    m_terminalPanel->appendBuildOutput("=== Build Started: " + target + " ===\n");
}

void MainWindow::onBuildFinished(bool success, int exitCode) {
    m_terminalPanel->appendBuildOutput(QString("\n=== Build %1 (exit code: %2) ===\n")
                                       .arg(success ? "SUCCESSFUL" : "FAILED")
                                       .arg(exitCode));
    m_diagnosticsPanel->updateTabTitle(m_bottomTabWidget, 1);
}

void MainWindow::onBuildOutput(const QString& text, bool isError) {
    m_terminalPanel->appendBuildOutput(text, isError);
}

void MainWindow::onDiagnosticFound(DiagnosticSeverity severity, const QString& message, const QString& file, int line, int column) {
    m_diagnosticsPanel->addDiagnostic(severity, message, file, line, column);
    m_diagnosticsPanel->updateTabTitle(m_bottomTabWidget, 1);
}

void MainWindow::onJumpToSource(const QString& file, int line, int column) {
    openFile(file);
    if (auto* ed = m_editorSplitter->currentEditor()) {
        ed->gotoLine(line, column);
    }
}

void MainWindow::onSettingsApplied() {
    const auto& edCfg = core::Config::instance().editor();
    m_editorSplitter->updateFontSettings(edCfg.fontFamily, edCfg.fontSize);
}

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
        for (const auto& loc : locs) {
            m_diagnosticsPanel->addDiagnostic(DiagnosticSeverity::Information, "Reference", loc.localPath(), loc.range.start.line + 1, loc.range.start.character + 1);
        }
        m_bottomTabWidget->show();
        m_bottomTabWidget->setCurrentWidget(m_diagnosticsPanel);
    });
}

void MainWindow::onLspHoverRequested(int line, int col, const QPoint& globalPos) {
    auto* ed = m_editorSplitter->currentEditor();
    if (!ed || !ed->documentModel()) return;

    m_lspClient->requestHover(ed->documentModel()->filePath(), line, col, [globalPos](const lsp::HoverInfo& hover) {
        if (hover.valid && !hover.contents.isEmpty()) {
            QToolTip::showText(globalPos, hover.contents);
        }
    });
}

void MainWindow::onLspCompletionRequested(int line, int col, const QString&) {
    auto* ed = m_editorSplitter->currentEditor();
    if (!ed || !ed->documentModel()) return;

    m_lspClient->requestCompletion(ed->documentModel()->filePath(), line, col, [this, ed](const std::vector<lsp::CompletionItem>& items) {
        if (!items.empty()) {
            QPoint pos = ed->mapToGlobal(ed->cursorRect().bottomLeft());
            m_completionPopup->showCompletions(items, pos);
        }
    });
}

void MainWindow::onLspFormatRequested() {
    auto* ed = m_editorSplitter->currentEditor();
    if (!ed || !ed->documentModel()) return;

    m_lspClient->requestFormatting(ed->documentModel()->filePath(), [ed](const std::vector<lsp::TextEdit>& edits) {
        if (!edits.empty()) {
            ed->selectAll();
            ed->insertPlainText(edits.front().newText);
        }
    });
}

void MainWindow::onLspRenameRequested(int line, int col) {
    auto* ed = m_editorSplitter->currentEditor();
    if (!ed || !ed->documentModel()) return;

    bool ok = false;
    QString newName = QInputDialog::getText(this, "Rename Symbol", "New symbol name:", QLineEdit::Normal, "", &ok);
    if (!ok || newName.trimmed().isEmpty()) return;

    m_lspClient->requestRename(ed->documentModel()->filePath(), line, col, newName.trimmed(), [ed](const std::vector<lsp::TextEdit>& edits) {
        // Apply edits
        for (const auto& edit : edits) {
            ed->selectAll();
            ed->insertPlainText(edit.newText);
        }
    });
}

void MainWindow::applyTheme(const QString& themeName) {
    auto& tm = core::ThemeManager::instance();
    tm.applyTheme(themeName);
    setStyleSheet(tm.generateStyleSheet());
    m_editorSplitter->applyTheme(tm.currentTheme());
}

void MainWindow::setupCommandPaletteActions() {
    m_commandPalette->addActionItem("File: New File", [this]() { newFile(); }, "Ctrl+N");
    m_commandPalette->addActionItem("File: Open File...", [this]() { openFileDialog(); }, "Ctrl+O");
    m_commandPalette->addActionItem("File: Open Folder...", [this]() { openFolderDialog(); }, "Ctrl+Shift+O");
    m_commandPalette->addActionItem("File: Save", [this]() { saveFile(); }, "Ctrl+S");
    m_commandPalette->addActionItem("File: Save All", [this]() { saveAllFiles(); }, "Ctrl+Shift+S");
    m_commandPalette->addActionItem("Edit: Select Next Occurrence", [this]() { selectNextOccurrence(); }, "Ctrl+D");
    m_commandPalette->addActionItem("Edit: Toggle Comment", [this]() { toggleComment(); }, "Ctrl+/");
    m_commandPalette->addActionItem("Edit: Duplicate Line", [this]() { duplicateLine(); }, "Ctrl+Shift+D");
    m_commandPalette->addActionItem("View: Toggle Sidebar", [this]() { toggleSidebar(); }, "Ctrl+B");
    m_commandPalette->addActionItem("View: Toggle Bottom Panel", [this]() { toggleBottomPanel(); }, "Ctrl+~");
    m_commandPalette->addActionItem("View: Toggle Minimap", [this]() { toggleMinimap(); });
    m_commandPalette->addActionItem("View: Toggle Sticky Scroll", [this]() { toggleStickyScroll(); });
    m_commandPalette->addActionItem("View: Toggle Word Wrap", [this]() { toggleWordWrap(); });
    m_commandPalette->addActionItem("View: Split Right (Vertical)", [this]() { splitVertical(); }, "Ctrl+\\");
    m_commandPalette->addActionItem("View: Split Down (Horizontal)", [this]() { splitHorizontal(); });
    m_commandPalette->addActionItem("Build: Build Project", [this]() { buildProject(); }, "Ctrl+F7");
    m_commandPalette->addActionItem("Build: Run Project", [this]() { runProject(); }, "F5");
    m_commandPalette->addActionItem("Debug: Start Debugging", [this]() { startDebugging(); }, "F9");
    m_commandPalette->addActionItem("Git: Refresh Status", [this]() { m_gitPanel->refresh(); });
    m_commandPalette->addActionItem("Preferences: Settings", [this]() { showSettings(); }, "Ctrl+,");
}

void MainWindow::loadSession() {
    auto& cfg = core::Config::instance();
    cfg.load();

    applyTheme(cfg.activeTheme());

    if (!cfg.lastProjectPath().isEmpty() && QDir(cfg.lastProjectPath()).exists()) {
        openProject(cfg.lastProjectPath());
    }

    const QStringList files = cfg.openFiles();
    for (const QString& f : files) {
        if (QFile::exists(f)) {
            openFile(f);
        }
    }

    if (!cfg.activeFile().isEmpty() && QFile::exists(cfg.activeFile())) {
        openFile(cfg.activeFile());
    }

    updateCenterView();
}

void MainWindow::saveSession() {
    auto& cfg = core::Config::instance();
    cfg.setOpenFiles(m_editorSplitter->openFilePaths());
    if (auto* ed = m_editorSplitter->currentEditor()) {
        if (ed->documentModel() && !ed->documentModel()->isUntitled()) {
            cfg.setActiveFile(ed->documentModel()->filePath());
        }
    }
    cfg.save();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSession();
    if (!m_editorSplitter->closeAllFiles()) {
        event->ignore();
        return;
    }
    event->accept();
}

} // namespace liteforge::ui
