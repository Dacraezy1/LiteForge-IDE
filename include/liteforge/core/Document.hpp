#pragma once

#include <QString>
#include <QDateTime>
#include <QObject>

namespace liteforge::core {

enum class Language {
    PlainText,
    Cpp,
    C,
    Header,
    Python,
    Rust,
    JavaScript,
    TypeScript,
    Json,
    Markdown,
    Html,
    Css,
    Shell,
    CMake,
    Makefile
};

enum class LineEnding {
    LF,
    CRLF
};

class Document : public QObject {
    Q_OBJECT

public:
    explicit Document(const QString& filePath = QString(), QObject* parent = nullptr);
    ~Document() override = default;

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString& path);

    QString fileName() const;
    QString fileDirectory() const;

    bool isModified() const { return m_isModified; }
    void setModified(bool modified);

    bool isUntitled() const { return m_filePath.isEmpty(); }

    Language language() const { return m_language; }
    void setLanguage(Language lang);
    QString languageName() const;

    LineEnding lineEnding() const { return m_lineEnding; }
    void setLineEnding(LineEnding le) { m_lineEnding = le; }
    QString lineEndingString() const { return m_lineEnding == LineEnding::LF ? "LF" : "CRLF"; }

    QString encoding() const { return m_encoding; }
    bool hasBom() const { return m_hasBom; }
    void setHasBom(bool bom) { m_hasBom = bom; }
    // Call after our own write so we don't mistake it for an external change.
    void refreshDiskTimestamp();

    bool loadFromFile(const QString& path);
    bool saveToFile(const QString& path = QString());

    QDateTime lastModifiedOnDisk() const { return m_lastModifiedOnDisk; }
    bool checkDiskModified() const;

    static Language detectLanguage(const QString& filePath);

signals:
    void modifiedChanged(bool modified);
    void filePathChanged(const QString& newPath);
    void languageChanged(Language lang);

private:
    QString m_filePath;
    bool m_isModified{false};
    Language m_language{Language::PlainText};
    LineEnding m_lineEnding{LineEnding::LF};
    QString m_encoding{"UTF-8"};
    bool m_hasBom{false};
    QDateTime m_lastModifiedOnDisk;
};

} // namespace liteforge::core
