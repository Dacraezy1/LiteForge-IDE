#include "liteforge/core/Document.hpp"
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QSaveFile>

namespace liteforge::core {

Document::Document(const QString& filePath, QObject* parent)
    : QObject(parent), m_filePath(filePath) {
    if (!m_filePath.isEmpty()) {
        m_language = detectLanguage(m_filePath);
        QFileInfo fi(m_filePath);
        if (fi.exists()) {
            m_lastModifiedOnDisk = fi.lastModified();
        }
    }
}

void Document::setFilePath(const QString& path) {
    if (m_filePath != path) {
        m_filePath = path;
        m_language = detectLanguage(path);
        QFileInfo fi(path);
        if (fi.exists()) {
            m_lastModifiedOnDisk = fi.lastModified();
        }
        emit filePathChanged(m_filePath);
        emit languageChanged(m_language);
    }
}

QString Document::fileName() const {
    if (m_filePath.isEmpty()) {
        return "Untitled";
    }
    return QFileInfo(m_filePath).fileName();
}

QString Document::fileDirectory() const {
    if (m_filePath.isEmpty()) {
        return QString();
    }
    return QFileInfo(m_filePath).absolutePath();
}

void Document::setModified(bool modified) {
    if (m_isModified != modified) {
        m_isModified = modified;
        emit modifiedChanged(modified);
    }
}

void Document::setLanguage(Language lang) {
    if (m_language != lang) {
        m_language = lang;
        emit languageChanged(lang);
    }
}

QString Document::languageName() const {
    switch (m_language) {
    case Language::Cpp: return "C++";
    case Language::C: return "C";
    case Language::Header: return "C/C++ Header";
    case Language::Python: return "Python";
    case Language::Rust: return "Rust";
    case Language::JavaScript: return "JavaScript";
    case Language::TypeScript: return "TypeScript";
    case Language::Json: return "JSON";
    case Language::Markdown: return "Markdown";
    case Language::Html: return "HTML";
    case Language::Css: return "CSS";
    case Language::Shell: return "Shell Script";
    case Language::CMake: return "CMake";
    case Language::Makefile: return "Makefile";
    case Language::PlainText:
    default:
        return "Plain Text";
    }
}

void Document::refreshDiskTimestamp() {
    if (m_filePath.isEmpty()) return;
    const QFileInfo fi(m_filePath);
    if (fi.exists()) m_lastModifiedOnDisk = fi.lastModified();
}

bool Document::checkDiskModified() const {
    if (m_filePath.isEmpty()) return false;
    QFileInfo fi(m_filePath);
    if (!fi.exists()) return false;
    return fi.lastModified() > m_lastModifiedOnDisk;
}

Language Document::detectLanguage(const QString& filePath) {
    QFileInfo fi(filePath);
    QString ext = fi.suffix().toLower();
    QString baseName = fi.fileName().toLower();

    if (baseName == "cmakelists.txt" || ext == "cmake") {
        return Language::CMake;
    }
    if (baseName == "makefile" || baseName == "gnumakefile") {
        return Language::Makefile;
    }

    if (ext == "cpp" || ext == "cxx" || ext == "cc" || ext == "c++") {
        return Language::Cpp;
    }
    if (ext == "c") {
        return Language::C;
    }
    if (ext == "h" || ext == "hpp" || ext == "hxx" || ext == "hh") {
        return Language::Header;
    }
    if (ext == "py" || ext == "pyw") {
        return Language::Python;
    }
    if (ext == "rs") {
        return Language::Rust;
    }
    if (ext == "js" || ext == "mjs" || ext == "cjs") {
        return Language::JavaScript;
    }
    if (ext == "ts") {
        return Language::TypeScript;
    }
    if (ext == "json") {
        return Language::Json;
    }
    if (ext == "md" || ext == "markdown") {
        return Language::Markdown;
    }
    if (ext == "html" || ext == "htm") {
        return Language::Html;
    }
    if (ext == "css" || ext == "scss" || ext == "sass") {
        return Language::Css;
    }
    if (ext == "sh" || ext == "bash" || ext == "zsh") {
        return Language::Shell;
    }

    return Language::PlainText;
}

} // namespace liteforge::core
