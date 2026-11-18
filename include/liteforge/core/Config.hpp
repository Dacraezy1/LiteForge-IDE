#pragma once

#include <QString>
#include <QStringList>
#include <QFont>
#include <QColor>
#include <QByteArray>
#include <memory>

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

    // Configuration categories
    EditorConfig& editor() { return m_editor; }
    const EditorConfig& editor() const { return m_editor; }

    WindowConfig& window() { return m_window; }
    const WindowConfig& window() const { return m_window; }

    BuildConfig& build() { return m_build; }
    const BuildConfig& build() const { return m_build; }

    // Session state
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

    QString configFilePath() const;

private:
    Config() = default;

    EditorConfig m_editor;
    WindowConfig m_window;
    BuildConfig m_build;

    QString m_lastProjectPath;
    QStringList m_recentProjects;
    QStringList m_openFiles;
    QString m_activeFile;
    QString m_activeTheme{"LiteForge Obsidian"};
};

} // namespace liteforge::core
