#pragma once

#include <QString>
#include <QStringList>
#include <QObject>
#include <QProcess>
#include <vector>
#include <functional>

namespace liteforge::git {

enum class FileGitStatus {
    Unmodified,
    Modified,
    Added,
    Deleted,
    Renamed,
    Untracked,
    Conflicted
};

struct GitFileStatus {
    QString filePath;
    FileGitStatus status{FileGitStatus::Unmodified};
    bool isStaged{false};
};

struct GitDiffLine {
    enum class Type { Context, Addition, Deletion, Header };
    Type type{Type::Context};
    QString text;
    int oldLineNumber{-1};
    int newLineNumber{-1};
};

struct GitDiffHunk {
    QString header;
    std::vector<GitDiffLine> lines;
};

class GitService : public QObject {
    Q_OBJECT

public:
    explicit GitService(QObject* parent = nullptr);
    ~GitService() override = default;

    void setRepositoryPath(const QString& path);
    QString repositoryPath() const { return m_repoPath; }
    bool isRepository() const { return m_isRepo; }

    QString currentBranch() const { return m_currentBranch; }
    QStringList availableBranches() const { return m_branches; }
    std::vector<GitFileStatus> statusFiles() const { return m_statusFiles; }

    // Core async operations
    void refreshStatus();
    void refreshBranches();
    void stageFile(const QString& relativePath, std::function<void(bool)> callback = nullptr);
    void unstageFile(const QString& relativePath, std::function<void(bool)> callback = nullptr);
    void stageAll(std::function<void(bool)> callback = nullptr);
    void unstageAll(std::function<void(bool)> callback = nullptr);
    void commit(const QString& message, std::function<void(bool, const QString&)> callback = nullptr);
    void discardChanges(const QString& relativePath, std::function<void(bool)> callback = nullptr);
    void switchBranch(const QString& branchName, std::function<void(bool, const QString&)> callback = nullptr);
    void createBranch(const QString& branchName, std::function<void(bool, const QString&)> callback = nullptr);
    void pull(std::function<void(bool, const QString&)> callback = nullptr);
    void push(std::function<void(bool, const QString&)> callback = nullptr);

    // Diff inspection
    void getFileDiff(const QString& relativePath, bool staged, std::function<void(const QString& diffText)> callback);

    // Helpers
    static QString statusToString(FileGitStatus status);
    FileGitStatus getStatusForFile(const QString& absoluteOrRelativePath) const;

signals:
    void statusUpdated();
    void branchChanged(const QString& branch);
    void operationFinished(const QString& operation, bool success, const QString& message);

private:
    void executeGitAsync(const QStringList& args, std::function<void(int exitCode, const QString& stdoutStr, const QString& stderrStr)> callback);
    void parseStatusOutput(const QString& output);
    void parseBranchesOutput(const QString& output);

    QString m_repoPath;
    bool m_isRepo{false};
    QString m_currentBranch;
    QStringList m_branches;
    std::vector<GitFileStatus> m_statusFiles;
};

} // namespace liteforge::git
