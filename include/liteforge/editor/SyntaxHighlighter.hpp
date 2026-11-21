#pragma once

#include "liteforge/core/Document.hpp"
#include "liteforge/core/Theme.hpp"
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <vector>

namespace liteforge::editor {

struct HighlightingRule {
    QRegularExpression pattern;
    QTextCharFormat format;
};

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument* parent = nullptr, core::Language lang = core::Language::PlainText);
    ~SyntaxHighlighter() override = default;

    void setLanguage(core::Language lang);
    core::Language language() const { return m_language; }

    void updateTheme(const core::ThemeColors& theme);

protected:
    void highlightBlock(const QString& text) override;

private:
    void setupRules();
    void addWords(const QStringList& words, const QTextCharFormat& fmt);
    void setupCppRules();
    void setupPythonRules();
    void setupRustRules();
    void setupJavaScriptRules();
    void setupJsonRules();
    void setupMarkdownRules();
    void setupCMakeRules();
    void setupShellRules();

    core::Language m_language{core::Language::PlainText};
    std::vector<HighlightingRule> m_rules;

    // Multi-line comment parsing
    QRegularExpression m_commentStartPattern;
    QRegularExpression m_commentEndPattern;
    QTextCharFormat m_multiLineCommentFormat;

    // Formats
    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_typeFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_singleLineCommentFormat;
    QTextCharFormat m_preprocessorFormat;
    QTextCharFormat m_operatorFormat;
    QTextCharFormat m_constantFormat;
};

} // namespace liteforge::editor
