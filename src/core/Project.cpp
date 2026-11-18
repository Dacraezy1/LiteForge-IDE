#include "liteforge/core/Project.hpp"
#include "liteforge/core/FileUtils.hpp"
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QThread>

namespace liteforge::core {

Project::Project(QObject* parent)
    : QObject(parent) {}

bool Project::open(const QString& rootPath) {
    QDir dir(rootPath);
    if (!dir.exists()) {
        return false;
    }

    m_rootPath = dir.absolutePath();
    detectBuildType();
    updateGitBranch();
    refreshProjectFiles();

    emit projectOpened(m_rootPath);
    return true;
}

void Project::close() {
    if (m_rootPath.isEmpty()) return;
    m_rootPath.clear();
    m_gitBranch.clear();
    m_buildType = ProjectBuildType::None;
    m_projectFiles.clear();

    emit projectClosed();
}

QString Project::projectName() const {
    if (m_rootPath.isEmpty()) {
        return QString();
    }
    return QFileInfo(m_rootPath).fileName();
}

void Project::updateGitBranch() {
    m_gitBranch.clear();
    if (m_rootPath.isEmpty()) return;

    QString gitHeadPath = m_rootPath + "/.git/HEAD";
    QFile file(gitHeadPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString line = in.readLine().trimmed();
        if (line.startsWith("ref: refs/heads/")) {
            m_gitBranch = line.mid(16);
        } else if (!line.isEmpty()) {
            m_gitBranch = line.left(7); // Detached HEAD SHA
        }
    }

    emit gitBranchChanged(m_gitBranch);
}

void Project::detectBuildType() {
    m_buildType = ProjectBuildType::None;
    if (m_rootPath.isEmpty()) return;

    if (QFile::exists(m_rootPath + "/CMakeLists.txt")) {
        m_buildType = ProjectBuildType::CMake;
    } else if (QFile::exists(m_rootPath + "/Makefile") || QFile::exists(m_rootPath + "/GNUmakefile")) {
        m_buildType = ProjectBuildType::Makefile;
    } else if (QFile::exists(m_rootPath + "/Cargo.toml")) {
        m_buildType = ProjectBuildType::Cargo;
    }
}

QString Project::buildTypeString() const {
    switch (m_buildType) {
    case ProjectBuildType::CMake: return "CMake";
    case ProjectBuildType::Makefile: return "Make";
    case ProjectBuildType::Cargo: return "Cargo";
    case ProjectBuildType::Custom: return "Custom";
    case ProjectBuildType::None:
    default:
        return "None";
    }
}

void Project::refreshProjectFiles() {
    if (m_rootPath.isEmpty()) {
        m_projectFiles.clear();
        emit filesRefreshed();
        return;
    }

    if (m_isScanning) return;
    m_isScanning = true;
    emit scanningChanged(true);

    QString root = m_rootPath;
    QStringList customIgnored = m_customIgnoredDirs;

    QThread* thread = QThread::create([this, root, customIgnored]() {
        QStringList defaultIgnore{".git", "build", "node_modules", ".cache", ".idea", ".vscode", "target", "bin", "obj"};
        defaultIgnore.append(customIgnored);

        QStringList gitIgnoreRules;
        QString gitIgnorePath = root + "/.gitignore";
        if (QFile::exists(gitIgnorePath)) {
            gitIgnoreRules = FileUtils::parseGitIgnore(gitIgnorePath);
        }

        QStringList scanned = FileUtils::findFiles(root, QStringList(), defaultIgnore, 25000);

        // Filter files using .gitignore rules
        QStringList finalFiles;
        finalFiles.reserve(scanned.size());
        for (const QString& f : scanned) {
            QString rel = FileUtils::relativePath(root, f);
            if (!FileUtils::isIgnoredByRule(rel, gitIgnoreRules)) {
                finalFiles.append(f);
            }
        }

        QMetaObject::invokeMethod(this, [this, finalFiles]() {
            m_projectFiles = finalFiles;
            m_isScanning = false;
            emit scanningChanged(false);
            emit filesRefreshed();
        }, Qt::QueuedConnection);
    });

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

} // namespace liteforge::core
