#pragma once

#include <QString>
#include <QColor>
#include <QHash>
#include <vector>

namespace liteforge::core {

struct ThemeColors {
    // Window & Panels
    QColor windowBg, editorBg, sidebarBg, bottomPanelBg, titleBarBg, menuBarBg, statusBarBg;
    // Borders & Dividers
    QColor border, splitter;
    // Tabs
    QColor tabActiveBg, tabInactiveBg, tabHoverBg, tabActiveFg, tabInactiveFg, tabIndicator;
    // Editor elements
    QColor textNormal, textSelected, selectionBg, currentLineBg, lineNumberNormal, lineNumberActive,
           cursor, indentGuide, bracketMatchBg, bracketMatchBorder;
    // Syntax Highlighting
    QColor keyword, type, function, string, number, comment, preprocessor, operatorColor, constant, variable;
    // Status & Accent
    QColor accent, success, warning, error, info;
};

class ThemeManager {
public:
    static ThemeManager& instance();

    const ThemeColors& currentTheme() const { return m_currentColors; }
    QString currentThemeName() const { return m_currentThemeName; }

    // Applies a built-in theme, or a user theme from <config>/themes/*.json.
    // Unknown names fall back to "LiteForge Obsidian". The accent override from
    // Config::ui().accentOverride is applied on top.
    void applyTheme(const QString& themeName);

    // Built-in themes first, then user themes (sorted).
    std::vector<QString> availableThemes() const;
    std::vector<QString> builtinThemes() const;

    // Re-scan <config>/themes. Only names + paths are kept in memory; a theme
    // file is read when it is actually applied.
    void reloadUserThemes();

    // Writes the current theme as a ready-to-edit JSON template.
    bool exportThemeTemplate(const QString& path) const;

    // Parses a user theme file on top of its "base" theme. Returns false on error.
    bool loadThemeFile(const QString& path, ThemeColors& out, QString* nameOut = nullptr) const;

    QString generateStyleSheet() const;

private:
    ThemeManager();
    bool loadBuiltin(const QString& name, ThemeColors& out) const;
    void applyAccentOverride(ThemeColors& c) const;

    QString m_currentThemeName;
    ThemeColors m_currentColors;
    QHash<QString, QString> m_userThemes; // display name -> file path
};

} // namespace liteforge::core
