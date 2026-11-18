#include "liteforge/core/Config.hpp"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace liteforge::core {

Config& Config::instance() {
    static Config s_instance;
    return s_instance;
}

QString Config::configFilePath() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.filePath("config.json");
}

void Config::addRecentProject(const QString& path) {
    if (path.isEmpty()) return;
    m_recentProjects.removeAll(path);
    m_recentProjects.prepend(path);
    while (m_recentProjects.size() > 10) {
        m_recentProjects.removeLast();
    }
}

void Config::load() {
    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return; // default settings
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return;

    QJsonObject root = doc.object();

    // Editor settings
    if (root.contains("editor")) {
        QJsonObject ed = root["editor"].toObject();
        m_editor.fontFamily = ed.value("fontFamily").toString(m_editor.fontFamily);
        m_editor.fontSize = ed.value("fontSize").toInt(m_editor.fontSize);
        m_editor.tabWidth = ed.value("tabWidth").toInt(m_editor.tabWidth);
        m_editor.useSpacesForTabs = ed.value("useSpacesForTabs").toBool(m_editor.useSpacesForTabs);
        m_editor.wordWrap = ed.value("wordWrap").toBool(m_editor.wordWrap);
        m_editor.showLineNumbers = ed.value("showLineNumbers").toBool(m_editor.showLineNumbers);
        m_editor.highlightCurrentLine = ed.value("highlightCurrentLine").toBool(m_editor.highlightCurrentLine);
        m_editor.autoCloseBrackets = ed.value("autoCloseBrackets").toBool(m_editor.autoCloseBrackets);
        m_editor.showWhitespace = ed.value("showWhitespace").toBool(m_editor.showWhitespace);
        m_editor.trimTrailingWhitespaceOnSave = ed.value("trimTrailingWhitespaceOnSave").toBool(m_editor.trimTrailingWhitespaceOnSave);
        m_editor.insertFinalNewline = ed.value("insertFinalNewline").toBool(m_editor.insertFinalNewline);
    }

    // Window settings
    if (root.contains("window")) {
        QJsonObject win = root["window"].toObject();
        m_window.sidebarVisible = win.value("sidebarVisible").toBool(m_window.sidebarVisible);
        m_window.bottomPanelVisible = win.value("bottomPanelVisible").toBool(m_window.bottomPanelVisible);
        m_window.activeBottomTab = win.value("activeBottomTab").toInt(m_window.activeBottomTab);
        if (win.contains("geometry")) {
            m_window.geometry = QByteArray::fromBase64(win["geometry"].toString().toUtf8());
        }
        if (win.contains("state")) {
            m_window.state = QByteArray::fromBase64(win["state"].toString().toUtf8());
        }
        if (win.contains("mainSplitter")) {
            m_window.mainSplitterState = QByteArray::fromBase64(win["mainSplitter"].toString().toUtf8());
        }
        if (win.contains("bottomSplitter")) {
            m_window.bottomSplitterState = QByteArray::fromBase64(win["bottomSplitter"].toString().toUtf8());
        }
    }

    // Build settings
    if (root.contains("build")) {
        QJsonObject b = root["build"].toObject();
        m_build.customBuildCommand = b.value("customBuildCommand").toString();
        m_build.customRunCommand = b.value("customRunCommand").toString();
        m_build.cmakeGenerator = b.value("cmakeGenerator").toString(m_build.cmakeGenerator);
        m_build.buildType = b.value("buildType").toString(m_build.buildType);
        m_build.autoSaveBeforeBuild = b.value("autoSaveBeforeBuild").toBool(m_build.autoSaveBeforeBuild);
    }

    // Session settings
    m_lastProjectPath = root.value("lastProjectPath").toString();
    m_activeTheme = root.value("activeTheme").toString(m_activeTheme);
    m_activeFile = root.value("activeFile").toString();

    m_recentProjects.clear();
    for (const auto& item : root.value("recentProjects").toArray()) {
        m_recentProjects.append(item.toString());
    }

    m_openFiles.clear();
    for (const auto& item : root.value("openFiles").toArray()) {
        m_openFiles.append(item.toString());
    }
}

void Config::save() {
    QJsonObject root;

    // Editor settings
    QJsonObject ed;
    ed["fontFamily"] = m_editor.fontFamily;
    ed["fontSize"] = m_editor.fontSize;
    ed["tabWidth"] = m_editor.tabWidth;
    ed["useSpacesForTabs"] = m_editor.useSpacesForTabs;
    ed["wordWrap"] = m_editor.wordWrap;
    ed["showLineNumbers"] = m_editor.showLineNumbers;
    ed["highlightCurrentLine"] = m_editor.highlightCurrentLine;
    ed["autoCloseBrackets"] = m_editor.autoCloseBrackets;
    ed["showWhitespace"] = m_editor.showWhitespace;
    ed["trimTrailingWhitespaceOnSave"] = m_editor.trimTrailingWhitespaceOnSave;
    ed["insertFinalNewline"] = m_editor.insertFinalNewline;
    root["editor"] = ed;

    // Window settings
    QJsonObject win;
    win["sidebarVisible"] = m_window.sidebarVisible;
    win["bottomPanelVisible"] = m_window.bottomPanelVisible;
    win["activeBottomTab"] = m_window.activeBottomTab;
    win["geometry"] = QString::fromUtf8(m_window.geometry.toBase64());
    win["state"] = QString::fromUtf8(m_window.state.toBase64());
    win["mainSplitter"] = QString::fromUtf8(m_window.mainSplitterState.toBase64());
    win["bottomSplitter"] = QString::fromUtf8(m_window.bottomSplitterState.toBase64());
    root["window"] = win;

    // Build settings
    QJsonObject b;
    b["customBuildCommand"] = m_build.customBuildCommand;
    b["customRunCommand"] = m_build.customRunCommand;
    b["cmakeGenerator"] = m_build.cmakeGenerator;
    b["buildType"] = m_build.buildType;
    b["autoSaveBeforeBuild"] = m_build.autoSaveBeforeBuild;
    root["build"] = b;

    // Session settings
    root["lastProjectPath"] = m_lastProjectPath;
    root["activeTheme"] = m_activeTheme;
    root["activeFile"] = m_activeFile;

    QJsonArray recents;
    for (const auto& p : m_recentProjects) {
        recents.append(p);
    }
    root["recentProjects"] = recents;

    QJsonArray files;
    for (const auto& f : m_openFiles) {
        files.append(f);
    }
    root["openFiles"] = files;

    QFile file(configFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QJsonDocument doc(root);
        file.write(doc.toJson(QJsonDocument::Indented));
    }
}

} // namespace liteforge::core
