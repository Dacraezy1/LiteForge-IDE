#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <map>

namespace liteforge::core {

struct WorkspaceProfile {
    QString name;
    QString theme{"LiteForge Obsidian"};
    QString lspExecutable;
    QStringList lspArgs;
    QString buildCommand;
    QString runCommand;
    int tabWidth{4};
    bool useSpaces{true};
    bool wordWrap{false};
    bool minimapEnabled{false};
    bool showTerminal{true};

    QJsonObject toJson() const;
    static WorkspaceProfile fromJson(const QJsonObject& json);
};

class ProfileManager {
public:
    static ProfileManager& instance();

    ProfileManager(const ProfileManager&) = delete;
    ProfileManager& operator=(const ProfileManager&) = delete;

    QStringList availableProfiles() const;
    const WorkspaceProfile& profile(const QString& name) const;
    const WorkspaceProfile& currentProfile() const { return m_currentProfile; }

    void setCurrentProfile(const QString& name);
    void saveCustomProfile(const WorkspaceProfile& prof);

    bool exportProfile(const QString& name, const QString& filePath);
    bool importProfile(const QString& filePath);

private:
    ProfileManager();
    void loadDefaults();

    std::map<QString, WorkspaceProfile> m_profiles;
    WorkspaceProfile m_currentProfile;
};

} // namespace liteforge::core
