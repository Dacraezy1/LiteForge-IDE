#pragma once

#include "liteforge/core/Config.hpp"
#include "liteforge/core/Project.hpp"
#include "liteforge/core/Theme.hpp"
#include "liteforge/editor/TabManager.hpp"
#include "liteforge/ui/FileExplorer.hpp"
#include "liteforge/ui/TerminalPanel.hpp"
#include "liteforge/ui/DiagnosticsPanel.hpp"
#include "liteforge/ui/StatusBar.hpp"
#include "liteforge/ui/CommandPalette.hpp"
#include "liteforge/ui/FindInFilesDialog.hpp"
#include "liteforge/ui/SettingsDialog.hpp"
#include "liteforge/ui/AboutDialog.hpp"
#include "liteforge/build/BuildSystem.hpp"
#include "liteforge/lsp/LSPClient.hpp"

#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <memory>

namespace liteforge::ui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void openProject(const QString& path);
    void openFile(const QString& path);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // File actions
    void newFile();
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

    // View actions
    void toggleSidebar();
    void toggleBottomPanel();
    void toggleDistractionFree();
    void zoomIn();
    void zoomOut();
    void resetZoom();

    // Build actions
    void buildProject();
    void runProject();
    void cleanProject();
    void rebuildProject();
    void stopBuild();

    // Palettes & Dialogs
    void showQuickOpen();
    void showCommandPalette();
    void showSettings();
    void showAbout();

    // Theme switching
    void switchTheme(const QString& themeName);

    // Event handlers from components
    void onActiveEditorChanged(editor::CodeEditor* editor);
    void onCursorPositionChanged(int line, int col, int totalLines, int totalChars);
    void onBuildStarted(const QString& target);
    void onBuildFinished(bool success, int exitCode);
    void onBuildOutput(const QString& text, bool isError);
    void onDiagnosticFound(DiagnosticSeverity severity, const QString& message, const QString& file, int line, int column);
    void onJumpToSource(const QString& file, int line, int column);
    void onSettingsApplied();

private:
    void setupUi();
    void setupMenus();
    void setupToolBar();
    void setupShortcuts();
    void setupConnections();
    void setupCommandPaletteActions();
    void applyTheme(const QString& themeName);
    void loadSession();
    void saveSession();

    // Core state
    std::unique_ptr<core::Project> m_project;
    std::unique_ptr<build::BuildSystem> m_buildSystem;
    std::unique_ptr<lsp::LSPClient> m_lspClient;

    // UI Components
    QSplitter* m_mainSplitter{nullptr};
    QSplitter* m_rightSplitter{nullptr};
    FileExplorer* m_fileExplorer{nullptr};
    editor::TabManager* m_tabManager{nullptr};
    QTabWidget* m_bottomTabWidget{nullptr};
    TerminalPanel* m_terminalPanel{nullptr};
    DiagnosticsPanel* m_diagnosticsPanel{nullptr};
    StatusBar* m_statusBar{nullptr};

    CommandPalette* m_commandPalette{nullptr};
    FindInFilesDialog* m_findInFilesDialog{nullptr};
    SettingsDialog* m_settingsDialog{nullptr};
    AboutDialog* m_aboutDialog{nullptr};

    bool m_distractionFree{false};
    QByteArray m_savedGeometry;
};

} // namespace liteforge::ui
