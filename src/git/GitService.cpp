#include "liteforge/git/GitService.hpp"
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

namespace liteforge::git {

GitService::GitService(QObject* parent)
    : QObject(parent) {
}

void GitService::setRepositoryPath(const QString& path) {
    m_repoPath = path;
    m_isRepo = false;
    m_currentBranch.clear();
    m_branches.clear();
    m_statusFiles.clear();

    if (m_repoPath.isEmpty()) {
        emit statusUpdated();
        return;
    }

    // Check if path contains .git folder or file (submodule/worktree)
    QFileInfo gitInfo(m_repoPath + "/.git");
    if (gitInfo.exists()) {
        m_isRepo = true;
        refreshBranches();
        refreshStatus();
    } else {
        // Try running git rev-parse --is-inside-work-tree
        executeGitAsync(QStringList{"rev-parse", "--is-inside-work-tree"}, [this](int exitCode, const QString& stdoutStr, const QString&) {
            m_isRepo = (exitCode == 0 && stdoutStr.trimmed() == "true");
            if (m_isRepo) {
                refreshBranches();
                refreshStatus();
            } else {
                emit statusUpdated();
            }
        });
    }
}

void GitService::executeGitAsync(const QStringList& args, std::function<void(int, const QString&, const QString&)> callback) {
    if (m_repoPath.isEmpty()) {
        if (callback) callback(-1, QString(), "Repository path not set");
        return;
    }

    auto* process = new QProcess(this);
    process->setWorkingDirectory(m_repoPath);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [process, callback](int exitCode, QProcess::ExitStatus) {
        QString out = QString::fromUtf8(process->readAllStandardOutput());
        QString err = QString::fromUtf8(process->readAllStandardError());
        if (callback) {
            callback(exitCode, out, err);
        }
        process->deleteLater();
    });

    process->start("git", args);
}

void GitService::refreshBranches() {
    if (!m_isRepo) return;

    executeGitAsync(QStringList{"branch", "--list", "--no-color"}, [this](int exitCode, const QString& out, const QString&) {
        if (exitCode == 0) {
            parseBranchesOutput(out);
        }
    });
}

void GitService::parseBranchesOutput(const QString& output) {
    m_branches.clear();
    QString current;

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith('*')) {
            current = trimmed.mid(1).trimmed();
            m_branches.append(current);
        } else if (!trimmed.isEmpty()) {
            m_branches.append(trimmed);
        }
    }

    if (current != m_currentBranch) {
        m_currentBranch = current;
        emit branchChanged(m_currentBranch);
    }
}

void GitService::refreshStatus() {
    if (!m_isRepo) return;

    executeGitAsync(QStringList{"status", "--porcelain=v1", "-uall"}, [this](int exitCode, const QString& out, const QString&) {
        if (exitCode == 0) {
            parseStatusOutput(out);
        }
    });
}

void GitService::parseStatusOutput(const QString& output) {
    m_statusFiles.clear();

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        if (line.length() < 3) continue;

        QChar indexStatus = line[0];
        QChar workTreeStatus = line[1];
        QString filePath = line.mid(3).trimmed();

        // Handle quoted filenames
        if (filePath.startsWith('"') && filePath.endsWith('"')) {
            filePath = filePath.mid(1, filePath.length() - 2);
        }

        // Staged changes
        if (indexStatus != ' ' && indexStatus != '?') {
            GitFileStatus st;
            st.filePath = filePath;
            st.isStaged = true;

            switch (indexStatus.toLatin1()) {
            case 'M': st.status = FileGitStatus::Modified; break;
            case 'A': st.status = FileGitStatus::Added; break;
            case 'D': st.status = FileGitStatus::Deleted; break;
            case 'R': st.status = FileGitStatus::Renamed; break;
            default:  st.status = FileGitStatus::Modified; break;
            }
            m_statusFiles.push_back(st);
        }

        // Unstaged or untracked changes
        if (workTreeStatus != ' ') {
            GitFileStatus st;
            st.filePath = filePath;
            st.isStaged = false;

            if (indexStatus == '?' && workTreeStatus == '?') {
                st.status = FileGitStatus::Untracked;
            } else {
                switch (workTreeStatus.toLatin1()) {
                case 'M': st.status = FileGitStatus::Modified; break;
                case 'D': st.status = FileGitStatus::Deleted; break;
                default:  st.status = FileGitStatus::Modified; break;
                }
            }
            m_statusFiles.push_back(st);
        }
    }

    emit statusUpdated();
}

void GitService::stageFile(const QString& relativePath, std::function<void(bool)> callback) {
    executeGitAsync(QStringList{"add", "--", relativePath}, [this, callback](int exitCode, const QString&, const QString&) {
        bool ok = (exitCode == 0);
        refreshStatus();
        if (callback) callback(ok);
    });
}

void GitService::unstageFile(const QString& relativePath, std::function<void(bool)> callback) {
    executeGitAsync(QStringList{"reset", "HEAD", "--", relativePath}, [this, callback](int exitCode, const QString&, const QString&) {
        bool ok = (exitCode == 0);
        refreshStatus();
        if (callback) callback(ok);
    });
}

