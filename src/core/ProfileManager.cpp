#include "liteforge/core/ProfileManager.hpp"
#include "liteforge/core/FileUtils.hpp"
#include <QJsonDocument>
#include <QJsonArray>

namespace liteforge::core {

QJsonObject WorkspaceProfile::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["theme"] = theme;
    obj["lspExecutable"] = lspExecutable;
    QJsonArray argsArr;
    for (const QString& a : lspArgs) argsArr.append(a);
    obj["lspArgs"] = argsArr;
    obj["buildCommand"] = buildCommand;
    obj["runCommand"] = runCommand;
    obj["tabWidth"] = tabWidth;
    obj["useSpaces"] = useSpaces;
    obj["wordWrap"] = wordWrap;
    obj["minimapEnabled"] = minimapEnabled;
    obj["showTerminal"] = showTerminal;
    return obj;
}

WorkspaceProfile WorkspaceProfile::fromJson(const QJsonObject& json) {
    WorkspaceProfile p;
    p.name = json["name"].toString();
    p.theme = json["theme"].toString("LiteForge Obsidian");
    p.lspExecutable = json["lspExecutable"].toString();
    if (json.contains("lspArgs") && json["lspArgs"].isArray()) {
        for (const auto& val : json["lspArgs"].toArray()) {
            p.lspArgs.append(val.toString());
        }
    }
    p.buildCommand = json["buildCommand"].toString();
    p.runCommand = json["runCommand"].toString();
    p.tabWidth = json["tabWidth"].toInt(4);
    p.useSpaces = json["useSpaces"].toBool(true);
    p.wordWrap = json["wordWrap"].toBool(false);
    p.minimapEnabled = json["minimapEnabled"].toBool(false);
    p.showTerminal = json["showTerminal"].toBool(true);
    return p;
}

ProfileManager& ProfileManager::instance() {
    static ProfileManager s_instance;
    return s_instance;
}

ProfileManager::ProfileManager() {
    loadDefaults();
}

void ProfileManager::loadDefaults() {
    // 1. C/C++ Development
    WorkspaceProfile cpp;
    cpp.name = "C/C++ Development";
    cpp.theme = "LiteForge Obsidian";
    cpp.lspExecutable = "clangd";
    cpp.buildCommand = "cmake --build build --parallel";
    cpp.runCommand = "./build/app";
    cpp.tabWidth = 4;
    cpp.useSpaces = true;
    cpp.wordWrap = false;
    cpp.minimapEnabled = false;
    cpp.showTerminal = true;
    m_profiles[cpp.name] = cpp;

    // 2. Python Development
    WorkspaceProfile py;
    py.name = "Python Development";
    py.theme = "One Dark Pro";
    py.lspExecutable = "pylsp";
    py.buildCommand = "python3 -m py_compile main.py";
    py.runCommand = "python3 main.py";
    py.tabWidth = 4;
    py.useSpaces = true;
    py.wordWrap = false;
    py.minimapEnabled = false;
    py.showTerminal = true;
    m_profiles[py.name] = py;

    // 3. Rust Development
    WorkspaceProfile rust;
    rust.name = "Rust Development";
    rust.theme = "Tokyo Night";
    rust.lspExecutable = "rust-analyzer";
    rust.buildCommand = "cargo build";
    rust.runCommand = "cargo run";
    rust.tabWidth = 4;
    rust.useSpaces = true;
    rust.wordWrap = false;
    rust.minimapEnabled = false;
    rust.showTerminal = true;
    m_profiles[rust.name] = rust;

    // 4. Web Development
    WorkspaceProfile web;
    web.name = "Web Development";
    web.theme = "Cyberpunk Neon";
    web.buildCommand = "";
    web.runCommand = "xdg-open index.html";
    web.tabWidth = 2;
    web.useSpaces = true;
    web.wordWrap = true;
    web.minimapEnabled = false;
    web.showTerminal = true;
    m_profiles[web.name] = web;

    // 5. Minimal Writing
    WorkspaceProfile writing;
    writing.name = "Minimal Writing";
    writing.theme = "Monokai Pro";
    writing.buildCommand = "";
    writing.runCommand = "";
    writing.tabWidth = 4;
    writing.useSpaces = true;
    writing.wordWrap = true;
    writing.minimapEnabled = false;
    writing.showTerminal = false;
    m_profiles[writing.name] = writing;

    m_currentProfile = cpp;
}

QStringList ProfileManager::availableProfiles() const {
    QStringList names;
    for (const auto& [name, _] : m_profiles) {
        names.append(name);
    }
    return names;
}

const WorkspaceProfile& ProfileManager::profile(const QString& name) const {
    auto it = m_profiles.find(name);
    if (it != m_profiles.end()) {
        return it->second;
    }
    return m_currentProfile;
}

void ProfileManager::setCurrentProfile(const QString& name) {
    auto it = m_profiles.find(name);
    if (it != m_profiles.end()) {
        m_currentProfile = it->second;
    }
}

void ProfileManager::saveCustomProfile(const WorkspaceProfile& prof) {
    m_profiles[prof.name] = prof;
}

bool ProfileManager::exportProfile(const QString& name, const QString& filePath) {
    auto it = m_profiles.find(name);
    if (it == m_profiles.end()) return false;

    QJsonObject json = it->second.toJson();
    QJsonDocument doc(json);
    return FileUtils::writeFileUtf8(filePath, QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
}

bool ProfileManager::importProfile(const QString& filePath) {
    bool ok = false;
    QString content = FileUtils::readFileUtf8(filePath, &ok);
    if (!ok) return false;

    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (!doc.isObject()) return false;

    WorkspaceProfile prof = WorkspaceProfile::fromJson(doc.object());
    if (prof.name.isEmpty()) return false;

    m_profiles[prof.name] = prof;
    return true;
}

} // namespace liteforge::core
