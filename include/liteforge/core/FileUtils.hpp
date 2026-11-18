#pragma once

#include <QString>
#include <QStringList>
#include <vector>

namespace liteforge::core {

struct FuzzyMatchResult {
    bool matched{false};
    int score{0};
    std::vector<int> matchPositions;
};

class FileUtils {
public:
    // Sublime-style fuzzy match scoring
    static FuzzyMatchResult fuzzyMatch(const QString& pattern, const QString& text);

    static QString formatFileSize(qint64 bytes);

    static bool isBinaryFile(const QString& filePath);

    static QString readFileUtf8(const QString& filePath, bool* ok = nullptr);

    static bool writeFileUtf8(const QString& filePath, const QString& content);

    static QStringList findFiles(const QString& rootDir, 
                                 const QStringList& filters = QStringList(),
                                 const QStringList& ignoreDirs = QStringList{"build", ".git", ".cache", "node_modules", ".idea", ".vscode"},
                                 int maxFiles = 10000);

    static QString relativePath(const QString& rootDir, const QString& absolutePath);
};

} // namespace liteforge::core
