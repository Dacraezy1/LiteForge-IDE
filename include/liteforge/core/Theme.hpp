#pragma once

#include <QString>
#include <QColor>
#include <map>
#include <vector>

namespace liteforge::core {

enum class ThemePreset {
    LiteForgeObsidian,
    OneDarkPro,
    TokyoNight,
    MonokaiPro,
    CyberpunkNeon
};

struct ThemeColors {
    // Window & Panels
    QColor windowBg;
    QColor editorBg;
    QColor sidebarBg;
    QColor bottomPanelBg;
    QColor titleBarBg;
    QColor menuBarBg;
    QColor statusBarBg;

    // Borders & Dividers
    QColor border;
    QColor splitter;

    // Tabs
    QColor tabActiveBg;
    QColor tabInactiveBg;
    QColor tabHoverBg;
    QColor tabActiveFg;
    QColor tabInactiveFg;
    QColor tabIndicator;

    // Editor elements
    QColor textNormal;
    QColor textSelected;
    QColor selectionBg;
    QColor currentLineBg;
    QColor lineNumberNormal;
    QColor lineNumberActive;
    QColor cursor;
    QColor indentGuide;
    QColor bracketMatchBg;
    QColor bracketMatchBorder;

    // Syntax Highlighting
    QColor keyword;
    QColor type;
    QColor function;
    QColor string;
    QColor number;
    QColor comment;
    QColor preprocessor;
    QColor operatorColor;
    QColor constant;
    QColor variable;

    // Status & Accent
    QColor accent;
    QColor success;
    QColor warning;
    QColor error;
    QColor info;
};

class ThemeManager {
public:
    static ThemeManager& instance();

    const ThemeColors& currentTheme() const { return m_currentColors; }
    QString currentThemeName() const { return m_currentThemeName; }

    void applyTheme(const QString& themeName);
    void applyTheme(ThemePreset preset);

    std::vector<QString> availableThemes() const;

    QString generateStyleSheet() const;

private:
    ThemeManager();

    void loadPreset(ThemePreset preset);

    QString m_currentThemeName{"LiteForge Obsidian"};
    ThemeColors m_currentColors;
};

} // namespace liteforge::core
