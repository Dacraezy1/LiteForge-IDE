#include "liteforge/core/FileUtils.hpp"
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <algorithm>
#include <cctype>

namespace liteforge::core {

FuzzyMatchResult FileUtils::fuzzyMatch(const QString& pattern, const QString& text) {
    FuzzyMatchResult result;
    if (pattern.isEmpty()) {
        result.matched = true;
        result.score = 0;
        return result;
    }

    if (text.isEmpty() || pattern.length() > text.length()) {
        result.matched = false;
        return result;
    }

    int pIdx = 0;
    int prevMatchedIdx = -2;
    int score = 0;

    for (int tIdx = 0; tIdx < text.length() && pIdx < pattern.length(); ++tIdx) {
        QChar pc = pattern[pIdx].toLower();
        QChar tc = text[tIdx].toLower();

        if (pc == tc) {
            result.matchPositions.push_back(tIdx);

            // Scoring bonuses
            int matchScore = 10;

            // Consecutive match bonus
            if (tIdx == prevMatchedIdx + 1) {
                matchScore += 20;
            }

            // Word boundary bonuses (after '/', '_', '-', or camelCase)
            if (tIdx == 0) {
                matchScore += 30; // Prefix match bonus
            } else {
                QChar prevChar = text[tIdx - 1];
                if (prevChar == '/' || prevChar == '\\') {
                    matchScore += 35; // Filename / segment start bonus
                } else if (prevChar == '_' || prevChar == '-' || prevChar == '.') {
                    matchScore += 25;
                } else if (prevChar.isLower() && text[tIdx].isUpper()) {
                    matchScore += 20; // CamelCase bonus
                }
            }

            // Exact case match bonus
            if (pattern[pIdx] == text[tIdx]) {
                matchScore += 5;
            }

            score += matchScore;
            prevMatchedIdx = tIdx;
            ++pIdx;
        }
    }

    if (pIdx == pattern.length()) {
        result.matched = true;
        // Penalize long text to prefer shorter exact matches
        result.score = score - (text.length() - pattern.length());
    } else {
        result.matched = false;
        result.matchPositions.clear();
    }

    return result;
}

QString FileUtils::formatFileSize(qint64 bytes) {
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    }
    double kb = bytes / 1024.0;
    if (kb < 1024.0) {
        return QString("%1 KB").arg(kb, 0, 'f', 1);
    }
    double mb = kb / 1024.0;
    return QString("%1 MB").arg(mb, 0, 'f', 1);
}

bool FileUtils::isBinaryFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray buffer = file.read(1024);
    for (char c : buffer) {
        if (c == '\0') {
            return true;
        }
    }
    return false;
}

QString FileUtils::readFileUtf8(const QString& filePath, bool* ok) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (ok) *ok = false;
        return QString();
    }
    QTextStream in(&file);
    QString content = in.readAll();
    if (ok) *ok = true;
    return content;
}

bool FileUtils::writeFileUtf8(const QString& filePath, const QString& content) {
    return writeFileText(filePath, content, false, false);
}

QString FileUtils::readFileText(const QString& filePath, bool* ok, bool* hadCrlf, bool* hadBom) {
    if (ok) *ok = false;
    if (hadCrlf) *hadCrlf = false;
    if (hadBom) *hadBom = false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return QString();
    QByteArray data = file.readAll();
    file.close();

    bool bom = false;
    if (data.size() >= 3 && static_cast<uchar>(data[0]) == 0xEF && static_cast<uchar>(data[1]) == 0xBB &&
        static_cast<uchar>(data[2]) == 0xBF) {
        bom = true;
        data.remove(0, 3);
    }
    const bool crlf = data.contains("\r\n");
    if (crlf) data.replace("\r\n", "\n");

    QString text = QString::fromUtf8(data);
    if (ok) *ok = true;
    if (hadCrlf) *hadCrlf = crlf;
    if (hadBom) *hadBom = bom;
    return text;
}

bool FileUtils::writeFileText(const QString& filePath, const QString& content, bool crlf, bool bom) {
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    QByteArray data = content.toUtf8();
    if (crlf) data.replace("\n", "\r\n");
    if (bom) file.write("\xEF\xBB\xBF", 3);
    file.write(data);
    return file.commit();
}

QStringList FileUtils::findFiles(const QString& rootDir, 
                                 const QStringList& filters,
                                 const QStringList& ignoreDirs,
                                 int maxFiles) {
    QStringList result;
    if (rootDir.isEmpty()) return result;

    QDir root(rootDir);
    if (!root.exists()) return result;

    QDirIterator it(rootDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext() && result.size() < maxFiles) {
        QString path = it.next();

        // Check if path contains ignored directories
        bool shouldIgnore = false;
        for (const QString& ign : ignoreDirs) {
            QString ignPattern = "/" + ign + "/";
            if (path.contains(ignPattern) || path.endsWith("/" + ign)) {
                shouldIgnore = true;
                break;
            }
        }
        if (shouldIgnore) continue;

        if (!filters.isEmpty()) {
            bool matchesFilter = false;
            for (const QString& f : filters) {
                if (QDir::match(f, path)) {
                    matchesFilter = true;
                    break;
                }
            }
            if (!matchesFilter) continue;
        }

        result.append(path);
    }

    return result;
}

QString FileUtils::relativePath(const QString& rootDir, const QString& absolutePath) {
    QDir dir(rootDir);
    return dir.relativeFilePath(absolutePath);
}

QStringList FileUtils::parseGitIgnore(const QString& gitIgnorePath) {
    QStringList rules;
    QFile file(gitIgnorePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return rules;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        rules.append(line);
    }
    return rules;
}

bool FileUtils::isIgnoredByRule(const QString& relPath, const QStringList& rules) {
    for (const QString& rule : rules) {
        QString cleanRule = rule;
        bool dirOnly = cleanRule.endsWith('/');
        if (dirOnly) {
            cleanRule.chop(1);
        }

        // Exact match
        if (relPath == cleanRule || relPath.startsWith(cleanRule + "/")) {
            return true;
        }

        // Wildcard match on file name or path segment
        QFileInfo fi(relPath);
        if (QDir::match(cleanRule, fi.fileName()) || QDir::match(cleanRule, relPath)) {
            return true;
        }
    }
    return false;
}

} // namespace liteforge::core
