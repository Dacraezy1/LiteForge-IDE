#include "liteforge/core/Theme.hpp"
#include "liteforge/core/Config.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <array>

namespace liteforge::core {

namespace {

constexpr int kFieldCount = 40;

struct FieldDef { const char* key; QColor ThemeColors::* member; };

// Order MUST match the color tables below.
const std::array<FieldDef, kFieldCount> kFields = {{
    {"windowBg", &ThemeColors::windowBg},
    {"editorBg", &ThemeColors::editorBg},
    {"sidebarBg", &ThemeColors::sidebarBg},
    {"bottomPanelBg", &ThemeColors::bottomPanelBg},
    {"titleBarBg", &ThemeColors::titleBarBg},
    {"menuBarBg", &ThemeColors::menuBarBg},
    {"statusBarBg", &ThemeColors::statusBarBg},
    {"border", &ThemeColors::border},
    {"splitter", &ThemeColors::splitter},
    {"tabActiveBg", &ThemeColors::tabActiveBg},
    {"tabInactiveBg", &ThemeColors::tabInactiveBg},
    {"tabHoverBg", &ThemeColors::tabHoverBg},
    {"tabActiveFg", &ThemeColors::tabActiveFg},
    {"tabInactiveFg", &ThemeColors::tabInactiveFg},
    {"tabIndicator", &ThemeColors::tabIndicator},
    {"textNormal", &ThemeColors::textNormal},
    {"textSelected", &ThemeColors::textSelected},
    {"selectionBg", &ThemeColors::selectionBg},
    {"currentLineBg", &ThemeColors::currentLineBg},
    {"lineNumberNormal", &ThemeColors::lineNumberNormal},
    {"lineNumberActive", &ThemeColors::lineNumberActive},
    {"cursor", &ThemeColors::cursor},
    {"indentGuide", &ThemeColors::indentGuide},
    {"bracketMatchBg", &ThemeColors::bracketMatchBg},
    {"bracketMatchBorder", &ThemeColors::bracketMatchBorder},
    {"keyword", &ThemeColors::keyword},
    {"type", &ThemeColors::type},
    {"function", &ThemeColors::function},
    {"string", &ThemeColors::string},
    {"number", &ThemeColors::number},
    {"comment", &ThemeColors::comment},
    {"preprocessor", &ThemeColors::preprocessor},
    {"operatorColor", &ThemeColors::operatorColor},
    {"constant", &ThemeColors::constant},
    {"variable", &ThemeColors::variable},
    {"accent", &ThemeColors::accent},
    {"success", &ThemeColors::success},
    {"warning", &ThemeColors::warning},
    {"error", &ThemeColors::error},
    {"info", &ThemeColors::info},
}};

struct BuiltinDef { const char* name; std::array<unsigned, kFieldCount> rgb; };

const std::array<BuiltinDef, 5> kBuiltins = {{
    {"LiteForge Obsidian", {0x141416, 0x18181b, 0x121214, 0x121214, 0x0f0f10, 0x141416, 0x0f0f10, 0x27272a, 0x27272a, 0x18181b, 0x121214, 0x1f1f23, 0xf4f4f5, 0x71717a, 0x38bdf8, 0xe4e4e7, 0xffffff, 0x2b3245, 0x202024, 0x52525b, 0x38bdf8, 0x38bdf8, 0x27272a, 0x334155, 0x38bdf8, 0xc084fc, 0x38bdf8, 0x60a5fa, 0x4ade80, 0xfb923c, 0x71717a, 0xf43f5e, 0x94a3b8, 0xf59e0b, 0xe4e4e7, 0x38bdf8, 0x34d399, 0xfbbf24, 0xf87171, 0x38bdf8}},
    {"One Dark Pro", {0x21252b, 0x282c34, 0x21252b, 0x21252b, 0x1e2227, 0x21252b, 0x1e2227, 0x181a1f, 0x181a1f, 0x282c34, 0x21252b, 0x2c313a, 0xabb2bf, 0x5c6370, 0x61afef, 0xabb2bf, 0xffffff, 0x3e4451, 0x2c313c, 0x4b5263, 0xabb2bf, 0x528bff, 0x3b4048, 0x515a6b, 0x61afef, 0xc678dd, 0xe5c07b, 0x61afef, 0x98c379, 0xd19a66, 0x5c6370, 0xe06c75, 0x56b6c2, 0xd19a66, 0xe06c75, 0x61afef, 0x98c379, 0xe5c07b, 0xe06c75, 0x56b6c2}},
    {"Tokyo Night", {0x16161e, 0x1a1b26, 0x16161e, 0x16161e, 0x13141c, 0x16161e, 0x13141c, 0x101014, 0x292e42, 0x1a1b26, 0x16161e, 0x24283b, 0xa9b1d6, 0x565f89, 0x7aa2f7, 0xa9b1d6, 0xffffff, 0x33467c, 0x1f2335, 0x363b54, 0x7aa2f7, 0xc0caf5, 0x292e42, 0x3b4261, 0x7aa2f7, 0xbb9af7, 0x2ac3de, 0x7aa2f7, 0x9ece6a, 0xff9e64, 0x565f89, 0xf7768e, 0x89ddff, 0xff9e64, 0xe0af68, 0x7aa2f7, 0x9ece6a, 0xe0af68, 0xf7768e, 0x2ac3de}},
    {"Monokai Pro", {0x221f22, 0x2d2a2e, 0x221f22, 0x221f22, 0x19181a, 0x221f22, 0x19181a, 0x19181a, 0x3a383b, 0x2d2a2e, 0x221f22, 0x353236, 0xfcfcfa, 0x727072, 0xffd866, 0xfcfcfa, 0xffffff, 0x49464e, 0x353337, 0x5b595c, 0xffd866, 0xfcfcfa, 0x403e41, 0x49464e, 0xffd866, 0xff6188, 0x78dce8, 0xa9dc76, 0xffd866, 0xab9df2, 0x727072, 0xff6188, 0xff6188, 0xab9df2, 0xfc9867, 0xffd866, 0xa9dc76, 0xfc9867, 0xff6188, 0x78dce8}},
    {"Cyberpunk Neon", {0x080811, 0x0f0f1b, 0x080811, 0x080811, 0x05050a, 0x080811, 0x05050a, 0x1e1e38, 0xff007f, 0x0f0f1b, 0x080811, 0x1a1a2e, 0x00fff5, 0x606080, 0x00fff5, 0xe0e0ff, 0xffffff, 0xff007f, 0x17172b, 0x404070, 0x00fff5, 0x00fff5, 0x222244, 0x331033, 0xff007f, 0xff007f, 0x00fff5, 0xffe600, 0x05ffa1, 0xff9900, 0x606090, 0xff0055, 0x00fff5, 0xffe600, 0xff77aa, 0x00fff5, 0x05ffa1, 0xffe600, 0xff0055, 0x00fff5}},
}};

// ---- Palette-derived themes: ~14 colors in, a full 40-color theme out. ----
struct Palette {
    const char* name;
    bool dark;
    unsigned bg, fg, accent, keyword, type, function, string, number, comment, preproc, constant,
             success, warning, error;
};

QColor mix(const QColor& a, const QColor& b, double t) {
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t);
}

ThemeColors derive(const Palette& p) {
    ThemeColors c;
    const QColor bg(p.bg), fg(p.fg), accent(p.accent);
    const QColor chrome = p.dark ? bg.darker(112) : bg.darker(104);
    c.windowBg = chrome;           c.editorBg = bg;
    c.sidebarBg = chrome;          c.bottomPanelBg = chrome;
    c.titleBarBg = chrome;         c.menuBarBg = chrome;
    c.statusBarBg = p.dark ? bg.darker(125) : bg.darker(108);
    c.border = mix(bg, fg, 0.13);  c.splitter = c.border;
    c.tabActiveBg = bg;            c.tabInactiveBg = chrome;
    c.tabHoverBg = mix(bg, fg, 0.07);
    c.tabActiveFg = fg;            c.tabInactiveFg = mix(bg, fg, 0.55);
    c.tabIndicator = accent;
    c.textNormal = fg;             c.textSelected = p.dark ? QColor(Qt::white) : fg;
    c.selectionBg = mix(bg, accent, p.dark ? 0.30 : 0.22);
    c.currentLineBg = mix(bg, fg, 0.05);
    c.lineNumberNormal = mix(bg, fg, 0.38);
    c.lineNumberActive = accent;   c.cursor = accent;
    c.indentGuide = mix(bg, fg, 0.10);
    c.bracketMatchBg = mix(bg, accent, 0.25);
    c.bracketMatchBorder = accent;
    c.keyword = p.keyword;         c.type = p.type;
    c.function = p.function;       c.string = p.string;
    c.number = p.number;           c.comment = p.comment;
    c.preprocessor = p.preproc;    c.operatorColor = mix(bg, fg, 0.65);
    c.constant = p.constant;       c.variable = fg;
    c.accent = accent;             c.success = p.success;
    c.warning = p.warning;         c.error = p.error;
    c.info = accent;
    return c;
}

const std::array<Palette, 4> kPalettes = {{
    {"LiteForge Daylight", false, 0xfafafa, 0x27272a, 0x0284c7, 0x7c3aed, 0x0369a1, 0x2563eb, 0x15803d,
     0xc2410c, 0x8b8b95, 0xbe123c, 0xb45309, 0x16a34a, 0xd97706, 0xdc2626},
    {"Gruvbox Dark", true, 0x282828, 0xebdbb2, 0xfabd2f, 0xfb4934, 0xfabd2f, 0x83a598, 0xb8bb26,
     0xd3869b, 0x928374, 0x8ec07c, 0xd3869b, 0xb8bb26, 0xfe8019, 0xfb4934},
    {"Nord", true, 0x2e3440, 0xd8dee9, 0x88c0d0, 0x81a1c1, 0x8fbcbb, 0x88c0d0, 0xa3be8c,
     0xb48ead, 0x616e88, 0x5e81ac, 0xd08770, 0xa3be8c, 0xebcb8b, 0xbf616a},
    {"Catppuccin Mocha", true, 0x1e1e2e, 0xcdd6f4, 0xcba6f7, 0xcba6f7, 0xf9e2af, 0x89b4fa, 0xa6e3a1,
     0xfab387, 0x6c7086, 0xf38ba8, 0xfab387, 0xa6e3a1, 0xf9e2af, 0xf38ba8},
}};

const char* const kDefaultTheme = "LiteForge Obsidian";

} // namespace

