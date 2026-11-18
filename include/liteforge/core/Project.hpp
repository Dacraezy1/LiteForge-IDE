#pragma once

#include <QString>
#include <QStringList>
#include <QObject>
#include <memory>

namespace liteforge::core {

enum class ProjectBuildType {
    None,
    CMake,
    Makefile,
    Cargo,
    Custom
};

class Project : public QObject {
    Q_OBJECT

public:
    explicit Project(QObject* parent = nullptr);
    ~Project() override = default;

    bool open(const QString& rootPath);
    void close();

    bool isOpen() const { return !m_rootPath.isEmpty(); }
    QString rootPath() const { return m_rootPath; }
    QString projectName() const;

    QString gitBranch() const { return m_gitBranch; }
    void updateGitBranch();

    ProjectBuildType detectedBuildType() const { return m_buildType; }
    QString buildTypeString() const;

    // Fast indexed file list for fuzzy finder
    QStringList projectFiles() const { return m_projectFiles; }
    void refreshProjectFiles();

signals:
    void projectOpened(const QString& rootPath);
    void projectClosed();
    void gitBranchChanged(const QString& branch);
    void filesRefreshed();

private:
    void detectBuildType();

    QString m_rootPath;
    QString m_gitBranch;
    ProjectBuildType m_buildType{ProjectBuildType::None};
    QStringList m_projectFiles;
};

} // namespace liteforge::core
