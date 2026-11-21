#include "liteforge/core/Config.hpp"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

namespace liteforge::core {

Config& Config::instance() {
    static Config s_instance;
    return s_instance;
}

QString Config::configDir() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir;
}

QString Config::configFilePath() const { return QDir(configDir()).filePath("config.json"); }
QString Config::keybindingsFilePath() const { return QDir(configDir()).filePath("keybindings.json"); }
QString Config::themesDir() const { return QDir(configDir()).filePath("themes"); }

void Config::addRecentProject(const QString& path) {
    if (path.isEmpty()) return;
    m_recentProjects.removeAll(path);
    m_recentProjects.prepend(path);
    while (m_recentProjects.size() > 10) m_recentProjects.removeLast();
}

namespace {
QByteArray b64(const QJsonObject& o, const char* key) {
    return QByteArray::fromBase64(o.value(QLatin1String(key)).toString().toUtf8());
}
QString b64s(const QByteArray& a) { return QString::fromLatin1(a.toBase64()); }
} // namespace

void Config::load() {
    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return;
    const QJsonObject root = doc.object();

    {
        const QJsonObject o = root.value("editor").toObject();
        auto& e = m_editor;
        e.fontFamily = o.value("fontFamily").toString(e.fontFamily);
        e.fontSize = std::clamp(o.value("fontSize").toInt(e.fontSize), 6, 72);
        e.tabWidth = std::clamp(o.value("tabWidth").toInt(e.tabWidth), 1, 16);
        e.useSpacesForTabs = o.value("useSpacesForTabs").toBool(e.useSpacesForTabs);
        e.wordWrap = o.value("wordWrap").toBool(e.wordWrap);
        e.showLineNumbers = o.value("showLineNumbers").toBool(e.showLineNumbers);
        e.highlightCurrentLine = o.value("highlightCurrentLine").toBool(e.highlightCurrentLine);
        e.autoCloseBrackets = o.value("autoCloseBrackets").toBool(e.autoCloseBrackets);
        e.showWhitespace = o.value("showWhitespace").toBool(e.showWhitespace);
        e.trimTrailingWhitespaceOnSave = o.value("trimTrailingWhitespaceOnSave").toBool(e.trimTrailingWhitespaceOnSave);
        e.insertFinalNewline = o.value("insertFinalNewline").toBool(e.insertFinalNewline);
        e.rulerColumn = o.value("rulerColumn").toInt(e.rulerColumn);
        e.showRuler = o.value("showRuler").toBool(e.showRuler);
        e.minimap = o.value("minimap").toBool(e.minimap);
        e.stickyScroll = o.value("stickyScroll").toBool(e.stickyScroll);
        e.largeFileKiB = std::max(0, o.value("largeFileKiB").toInt(e.largeFileKiB));
        e.autoSaveDelayMs = std::max(0, o.value("autoSaveDelayMs").toInt(e.autoSaveDelayMs));
        e.autoSaveOnFocusLoss = o.value("autoSaveOnFocusLoss").toBool(e.autoSaveOnFocusLoss);
    }
    {
        const QJsonObject o = root.value("ui").toObject();
        auto& u = m_ui;
        u.fontFamily = o.value("fontFamily").toString(u.fontFamily);
        u.fontSize = std::clamp(o.value("fontSize").toInt(u.fontSize), 0, 40);
        u.density = o.value("density").toString(u.density);
        u.cornerRadius = std::clamp(o.value("cornerRadius").toInt(u.cornerRadius), 0, 16);
        u.accentOverride = o.value("accent").toString(u.accentOverride);
        u.showToolbar = o.value("showToolbar").toBool(u.showToolbar);
        u.showStatusBar = o.value("showStatusBar").toBool(u.showStatusBar);
    }
    {
        const QJsonObject o = root.value("general").toObject();
        m_general.restoreSession = o.value("restoreSession").toBool(m_general.restoreSession);
        m_general.lspEnabled = o.value("lspEnabled").toBool(m_general.lspEnabled);
    }
    {
        const QJsonObject o = root.value("window").toObject();
        m_window.sidebarVisible = o.value("sidebarVisible").toBool(m_window.sidebarVisible);
        m_window.bottomPanelVisible = o.value("bottomPanelVisible").toBool(m_window.bottomPanelVisible);
        m_window.activeBottomTab = o.value("activeBottomTab").toInt(m_window.activeBottomTab);
        m_window.geometry = b64(o, "geometry");
        m_window.state = b64(o, "state");
        m_window.mainSplitterState = b64(o, "mainSplitter");
        m_window.bottomSplitterState = b64(o, "bottomSplitter");
    }
    {
        const QJsonObject o = root.value("build").toObject();
        m_build.customBuildCommand = o.value("customBuildCommand").toString();
        m_build.customRunCommand = o.value("customRunCommand").toString();
        m_build.cmakeGenerator = o.value("cmakeGenerator").toString(m_build.cmakeGenerator);
        m_build.buildType = o.value("buildType").toString(m_build.buildType);
        m_build.autoSaveBeforeBuild = o.value("autoSaveBeforeBuild").toBool(m_build.autoSaveBeforeBuild);
    }

    m_lastProjectPath = root.value("lastProjectPath").toString();
    m_activeTheme = root.value("activeTheme").toString(m_activeTheme);
    m_activeFile = root.value("activeFile").toString();

    m_recentProjects.clear();
    for (const auto& v : root.value("recentProjects").toArray()) m_recentProjects << v.toString();
    m_openFiles.clear();
    for (const auto& v : root.value("openFiles").toArray()) m_openFiles << v.toString();
}