ThemeManager& ThemeManager::instance() {
    static ThemeManager s_instance;
    return s_instance;
}

ThemeManager::ThemeManager() {
    loadBuiltin(kDefaultTheme, m_currentColors);
    m_currentThemeName = kDefaultTheme;
}

bool ThemeManager::loadBuiltin(const QString& name, ThemeColors& out) const {
    for (const auto& b : kBuiltins) {
        if (name == QLatin1String(b.name)) {
            for (int i = 0; i < kFieldCount; ++i) out.*(kFields[i].member) = QColor::fromRgb(b.rgb[i]);
            return true;
        }
    }
    for (const auto& p : kPalettes) {
        if (name == QLatin1String(p.name)) { out = derive(p); return true; }
    }
    return false;
}

std::vector<QString> ThemeManager::builtinThemes() const {
    std::vector<QString> v;
    for (const auto& b : kBuiltins) v.emplace_back(QLatin1String(b.name));
    for (const auto& p : kPalettes) v.emplace_back(QLatin1String(p.name));
    return v;
}

std::vector<QString> ThemeManager::availableThemes() const {
    auto v = builtinThemes();
    QStringList user = m_userThemes.keys();
    user.sort(Qt::CaseInsensitive);
    for (const auto& u : user) v.push_back(u);
    return v;
}

