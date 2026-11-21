#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>

namespace liteforge::core {

struct EditorConfig {
    QString fontFamily{"Monospace"};
    int fontSize{13};
    int tabWidth{4};
    bool useSpacesForTabs{true};
    bool wordWrap{false};
    bool showLineNumbers{true};
    bool highlightCurrentLine{true};
    bool autoCloseBrackets{true};
    bool showWhitespace{false};
    bool trimTrailingWhitespaceOnSave{true};
    bool insertFinalNewline{true};
    int rulerColumn{80};
    bool showRuler{false};
    bool minimap{false};
    bool stickyScroll{true};
    // Files larger than this (KiB) open without syntax highlighting / minimap
    // to keep RAM and CPU flat. 0 disables the guard.
    int largeFileKiB{2048};
    // Auto-save: 0 = off, otherwise delay in milliseconds after last edit.
    int autoSaveDelayMs{0};
    bool autoSaveOnFocusLoss{false};
};

struct UiConfig {
    QString fontFamily;            // empty = system default
    int fontSize{0};               // points, 0 = system default
    QString density{"normal"};     // compact | normal | comfortable
    int cornerRadius{4};           // px, 0 = square
    QString accentOverride;        // "#rrggbb" or empty = theme accent
    bool showToolbar{true};
    bool showStatusBar{true};
};

struct GeneralConfig {
    bool restoreSession{true};
    bool lspEnabled{true};
};

struct WindowConfig {
    QByteArray geometry;
    QByteArray state;
    QByteArray mainSplitterState;
    QByteArray bottomSplitterState;
    bool sidebarVisible{true};
    bool bottomPanelVisible{false};
    int activeBottomTab{0};
};

struct BuildConfig {
    QString customBuildCommand;
    QString customRunCommand;
    QString cmakeGenerator{"Unix Makefiles"};
    QString buildType{"Release"};
    bool autoSaveBeforeBuild{true};
};

class Config {
public:
    static Config& instance();

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    void load();
    void save();

    EditorConfig& editor() { return m_editor; }
    const EditorConfig& editor() const { return m_editor; }

    UiConfig& ui() { return m_ui; }
    const UiConfig& ui() const { return m_ui; }

    GeneralConfig& general() { return m_general; }
    const GeneralConfig& general() const { return m_general; }

    WindowConfig& window() { return m_window; }
    const WindowConfig& window() const { return m_window; }

    BuildConfig& build() { return m_build; }
    const BuildConfig& build() const { return m_build; }

    QString lastProjectPath() const { return m_lastProjectPath; }
    void setLastProjectPath(const QString& path) { m_lastProjectPath = path; }

    QStringList recentProjects() const { return m_recentProjects; }
    void addRecentProject(const QString& path);

    QStringList openFiles() const { return m_openFiles; }
    void setOpenFiles(const QStringList& files) { m_openFiles = files; }

    QString activeFile() const { return m_activeFile; }
    void setActiveFile(const QString& file) { m_activeFile = file; }

    QString activeTheme() const { return m_activeTheme; }
    void setActiveTheme(const QString& theme) { m_activeTheme = theme; }

    // ~/.config/LiteForge (or platform equivalent). Created on demand.
    QString configDir() const;
    QString configFilePath() const;
    QString themesDir() const;
    QString keybindingsFilePath() const;

private:
    Config() = default;

    EditorConfig m_editor;
    UiConfig m_ui;
    GeneralConfig m_general;
    WindowConfig m_window;
    BuildConfig m_build;

    QString m_lastProjectPath;
    QStringList m_recentProjects;
    QStringList m_openFiles;
    QString m_activeFile;
    QString m_activeTheme{"LiteForge Obsidian"};
};

} // namespace liteforge::core