void GitService::stageAll(std::function<void(bool)> callback) {
    executeGitAsync(QStringList{"add", "-A"}, [this, callback](int exitCode, const QString&, const QString&) {
        bool ok = (exitCode == 0);
        refreshStatus();
        if (callback) callback(ok);
    });
}

void GitService::unstageAll(std::function<void(bool)> callback) {
    executeGitAsync(QStringList{"reset", "HEAD"}, [this, callback](int exitCode, const QString&, const QString&) {
        bool ok = (exitCode == 0);
        refreshStatus();
        if (callback) callback(ok);
    });
}

void GitService::commit(const QString& message, std::function<void(bool, const QString&)> callback) {
    if (message.trimmed().isEmpty()) {
        if (callback) callback(false, "Commit message cannot be empty.");
        return;
    }

    executeGitAsync(QStringList{"commit", "-m", message}, [this, callback](int exitCode, const QString& out, const QString& err) {
        bool ok = (exitCode == 0);
        refreshStatus();
        refreshBranches();
        emit operationFinished("Commit", ok, ok ? out.trimmed() : err.trimmed());
        if (callback) callback(ok, ok ? out : err);
    });
}

void GitService::discardChanges(const QString& relativePath, std::function<void(bool)> callback) {
    // Check if file is untracked
    FileGitStatus st = getStatusForFile(relativePath);
    if (st == FileGitStatus::Untracked) {
        // Remove untracked file
        QFile::remove(m_repoPath + "/" + relativePath);
        refreshStatus();
        if (callback) callback(true);
        return;
    }

    // Restore modified file
    executeGitAsync(QStringList{"checkout", "--", relativePath}, [this, callback](int exitCode, const QString&, const QString&) {
        bool ok = (exitCode == 0);
        refreshStatus();
        if (callback) callback(ok);
    });
}

void GitService::switchBranch(const QString& branchName, std::function<void(bool, const QString&)> callback) {
    executeGitAsync(QStringList{"checkout", branchName}, [this, callback](int exitCode, const QString& out, const QString& err) {
        bool ok = (exitCode == 0);
        refreshBranches();
        refreshStatus();
        emit operationFinished("Checkout", ok, ok ? out.trimmed() : err.trimmed());
        if (callback) callback(ok, ok ? out : err);
    });
}

void GitService::createBranch(const QString& branchName, std::function<void(bool, const QString&)> callback) {
    executeGitAsync(QStringList{"checkout", "-b", branchName}, [this, callback](int exitCode, const QString& out, const QString& err) {
        bool ok = (exitCode == 0);
        refreshBranches();
        refreshStatus();
        emit operationFinished("Create Branch", ok, ok ? out.trimmed() : err.trimmed());
        if (callback) callback(ok, ok ? out : err);
    });
}

void GitService::pull(std::function<void(bool, const QString&)> callback) {
    executeGitAsync(QStringList{"pull"}, [this, callback](int exitCode, const QString& out, const QString& err) {
        bool ok = (exitCode == 0);
        refreshStatus();
        emit operationFinished("Pull", ok, ok ? out.trimmed() : err.trimmed());
        if (callback) callback(ok, ok ? out : err);
    });
}

void GitService::push(std::function<void(bool, const QString&)> callback) {
    executeGitAsync(QStringList{"push"}, [this, callback](int exitCode, const QString& out, const QString& err) {
        bool ok = (exitCode == 0);
        refreshStatus();
        emit operationFinished("Push", ok, ok ? out.trimmed() : err.trimmed());
        if (callback) callback(ok, ok ? out : err);
    });
}

void GitService::getFileDiff(const QString& relativePath, bool staged, std::function<void(const QString&)> callback) {
    QStringList args;
    if (staged) {
        args = QStringList{"diff", "--cached", "--no-color", "--", relativePath};
    } else {
        args = QStringList{"diff", "--no-color", "--", relativePath};
    }

    executeGitAsync(args, [callback](int, const QString& out, const QString&) {
        if (callback) callback(out);
    });
}

QString GitService::statusToString(FileGitStatus status) {
    switch (status) {
    case FileGitStatus::Modified: return "Modified";
    case FileGitStatus::Added: return "Added";
    case FileGitStatus::Deleted: return "Deleted";
    case FileGitStatus::Renamed: return "Renamed";
    case FileGitStatus::Untracked: return "Untracked";
    case FileGitStatus::Conflicted: return "Conflict";
    case FileGitStatus::Unmodified:
    default:
        return "Clean";
    }
}

FileGitStatus GitService::getStatusForFile(const QString& path) const {
    QString rel = path;
    if (path.startsWith(m_repoPath)) {
        rel = QDir(m_repoPath).relativeFilePath(path);
    }

    for (const auto& item : m_statusFiles) {
        if (item.filePath == rel) {
            return item.status;
        }
    }
    return FileGitStatus::Unmodified;
}

} // namespace liteforge::git