void ThemeManager::reloadUserThemes() {
    m_userThemes.clear();
    const QDir dir(Config::instance().themesDir());
    if (!dir.exists()) return;
    const auto files = dir.entryInfoList({"*.json"}, QDir::Files, QDir::Name);
    for (const auto& fi : files) {
        QString name;
        ThemeColors scratch;
        if (loadThemeFile(fi.absoluteFilePath(), scratch, &name) && !name.isEmpty())
            m_userThemes.insert(name, fi.absoluteFilePath());
    }
}

bool ThemeManager::loadThemeFile(const QString& path, ThemeColors& out, QString* nameOut) const {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return false;
    const QJsonObject root = doc.object();

    QString base = root.value("base").toString(kDefaultTheme);
    ThemeColors c;
    if (!loadBuiltin(base, c)) loadBuiltin(kDefaultTheme, c);

    const QJsonObject colors = root.value("colors").toObject();
    for (const auto& fd : kFields) {
        const QString hex = colors.value(QLatin1String(fd.key)).toString();
        if (hex.isEmpty()) continue;
        const QColor col(hex);
        if (col.isValid()) c.*(fd.member) = col;
    }
    if (nameOut) *nameOut = root.value("name").toString(QFileInfo(path).completeBaseName());
    out = c;
    return true;
}