void Config::save() {
    QJsonObject root;
    {
        const auto& e = m_editor;
        QJsonObject o;
        o["fontFamily"] = e.fontFamily;
        o["fontSize"] = e.fontSize;
        o["tabWidth"] = e.tabWidth;
        o["useSpacesForTabs"] = e.useSpacesForTabs;
        o["wordWrap"] = e.wordWrap;
        o["showLineNumbers"] = e.showLineNumbers;
        o["highlightCurrentLine"] = e.highlightCurrentLine;
        o["autoCloseBrackets"] = e.autoCloseBrackets;
        o["showWhitespace"] = e.showWhitespace;
        o["trimTrailingWhitespaceOnSave"] = e.trimTrailingWhitespaceOnSave;
        o["insertFinalNewline"] = e.insertFinalNewline;
        o["rulerColumn"] = e.rulerColumn;
        o["showRuler"] = e.showRuler;
        o["minimap"] = e.minimap;
        o["stickyScroll"] = e.stickyScroll;
        o["largeFileKiB"] = e.largeFileKiB;
        o["autoSaveDelayMs"] = e.autoSaveDelayMs;
        o["autoSaveOnFocusLoss"] = e.autoSaveOnFocusLoss;
        root["editor"] = o;
    }
    {
        const auto& u = m_ui;
        QJsonObject o;
        o["fontFamily"] = u.fontFamily;
        o["fontSize"] = u.fontSize;
        o["density"] = u.density;
        o["cornerRadius"] = u.cornerRadius;
        o["accent"] = u.accentOverride;
        o["showToolbar"] = u.showToolbar;
        o["showStatusBar"] = u.showStatusBar;
        root["ui"] = o;
    }
    {
        QJsonObject o;
        o["restoreSession"] = m_general.restoreSession;
        o["lspEnabled"] = m_general.lspEnabled;
        root["general"] = o;
    }
    {
        QJsonObject o;
        o["sidebarVisible"] = m_window.sidebarVisible;
        o["bottomPanelVisible"] = m_window.bottomPanelVisible;
        o["activeBottomTab"] = m_window.activeBottomTab;
        o["geometry"] = b64s(m_window.geometry);
        o["state"] = b64s(m_window.state);
        o["mainSplitter"] = b64s(m_window.mainSplitterState);
        o["bottomSplitter"] = b64s(m_window.bottomSplitterState);
        root["window"] = o;
    }
    {
        QJsonObject o;
        o["customBuildCommand"] = m_build.customBuildCommand;
        o["customRunCommand"] = m_build.customRunCommand;
        o["cmakeGenerator"] = m_build.cmakeGenerator;
        o["buildType"] = m_build.buildType;
        o["autoSaveBeforeBuild"] = m_build.autoSaveBeforeBuild;
        root["build"] = o;
    }

    root["lastProjectPath"] = m_lastProjectPath;
    root["activeTheme"] = m_activeTheme;
    root["activeFile"] = m_activeFile;
    root["recentProjects"] = QJsonArray::fromStringList(m_recentProjects);
    root["openFiles"] = QJsonArray::fromStringList(m_openFiles);

    // Atomic write: a crash mid-save can never leave a truncated config.json.
    QSaveFile file(configFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.commit();
    }
}

} // namespace liteforge::core
