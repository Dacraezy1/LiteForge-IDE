#pragma once

#include "liteforge/core/Config.hpp"
#include "liteforge/core/Project.hpp"
#include "liteforge/core/Theme.hpp"
#include "liteforge/core/ProfileManager.hpp"
#include "liteforge/editor/EditorSplitter.hpp"
#include "liteforge/git/GitService.hpp"
#include "liteforge/ui/GitPanel.hpp"
#include "liteforge/ui/FileExplorer.hpp"
#include "liteforge/ui/TerminalPanel.hpp"
#include "liteforge/ui/DiagnosticsPanel.hpp"
#include "liteforge/ui/DebuggerPanel.hpp"
#include "liteforge/ui/WelcomeScreen.hpp"
#include "liteforge/ui/CompletionPopup.hpp"
#include "liteforge/ui/StatusBar.hpp"
#include "liteforge/ui/CommandPalette.hpp"
#include "liteforge/ui/FindInFilesDialog.hpp"
#include "liteforge/ui/SettingsDialog.hpp"
#include "liteforge/ui/AboutDialog.hpp"
#include "liteforge/build/BuildSystem.hpp"
#include "liteforge/build/Debugger.hpp"
#include "liteforge/lsp/LSPClient.hpp"

#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <QStackedWidget>
#include <QPointer>
#include <QHash>
#include <QTimer>
#include <vector>
class QMenu;
class QToolBar;
#include <memory>

namespace liteforge::ui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void openProject(const QString& path);
    void openFile(const QString& path);
    void openFilePreview(const QString& path);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // File actions
    void newFile();
    void newProjectFromTemplate();
    void openFileDialog();
    void openFolderDialog();
    void saveFile();
    void saveFileAs();
    void saveAllFiles();
    void closeActiveTab();

    // Edit actions
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void showFind();
    void showReplace();
    void showFindInFiles();
    void toggleComment();
    void duplicateLine();
    void deleteCurrentLine();
    void moveLineUp();
    void moveLineDown();
    void gotoLine();
    void selectNextOccurrence();

    // View actions
    void toggleSidebar();
    void toggleBottomPanel();
    void toggleDistractionFree();
    void toggleMinimap();
    void toggleStickyScroll();
    void toggleWordWrap();
    void splitVertical();
    void splitHorizontal();
    void zoomIn();
    void zoomOut();
    void resetZoom();

    // Build & Debug actions
    void buildProject();
    void runProject();
    void cleanProject();
    void rebuildProject();
    void stopBuild();
    void startDebugging();
    void stopDebugging();

    // Palettes & Dialogs
    void showQuickOpen();
    void showCommandPalette();
    void showSettings();
    void showAbout();

    // Profiles & Themes
    void switchProfile(const QString& profileName);
    void switchTheme(const QString& themeName);
    void exportProfile();
    void importProfile();

    // Component event handlers
    void onActiveEditorChanged(editor::CodeEditor* editor);
    void onCursorPositionChanged(int line, int col, int totalLines, int totalChars);
    void onBuildStarted(const QString& target);
    void onBuildFinished(bool success, int exitCode);
    void onBuildOutput(const QString& text, bool isError);
    void onDiagnosticFound(DiagnosticSeverity severity, const QString& message, const QString& file, int line, int column);
    void onJumpToSource(const QString& file, int line, int column);
    void onSettingsApplied();

    // Customisation
    void reloadConfig();
    void openConfigFile();
    void openKeybindingsFile();
    void exportThemeTemplate();
    void openThemesFolder();
    void toggleToolbar();

    // LSP event handlers
    void onLspDefinitionRequested(int line, int col);
    void onLspReferencesRequested(int line, int col);
    void onLspHoverRequested(int line, int col, const QPoint& globalPos);
    void onLspCompletionRequested(int line, int col, const QString& prefix);
    void onLspFormatRequested();
    void onLspRenameRequested(int line, int col);

private:
    void setupUi();
    void setupMenus();
    void setupToolBar();
    void setupShortcuts();
    void setupConnections();
    void applyTheme(const QString& themeName);
    void applyUiSettings();
    void applyKeybindings();
    void rebuildThemeMenu();
    void rebuildProfileMenu();
    void loadSession();
    void restoreOpenFiles();
    void saveSession();
    void updateCenterView();
    void setupAutoSave();
    void startLspIfConfigured(const QString& rootPath);
    void queueLspChange(editor::CodeEditor* editor);
    void flushLspChanges();
    void announceOpenDocumentsToLsp();
    void openConfigTextFile(const QString& path, const QString& templateContent);
    QString keyFor(const QString& actionId) const;

    // Lazily created UI: nothing below exists (or costs RAM / startup time)
    // until the first time the user actually needs it.
    GitPanel* ensureGitPanel();
    TerminalPanel* ensureTerminal();
    DebuggerPanel* ensureDebugger();
    CommandPalette* palette();
    FindInFilesDialog* findInFiles();
    SettingsDialog* settingsDialog();
    AboutDialog* aboutDialog();
    CompletionPopup* completionPopup();
    void onSidebarTabChanged(int index);
    void onBottomTabChanged(int index);

    // Core subsystems
    std::unique_ptr<core::Project> m_project;
    std::unique_ptr<git::GitService> m_gitService;
    std::unique_ptr<build::BuildSystem> m_buildSystem;
    std::unique_ptr<build::Debugger> m_debugger;
    std::unique_ptr<lsp::LSPClient> m_lspClient;

    // UI Structure
    QSplitter* m_mainSplitter{nullptr};
    QTabWidget* m_sidebarTabWidget{nullptr};
    FileExplorer* m_fileExplorer{nullptr};
    GitPanel* m_gitPanel{nullptr};
    QWidget* m_gitPlaceholder{nullptr};
    QWidget* m_terminalPlaceholder{nullptr};
    QWidget* m_debuggerPlaceholder{nullptr};

    QSplitter* m_rightSplitter{nullptr};
    QStackedWidget* m_centerStack{nullptr};
    editor::EditorSplitter* m_editorSplitter{nullptr};
    WelcomeScreen* m_welcomeScreen{nullptr};

    QTabWidget* m_bottomTabWidget{nullptr};
    TerminalPanel* m_terminalPanel{nullptr};
    DiagnosticsPanel* m_diagnosticsPanel{nullptr};
    DebuggerPanel* m_debuggerPanel{nullptr};
    StatusBar* m_statusBar{nullptr};

    // Dialogs & Popups
    CompletionPopup* m_completionPopup{nullptr};
    CommandPalette* m_commandPalette{nullptr};
    FindInFilesDialog* m_findInFilesDialog{nullptr};
    SettingsDialog* m_settingsDialog{nullptr};
    AboutDialog* m_aboutDialog{nullptr};

    QMenu* m_themeMenu{nullptr};
    QMenu* m_profileMenu{nullptr};
    QToolBar* m_toolBar{nullptr};

    QTimer* m_autoSaveTimer{nullptr};
    QTimer* m_lspSyncTimer{nullptr};
    std::vector<QPointer<editor::CodeEditor>> m_lspPending;
    QHash<QString, int> m_lspVersions;
    bool m_pendingRebuild{false};

    bool m_distractionFree{false};
    QByteArray m_savedGeometry;
};

} // namespace liteforge::ui