void ThemeManager::applyAccentOverride(ThemeColors& c) const {
    const QString hex = Config::instance().ui().accentOverride;
    if (hex.isEmpty()) return;
    const QColor a(hex);
    if (!a.isValid()) return;
    c.accent = c.tabIndicator = c.lineNumberActive = c.cursor = c.bracketMatchBorder = c.info = a;
    c.selectionBg = mix(c.editorBg, a, 0.30);
    c.bracketMatchBg = mix(c.editorBg, a, 0.25);
}

void ThemeManager::applyTheme(const QString& themeName) {
    ThemeColors c;
    QString resolved = themeName;
    if (loadBuiltin(themeName, c)) {
        // built-in
    } else if (m_userThemes.contains(themeName) && loadThemeFile(m_userThemes.value(themeName), c)) {
        // user theme
    } else {
        loadBuiltin(kDefaultTheme, c);
        resolved = kDefaultTheme;
    }
    applyAccentOverride(c);
    m_currentColors = c;
    m_currentThemeName = resolved;
}

bool ThemeManager::exportThemeTemplate(const QString& path) const {
    QJsonObject colors;
    for (const auto& fd : kFields)
        colors[QLatin1String(fd.key)] = (m_currentColors.*(fd.member)).name();
    QJsonObject root;
    root["name"] = QStringLiteral("My Theme");
    root["base"] = m_currentThemeName;
    root["colors"] = colors;
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return f.commit();
}

