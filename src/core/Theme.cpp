#include "liteforge/core/Theme.hpp"

namespace liteforge::core {

ThemeManager& ThemeManager::instance() {
    static ThemeManager s_instance;
    return s_instance;
}

ThemeManager::ThemeManager() {
    loadPreset(ThemePreset::LiteForgeObsidian);
}

std::vector<QString> ThemeManager::availableThemes() const {
    return {
        "LiteForge Obsidian",
        "One Dark Pro",
        "Tokyo Night",
        "Monokai Pro",
        "Cyberpunk Neon"
    };
}

void ThemeManager::applyTheme(const QString& themeName) {
    if (themeName == "One Dark Pro") {
        loadPreset(ThemePreset::OneDarkPro);
    } else if (themeName == "Tokyo Night") {
        loadPreset(ThemePreset::TokyoNight);
    } else if (themeName == "Monokai Pro") {
        loadPreset(ThemePreset::MonokaiPro);
    } else if (themeName == "Cyberpunk Neon") {
        loadPreset(ThemePreset::CyberpunkNeon);
    } else {
        loadPreset(ThemePreset::LiteForgeObsidian);
    }
}

void ThemeManager::applyTheme(ThemePreset preset) {
    loadPreset(preset);
}

void ThemeManager::loadPreset(ThemePreset preset) {
    switch (preset) {
    case ThemePreset::OneDarkPro: {
        m_currentThemeName = "One Dark Pro";
        m_currentColors.windowBg = QColor("#21252b");
        m_currentColors.editorBg = QColor("#282c34");
        m_currentColors.sidebarBg = QColor("#21252b");
        m_currentColors.bottomPanelBg = QColor("#21252b");
        m_currentColors.titleBarBg = QColor("#1e2227");
        m_currentColors.menuBarBg = QColor("#21252b");
        m_currentColors.statusBarBg = QColor("#1e2227");

        m_currentColors.border = QColor("#181a1f");
        m_currentColors.splitter = QColor("#181a1f");

        m_currentColors.tabActiveBg = QColor("#282c34");
        m_currentColors.tabInactiveBg = QColor("#21252b");
        m_currentColors.tabHoverBg = QColor("#2c313a");
        m_currentColors.tabActiveFg = QColor("#abb2bf");
        m_currentColors.tabInactiveFg = QColor("#5c6370");
        m_currentColors.tabIndicator = QColor("#61afef");

        m_currentColors.textNormal = QColor("#abb2bf");
        m_currentColors.textSelected = QColor("#ffffff");
        m_currentColors.selectionBg = QColor("#3e4451");
        m_currentColors.currentLineBg = QColor("#2c313c");
        m_currentColors.lineNumberNormal = QColor("#4b5263");
        m_currentColors.lineNumberActive = QColor("#abb2bf");
        m_currentColors.cursor = QColor("#528bff");
        m_currentColors.indentGuide = QColor("#3b4048");
        m_currentColors.bracketMatchBg = QColor("#515a6b");
        m_currentColors.bracketMatchBorder = QColor("#61afef");

        m_currentColors.keyword = QColor("#c678dd");
        m_currentColors.type = QColor("#e5c07b");
        m_currentColors.function = QColor("#61afef");
        m_currentColors.string = QColor("#98c379");
        m_currentColors.number = QColor("#d19a66");
        m_currentColors.comment = QColor("#5c6370");
        m_currentColors.preprocessor = QColor("#e06c75");
        m_currentColors.operatorColor = QColor("#56b6c2");
        m_currentColors.constant = QColor("#d19a66");
        m_currentColors.variable = QColor("#e06c75");

        m_currentColors.accent = QColor("#61afef");
        m_currentColors.success = QColor("#98c379");
        m_currentColors.warning = QColor("#e5c07b");
        m_currentColors.error = QColor("#e06c75");
        m_currentColors.info = QColor("#56b6c2");
        break;
    }
    case ThemePreset::TokyoNight: {
        m_currentThemeName = "Tokyo Night";
        m_currentColors.windowBg = QColor("#16161e");
        m_currentColors.editorBg = QColor("#1a1b26");
        m_currentColors.sidebarBg = QColor("#16161e");
        m_currentColors.bottomPanelBg = QColor("#16161e");
        m_currentColors.titleBarBg = QColor("#13141c");
        m_currentColors.menuBarBg = QColor("#16161e");
        m_currentColors.statusBarBg = QColor("#13141c");

        m_currentColors.border = QColor("#101014");
        m_currentColors.splitter = QColor("#292e42");

        m_currentColors.tabActiveBg = QColor("#1a1b26");
        m_currentColors.tabInactiveBg = QColor("#16161e");
        m_currentColors.tabHoverBg = QColor("#24283b");
        m_currentColors.tabActiveFg = QColor("#a9b1d6");
        m_currentColors.tabInactiveFg = QColor("#565f89");
        m_currentColors.tabIndicator = QColor("#7aa2f7");

        m_currentColors.textNormal = QColor("#a9b1d6");
        m_currentColors.textSelected = QColor("#ffffff");
        m_currentColors.selectionBg = QColor("#33467c");
        m_currentColors.currentLineBg = QColor("#1f2335");
        m_currentColors.lineNumberNormal = QColor("#363b54");
        m_currentColors.lineNumberActive = QColor("#7aa2f7");
        m_currentColors.cursor = QColor("#c0caf5");
        m_currentColors.indentGuide = QColor("#292e42");
        m_currentColors.bracketMatchBg = QColor("#3b4261");
        m_currentColors.bracketMatchBorder = QColor("#7aa2f7");

        m_currentColors.keyword = QColor("#bb9af7");
        m_currentColors.type = QColor("#2ac3de");
        m_currentColors.function = QColor("#7aa2f7");
        m_currentColors.string = QColor("#9ece6a");
        m_currentColors.number = QColor("#ff9e64");
        m_currentColors.comment = QColor("#565f89");
        m_currentColors.preprocessor = QColor("#f7768e");
        m_currentColors.operatorColor = QColor("#89ddff");
        m_currentColors.constant = QColor("#ff9e64");
        m_currentColors.variable = QColor("#e0af68");

        m_currentColors.accent = QColor("#7aa2f7");
        m_currentColors.success = QColor("#9ece6a");
        m_currentColors.warning = QColor("#e0af68");
        m_currentColors.error = QColor("#f7768e");
        m_currentColors.info = QColor("#2ac3de");
        break;
    }
    case ThemePreset::MonokaiPro: {
        m_currentThemeName = "Monokai Pro";
        m_currentColors.windowBg = QColor("#221f22");
        m_currentColors.editorBg = QColor("#2d2a2e");
        m_currentColors.sidebarBg = QColor("#221f22");
        m_currentColors.bottomPanelBg = QColor("#221f22");
        m_currentColors.titleBarBg = QColor("#19181a");
        m_currentColors.menuBarBg = QColor("#221f22");
        m_currentColors.statusBarBg = QColor("#19181a");

        m_currentColors.border = QColor("#19181a");
        m_currentColors.splitter = QColor("#3a383b");

        m_currentColors.tabActiveBg = QColor("#2d2a2e");
        m_currentColors.tabInactiveBg = QColor("#221f22");
        m_currentColors.tabHoverBg = QColor("#353236");
        m_currentColors.tabActiveFg = QColor("#fcfcfa");
        m_currentColors.tabInactiveFg = QColor("#727072");
        m_currentColors.tabIndicator = QColor("#ffd866");

        m_currentColors.textNormal = QColor("#fcfcfa");
        m_currentColors.textSelected = QColor("#ffffff");
        m_currentColors.selectionBg = QColor("#49464e");
        m_currentColors.currentLineBg = QColor("#353337");
        m_currentColors.lineNumberNormal = QColor("#5b595c");
        m_currentColors.lineNumberActive = QColor("#ffd866");
        m_currentColors.cursor = QColor("#fcfcfa");
        m_currentColors.indentGuide = QColor("#403e41");
        m_currentColors.bracketMatchBg = QColor("#49464e");
        m_currentColors.bracketMatchBorder = QColor("#ffd866");

        m_currentColors.keyword = QColor("#ff6188");
        m_currentColors.type = QColor("#78dce8");
        m_currentColors.function = QColor("#a9dc76");
        m_currentColors.string = QColor("#ffd866");
        m_currentColors.number = QColor("#ab9df2");
        m_currentColors.comment = QColor("#727072");
        m_currentColors.preprocessor = QColor("#ff6188");
        m_currentColors.operatorColor = QColor("#ff6188");
        m_currentColors.constant = QColor("#ab9df2");
        m_currentColors.variable = QColor("#fc9867");

        m_currentColors.accent = QColor("#ffd866");
        m_currentColors.success = QColor("#a9dc76");
        m_currentColors.warning = QColor("#fc9867");
        m_currentColors.error = QColor("#ff6188");
        m_currentColors.info = QColor("#78dce8");
        break;
    }
    case ThemePreset::CyberpunkNeon: {
        m_currentThemeName = "Cyberpunk Neon";
        m_currentColors.windowBg = QColor("#080811");
        m_currentColors.editorBg = QColor("#0f0f1b");
        m_currentColors.sidebarBg = QColor("#080811");
        m_currentColors.bottomPanelBg = QColor("#080811");
        m_currentColors.titleBarBg = QColor("#05050a");
        m_currentColors.menuBarBg = QColor("#080811");
        m_currentColors.statusBarBg = QColor("#05050a");

        m_currentColors.border = QColor("#1e1e38");
        m_currentColors.splitter = QColor("#ff007f");

        m_currentColors.tabActiveBg = QColor("#0f0f1b");
        m_currentColors.tabInactiveBg = QColor("#080811");
        m_currentColors.tabHoverBg = QColor("#1a1a2e");
        m_currentColors.tabActiveFg = QColor("#00fff5");
        m_currentColors.tabInactiveFg = QColor("#606080");
        m_currentColors.tabIndicator = QColor("#00fff5");

        m_currentColors.textNormal = QColor("#e0e0ff");
        m_currentColors.textSelected = QColor("#ffffff");
        m_currentColors.selectionBg = QColor("#ff007f");
        m_currentColors.currentLineBg = QColor("#17172b");
        m_currentColors.lineNumberNormal = QColor("#404070");
        m_currentColors.lineNumberActive = QColor("#00fff5");
        m_currentColors.cursor = QColor("#00fff5");
        m_currentColors.indentGuide = QColor("#222244");
        m_currentColors.bracketMatchBg = QColor("#331033");
        m_currentColors.bracketMatchBorder = QColor("#ff007f");

        m_currentColors.keyword = QColor("#ff007f");
        m_currentColors.type = QColor("#00fff5");
        m_currentColors.function = QColor("#ffe600");
        m_currentColors.string = QColor("#05ffa1");
        m_currentColors.number = QColor("#ff9900");
        m_currentColors.comment = QColor("#606090");
        m_currentColors.preprocessor = QColor("#ff0055");
        m_currentColors.operatorColor = QColor("#00fff5");
        m_currentColors.constant = QColor("#ffe600");
        m_currentColors.variable = QColor("#ff77aa");

        m_currentColors.accent = QColor("#00fff5");
        m_currentColors.success = QColor("#05ffa1");
        m_currentColors.warning = QColor("#ffe600");
        m_currentColors.error = QColor("#ff0055");
        m_currentColors.info = QColor("#00fff5");
        break;
    }
    case ThemePreset::LiteForgeObsidian:
    default: {
        m_currentThemeName = "LiteForge Obsidian";
        m_currentColors.windowBg = QColor("#141416");
        m_currentColors.editorBg = QColor("#18181b");
        m_currentColors.sidebarBg = QColor("#121214");
        m_currentColors.bottomPanelBg = QColor("#121214");
        m_currentColors.titleBarBg = QColor("#0f0f10");
        m_currentColors.menuBarBg = QColor("#141416");
        m_currentColors.statusBarBg = QColor("#0f0f10");

        m_currentColors.border = QColor("#27272a");
        m_currentColors.splitter = QColor("#27272a");

        m_currentColors.tabActiveBg = QColor("#18181b");
        m_currentColors.tabInactiveBg = QColor("#121214");
        m_currentColors.tabHoverBg = QColor("#1f1f23");
        m_currentColors.tabActiveFg = QColor("#f4f4f5");
        m_currentColors.tabInactiveFg = QColor("#71717a");
        m_currentColors.tabIndicator = QColor("#38bdf8");

        m_currentColors.textNormal = QColor("#e4e4e7");
        m_currentColors.textSelected = QColor("#ffffff");
        m_currentColors.selectionBg = QColor("#2b3245");
        m_currentColors.currentLineBg = QColor("#202024");
        m_currentColors.lineNumberNormal = QColor("#52525b");
        m_currentColors.lineNumberActive = QColor("#38bdf8");
        m_currentColors.cursor = QColor("#38bdf8");
        m_currentColors.indentGuide = QColor("#27272a");
        m_currentColors.bracketMatchBg = QColor("#334155");
        m_currentColors.bracketMatchBorder = QColor("#38bdf8");

        m_currentColors.keyword = QColor("#c084fc");
        m_currentColors.type = QColor("#38bdf8");
        m_currentColors.function = QColor("#60a5fa");
        m_currentColors.string = QColor("#4ade80");
        m_currentColors.number = QColor("#fb923c");
        m_currentColors.comment = QColor("#71717a");
        m_currentColors.preprocessor = QColor("#f43f5e");
        m_currentColors.operatorColor = QColor("#94a3b8");
        m_currentColors.constant = QColor("#f59e0b");
        m_currentColors.variable = QColor("#e4e4e7");

        m_currentColors.accent = QColor("#38bdf8");
        m_currentColors.success = QColor("#34d399");
        m_currentColors.warning = QColor("#fbbf24");
        m_currentColors.error = QColor("#f87171");
        m_currentColors.info = QColor("#38bdf8");
        break;
    }
    }
}

QString ThemeManager::generateStyleSheet() const {
    const auto& c = m_currentColors;

    return QString(R"(
        /* Global Widget Style */
        QWidget {
            background-color: %1;
            color: %2;
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            font-size: 13px;
            selection-background-color: %3;
            selection-color: %4;
            border: none;
        }

        /* Main Window */
        QMainWindow {
            background-color: %1;
        }

        /* Menu Bar */
        QMenuBar {
            background-color: %5;
            color: %6;
            border-bottom: 1px solid %7;
            padding: 2px 4px;
        }
        QMenuBar::item {
            background: transparent;
            padding: 4px 10px;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background-color: %8;
            color: #ffffff;
        }

        /* Menus */
        QMenu {
            background-color: %1;
            color: %2;
            border: 1px solid %7;
            border-radius: 6px;
            padding: 4px;
        }
        QMenu::item {
            padding: 5px 24px 5px 20px;
            border-radius: 4px;
        }
        QMenu::item:selected {
            background-color: %8;
            color: #ffffff;
        }
        QMenu::separator {
            height: 1px;
            background-color: %7;
            margin: 4px 6px;
        }

        /* ToolBar */
        QToolBar {
            background-color: %5;
            border-bottom: 1px solid %7;
            padding: 2px 4px;
            spacing: 4px;
        }
        QToolButton {
            background-color: transparent;
            color: %2;
            border-radius: 4px;
            padding: 4px 8px;
            font-weight: 500;
        }
        QToolButton:hover {
            background-color: %8;
            color: #ffffff;
        }
        QToolButton:pressed {
            background-color: %3;
        }

        /* Tabs */
        QTabWidget::pane {
            border: none;
            background-color: %9;
        }
        QTabBar::tab {
            background-color: %10;
            color: %11;
            padding: 6px 14px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            border-right: 1px solid %7;
            min-width: 80px;
        }
        QTabBar::tab:hover {
            background-color: %12;
            color: %2;
        }
        QTabBar::tab:selected {
            background-color: %9;
            color: %13;
            border-bottom: 2px solid %14;
        }

        /* Splitters */
        QSplitter::handle {
            background-color: %7;
        }
        QSplitter::handle:horizontal {
            width: 1px;
        }
        QSplitter::handle:vertical {
            height: 1px;
        }

        /* Scrollbars (Modern Minimalist Thin) */
        QScrollBar:vertical {
            border: none;
            background-color: transparent;
            width: 8px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background-color: %15;
            min-height: 24px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: %16;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar::horizontal {
            border: none;
            background-color: transparent;
            height: 8px;
            margin: 0px;
        }
        QScrollBar::handle:horizontal {
            background-color: %15;
            min-width: 24px;
            border-radius: 4px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: %16;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }

        /* Trees / Lists */
        QTreeView, QListView, QTableWidget {
            background-color: %17;
            color: %2;
            border: none;
            outline: none;
        }
        QTreeView::item, QListView::item {
            padding: 4px 6px;
            border-radius: 3px;
        }
        QTreeView::item:hover, QListView::item:hover {
            background-color: %8;
        }
        QTreeView::item:selected, QListView::item:selected {
            background-color: %3;
            color: %4;
        }

        /* LineEdits and Inputs */
        QLineEdit {
            background-color: %18;
            color: %2;
            border: 1px solid %7;
            border-radius: 4px;
            padding: 5px 8px;
        }
        QLineEdit:focus {
            border: 1px solid %14;
        }

        /* PushButtons */
        QPushButton {
            background-color: %8;
            color: %2;
            border: 1px solid %7;
            border-radius: 4px;
            padding: 5px 12px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: %12;
            border-color: %14;
            color: #ffffff;
        }
        QPushButton:pressed {
            background-color: %3;
        }

        /* Status Bar */
        QStatusBar {
            background-color: %19;
            color: %11;
            border-top: 1px solid %7;
        }
        QStatusBar QLabel, QStatusBar QPushButton {
            font-size: 11px;
            padding: 1px 6px;
        }
        QStatusBar QPushButton {
            border: none;
            background: transparent;
        }
        QStatusBar QPushButton:hover {
            background-color: %8;
            border-radius: 3px;
            color: #ffffff;
        }
    )")
    .arg(c.windowBg.name())               // 1
    .arg(c.textNormal.name())             // 2
    .arg(c.selectionBg.name())            // 3
    .arg(c.textSelected.name())           // 4
    .arg(c.menuBarBg.name())              // 5
    .arg(c.textNormal.name())             // 6
    .arg(c.border.name())                 // 7
    .arg(c.tabHoverBg.name())             // 8
    .arg(c.editorBg.name())               // 9
    .arg(c.tabInactiveBg.name())          // 10
    .arg(c.tabInactiveFg.name())          // 11
    .arg(c.tabHoverBg.name())             // 12
    .arg(c.tabActiveFg.name())            // 13
    .arg(c.accent.name())                 // 14
    .arg(c.lineNumberNormal.name())       // 15
    .arg(c.accent.name())                 // 16
    .arg(c.sidebarBg.name())              // 17
    .arg(c.editorBg.name())               // 18
    .arg(c.statusBarBg.name());           // 19
}

} // namespace liteforge::core