QString ThemeManager::generateStyleSheet() const {
    const auto& c = m_currentColors;
    const auto& ui = Config::instance().ui();

    int padV = 4, padH = 10, itemPad = 4;
    if (ui.density == QLatin1String("compact")) { padV = 2; padH = 7; itemPad = 2; }
    else if (ui.density == QLatin1String("comfortable")) { padV = 7; padH = 14; itemPad = 7; }
    const int r = ui.cornerRadius;

    QString qss = QStringLiteral(R"(
QWidget { background-color: {{window}}; color: {{text}}; selection-background-color: {{selection}}; selection-color: {{selectedText}}; }
QLabel, QCheckBox, QRadioButton { background: transparent; }
QFrame, QAbstractScrollArea { border: none; }
QToolTip { background-color: {{editor}}; color: {{text}}; border: 1px solid {{border}}; padding: 3px 6px; }

QMenuBar { background-color: {{menubar}}; padding: 0px 4px; }
QMenuBar::item { background: transparent; padding: {{padV}}px {{padH}}px; border-radius: {{r}}px; }
QMenuBar::item:selected { background-color: {{hover}}; }
QMenu { background-color: {{editor}}; border: 1px solid {{border}}; padding: 4px; }
QMenu::item { padding: {{padV}}px 22px {{padV}}px 14px; border-radius: {{r}}px; }
QMenu::item:selected { background-color: {{selection}}; color: {{selectedText}}; }
QMenu::separator { height: 1px; background-color: {{border}}; margin: 4px 6px; }

QToolBar { background-color: {{menubar}}; border: none; border-bottom: 1px solid {{border}}; padding: 1px 4px; spacing: 2px; }
QToolButton { background: transparent; border-radius: {{r}}px; padding: {{padV}}px {{padH}}px; }
QToolButton:hover { background-color: {{hover}}; }
QToolButton:pressed { background-color: {{selection}}; }

QTabWidget::pane { border: none; background-color: {{editor}}; }
QTabBar { background: transparent; }
QTabBar::tab { background-color: {{tabInactive}}; color: {{tabInactiveFg}}; padding: {{padV}}px {{padH}}px; border: none; border-bottom: 2px solid transparent; }
QTabBar::tab:hover { background-color: {{hover}}; color: {{text}}; }
QTabBar::tab:selected { background-color: {{editor}}; color: {{tabActiveFg}}; border-bottom: 2px solid {{accent}}; }

QSplitter::handle { background-color: {{border}}; }
QSplitter::handle:horizontal { width: 1px; }
QSplitter::handle:vertical { height: 1px; }

QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 0; }
QScrollBar::handle { background-color: {{lineNum}}; border-radius: 3px; min-height: 28px; min-width: 28px; margin: 2px; }
QScrollBar::handle:hover { background-color: {{accent}}; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QTreeView, QListView, QTableWidget { background-color: {{sidebar}}; outline: none; }
QTreeView::item, QListView::item { padding: {{itemPad}}px 4px; border-radius: {{r}}px; }
QTreeView::item:hover, QListView::item:hover { background-color: {{hover}}; }
QTreeView::item:selected, QListView::item:selected { background-color: {{selection}}; color: {{selectedText}}; }
QHeaderView::section { background-color: {{sidebar}}; color: {{tabInactiveFg}}; border: none; padding: 3px 6px; }

QLineEdit, QSpinBox, QComboBox { background-color: {{editor}}; border: 1px solid {{border}}; border-radius: {{r}}px; padding: {{itemPad}}px 8px; }
QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: {{accent}}; }
QComboBox::drop-down { border: none; width: 18px; }
QComboBox QAbstractItemView { background-color: {{editor}}; border: 1px solid {{border}}; selection-background-color: {{selection}}; }

QPushButton { background-color: {{hover}}; border: 1px solid {{border}}; border-radius: {{r}}px; padding: {{padV}}px {{padH}}px; }
QPushButton:hover { border-color: {{accent}}; }
QPushButton:pressed { background-color: {{selection}}; }
QPushButton:disabled { color: {{lineNum}}; }

QStatusBar { background-color: {{statusbar}}; color: {{tabInactiveFg}}; border-top: 1px solid {{border}}; }
QStatusBar::item { border: none; }
QStatusBar QLabel, QStatusBar QPushButton { background: transparent; padding: 0px 6px; }
QStatusBar QPushButton { border: none; border-radius: {{r}}px; }
QStatusBar QPushButton:hover { background-color: {{hover}}; color: {{text}}; }
)");

    const QHash<QString, QString> tokens = {
        {"window", c.windowBg.name()},       {"text", c.textNormal.name()},
        {"selection", c.selectionBg.name()}, {"selectedText", c.textSelected.name()},
        {"menubar", c.menuBarBg.name()},     {"border", c.border.name()},
        {"hover", c.tabHoverBg.name()},      {"editor", c.editorBg.name()},
        {"tabInactive", c.tabInactiveBg.name()}, {"tabInactiveFg", c.tabInactiveFg.name()},
        {"tabActiveFg", c.tabActiveFg.name()},   {"accent", c.accent.name()},
        {"lineNum", c.lineNumberNormal.name()},  {"sidebar", c.sidebarBg.name()},
        {"statusbar", c.statusBarBg.name()},
        {"padV", QString::number(padV)}, {"padH", QString::number(padH)},
        {"itemPad", QString::number(itemPad)}, {"r", QString::number(r)},
    };
    for (auto it = tokens.cbegin(); it != tokens.cend(); ++it)
        qss.replace(QStringLiteral("{{") + it.key() + QStringLiteral("}}"), it.value());
    return qss;
}

} // namespace liteforge::core
