#include "liteforge/editor/SyntaxHighlighter.hpp"
#include <QHash>
#include <QStringList>

namespace liteforge::editor {

namespace {

// Compiled patterns are shared between every editor/highlighter instance:
// QRegularExpression is implicitly shared, so N open C++ files cost one set of
// compiled PCRE programs instead of N.
QRegularExpression rx(const QString& pattern, QRegularExpression::PatternOptions opts = QRegularExpression::NoPatternOption) {
    static QHash<QString, QRegularExpression> cache;
    const QString key = QString::number(static_cast<int>(opts)) + QLatin1Char(':') + pattern;
    auto it = cache.constFind(key);
    if (it != cache.constEnd()) return *it;
    QRegularExpression re(pattern, opts);
    cache.insert(key, re);
    return re;
}

// Lines longer than this are only highlighted up to this many characters
// (minified JS / generated data would otherwise stall the UI).
constexpr int kMaxHighlightedLine = 4000;

} // namespace

// One alternation per keyword group instead of one regex per keyword:
// ~100 rules per C++ document become 2, and each line is scanned 50x fewer times.
void SyntaxHighlighter::addWords(const QStringList& words, const QTextCharFormat& fmt) {
    if (words.isEmpty()) return;
    HighlightingRule rule;
    rule.pattern = rx(QStringLiteral("\\b(?:") + words.join(QLatin1Char('|')) + QStringLiteral(")\\b"));
    rule.format = fmt;
    m_rules.push_back(rule);
}


SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent, core::Language lang)
    : QSyntaxHighlighter(parent), m_language(lang) {
    updateTheme(core::ThemeManager::instance().currentTheme());
}

void SyntaxHighlighter::setLanguage(core::Language lang) {
    if (m_language != lang) {
        m_language = lang;
        setupRules();
        rehighlight();
    }
}

void SyntaxHighlighter::updateTheme(const core::ThemeColors& theme) {
    m_keywordFormat.setForeground(theme.keyword);
    m_keywordFormat.setFontWeight(QFont::Bold);

    m_typeFormat.setForeground(theme.type);
    m_functionFormat.setForeground(theme.function);
    m_stringFormat.setForeground(theme.string);
    m_numberFormat.setForeground(theme.number);
    m_singleLineCommentFormat.setForeground(theme.comment);
    m_singleLineCommentFormat.setFontItalic(true);
    m_multiLineCommentFormat.setForeground(theme.comment);
    m_multiLineCommentFormat.setFontItalic(true);
    m_preprocessorFormat.setForeground(theme.preprocessor);
    m_operatorFormat.setForeground(theme.operatorColor);
    m_constantFormat.setForeground(theme.constant);

    setupRules();
    rehighlight();
}

void SyntaxHighlighter::setupRules() {
    m_rules.clear();
    m_commentStartPattern = QRegularExpression();
    m_commentEndPattern = QRegularExpression();

    switch (m_language) {
    case core::Language::Cpp:
    case core::Language::C:
    case core::Language::Header:
        setupCppRules();
        break;
    case core::Language::Python:
        setupPythonRules();
        break;
    case core::Language::Rust:
        setupRustRules();
        break;
    case core::Language::JavaScript:
    case core::Language::TypeScript:
        setupJavaScriptRules();
        break;
    case core::Language::Json:
        setupJsonRules();
        break;
    case core::Language::Markdown:
        setupMarkdownRules();
        break;
    case core::Language::CMake:
        setupCMakeRules();
        break;
    case core::Language::Shell:
    case core::Language::Makefile:
        setupShellRules();
        break;
    default:
        break;
    }
}

void SyntaxHighlighter::setupCppRules() {
    // Numbers
    HighlightingRule numRule;
    numRule.pattern = rx(QStringLiteral("\\b(0x[0-9a-fA-F]+|0b[01]+|[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?(f|F|u|U|l|L|ll|LL)?)\\b"));
    numRule.format = m_numberFormat;
    m_rules.push_back(numRule);

    // Preprocessor
    HighlightingRule prepRule;
    prepRule.pattern = rx(QStringLiteral("^[ \t]*#[a-zA-Z_]+"));
    prepRule.format = m_preprocessorFormat;
    m_rules.push_back(prepRule);

    // Modern C++ Keywords
    static const QStringList cppKeywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit",
        "atomic_noexcept", "auto", "bitand", "bitor", "bool", "break", "case", "catch",
        "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const",
        "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await",
        "co_return", "co_yield", "decltype", "default", "delete", "do", "double",
        "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float",
        "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace",
        "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq",
        "private", "protected", "public", "reflexpr", "register", "reinterpret_cast",
        "requires", "return", "short", "signed", "sizeof", "static", "static_assert",
        "static_cast", "struct", "switch", "synchronized", "template", "this",
        "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
        "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
        "while", "xor", "xor_eq", "override", "final"
    };

    addWords(cppKeywords, m_keywordFormat);

    // Common Types
    static const QStringList cppTypes = {
        "std", "string", "string_view", "vector", "map", "set", "unordered_map",
        "unordered_set", "array", "deque", "list", "queue", "stack", "span",
        "unique_ptr", "shared_ptr", "weak_ptr", "optional", "variant", "any",
        "tuple", "pair", "size_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "int8_t", "int16_t", "int32_t", "int64_t", "ptrdiff_t", "intptr_t", "uintptr_t",
        "QString", "QByteArray", "QVector", "QList", "QMap", "QWidget", "QObject"
    };

    addWords(cppTypes, m_typeFormat);

    // Functions: identifier followed by '('
    HighlightingRule fnRule;
    fnRule.pattern = rx(QStringLiteral("\\b[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()"));
    fnRule.format = m_functionFormat;
    m_rules.push_back(fnRule);

    // Strings
    HighlightingRule strRule;
    strRule.pattern = rx(QStringLiteral("\".*?\""));
    strRule.format = m_stringFormat;
    m_rules.push_back(strRule);

    // Characters
    HighlightingRule charRule;
    charRule.pattern = rx(QStringLiteral("'.*?'"));
    charRule.format = m_stringFormat;
    m_rules.push_back(charRule);

    // Single-line comment
    HighlightingRule commentRule;
    commentRule.pattern = rx(QStringLiteral("//[^\n]*"));
    commentRule.format = m_singleLineCommentFormat;
    m_rules.push_back(commentRule);

    // Multi-line comments
    m_commentStartPattern = rx(QStringLiteral("/\\*"));
    m_commentEndPattern = rx(QStringLiteral("\\*/"));
}

void SyntaxHighlighter::setupPythonRules() {
    // Numbers
    HighlightingRule numRule;
    numRule.pattern = rx(QStringLiteral("\\b([0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?|0x[0-9a-fA-F]+|0b[01]+)\\b"));
    numRule.format = m_numberFormat;
    m_rules.push_back(numRule);

    // Keywords
    static const QStringList pyKeywords = {
        "and", "as", "assert", "async", "await", "break", "class", "continue",
        "def", "del", "elif", "else", "except", "finally", "for", "from",
        "global", "if", "import", "in", "is", "lambda", "nonlocal", "not",
        "or", "pass", "raise", "return", "try", "while", "with", "yield",
        "True", "False", "None", "self"
    };

    addWords(pyKeywords, m_keywordFormat);

    // Builtins
    static const QStringList pyBuiltins = {
        "abs", "all", "any", "bin", "bool", "bytes", "dict", "enumerate", "filter",
        "float", "format", "int", "isinstance", "issubclass", "iter", "len",
        "list", "map", "max", "min", "next", "open", "print", "range", "repr",
        "reversed", "round", "set", "slice", "sorted", "str", "sum", "super",
        "tuple", "type", "zip"
    };

    addWords(pyBuiltins, m_typeFormat);

    // Decorators: @decorator
    HighlightingRule decoRule;
    decoRule.pattern = rx(QStringLiteral("@[a-zA-Z_][a-zA-Z0-9_\\.]*"));
    decoRule.format = m_preprocessorFormat;
    m_rules.push_back(decoRule);

    // Functions
    HighlightingRule fnRule;
    fnRule.pattern = rx(QStringLiteral("\\b[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()"));
    fnRule.format = m_functionFormat;
    m_rules.push_back(fnRule);

    // Strings
    HighlightingRule strRule;
    strRule.pattern = rx(QStringLiteral("(\"\"\"[^\"]*\"\"\"|'''[^']*'''|\"[^\"]*\"|'[^']*')"));
    strRule.format = m_stringFormat;
    m_rules.push_back(strRule);

    // Comments
    HighlightingRule commentRule;
    commentRule.pattern = rx(QStringLiteral("#[^\n]*"));
    commentRule.format = m_singleLineCommentFormat;
    m_rules.push_back(commentRule);
}

void SyntaxHighlighter::setupRustRules() {
    // Numbers
    HighlightingRule numRule;
    numRule.pattern = rx(QStringLiteral("\\b([0-9][0-9_]*(\\.[0-9][0-9_]*)?([eE][+-]?[0-9_]+)?([iuf](8|16|32|64|128|size))?|0x[0-9a-fA-F_]+|0b[01_]+)\\b"));
    numRule.format = m_numberFormat;
    m_rules.push_back(numRule);

    // Rust Keywords
    static const QStringList rustKeywords = {
        "as", "async", "await", "break", "const", "continue", "crate", "dyn",
        "else", "enum", "extern", "false", "fn", "for", "if", "impl", "in",
        "let", "loop", "match", "mod", "move", "mut", "pub", "ref", "return",
        "self", "Self", "static", "struct", "super", "trait", "true", "type",
        "unsafe", "use", "where", "while"
    };

    addWords(rustKeywords, m_keywordFormat);

    // Types
    static const QStringList rustTypes = {
        "bool", "char", "str", "u8", "u16", "u32", "u64", "u128", "usize",
        "i8", "i16", "i32", "i64", "i128", "isize", "f32", "f64",
        "Option", "Result", "Some", "None", "Ok", "Err", "Box", "Rc", "Arc", "Vec", "String"
    };

    addWords(rustTypes, m_typeFormat);

    // Macros: identifier!
    HighlightingRule macroRule;
    macroRule.pattern = rx(QStringLiteral("\\b[a-zA-Z_][a-zA-Z0-9_]*!"));
    macroRule.format = m_preprocessorFormat;
    m_rules.push_back(macroRule);

    // Strings
    HighlightingRule strRule;
    strRule.pattern = rx(QStringLiteral("\".*?\""));
    strRule.format = m_stringFormat;
    m_rules.push_back(strRule);

    // Comments
    HighlightingRule commentRule;
    commentRule.pattern = rx(QStringLiteral("//[^\n]*"));
    commentRule.format = m_singleLineCommentFormat;
    m_rules.push_back(commentRule);

    m_commentStartPattern = rx(QStringLiteral("/\\*"));
    m_commentEndPattern = rx(QStringLiteral("\\*/"));
}

void SyntaxHighlighter::setupJavaScriptRules() {
    // Numbers
    HighlightingRule numRule;
    numRule.pattern = rx(QStringLiteral("\\b[0-9]+(\\.[0-9]+)?\\b"));
    numRule.format = m_numberFormat;
    m_rules.push_back(numRule);

    static const QStringList jsKeywords = {
        "break", "case", "catch", "class", "const", "continue", "debugger",
        "default", "delete", "do", "else", "export", "extends", "finally",
        "for", "function", "if", "import", "in", "instanceof", "new", "return",
        "super", "switch", "this", "throw", "try", "typeof", "var", "void",
        "while", "with", "yield", "let", "static", "enum", "await", "async",
        "true", "false", "null", "undefined", "interface", "type", "declare"
    };

    addWords(jsKeywords, m_keywordFormat);

    // Functions
    HighlightingRule fnRule;
    fnRule.pattern = rx(QStringLiteral("\\b[a-zA-Z_$][a-zA-Z0-9_$]*(?=\\s*\\()"));
    fnRule.format = m_functionFormat;
    m_rules.push_back(fnRule);

    // Strings & Template Literals
    HighlightingRule strRule;
    strRule.pattern = rx(QStringLiteral("(\"[^\"]*\"|'[^']*'|`[^`]*`)"));
    strRule.format = m_stringFormat;
    m_rules.push_back(strRule);

    // Comments
    HighlightingRule commentRule;
    commentRule.pattern = rx(QStringLiteral("//[^\n]*"));
    commentRule.format = m_singleLineCommentFormat;
    m_rules.push_back(commentRule);

    m_commentStartPattern = rx(QStringLiteral("/\\*"));
    m_commentEndPattern = rx(QStringLiteral("\\*/"));
}

void SyntaxHighlighter::setupJsonRules() {
    // Keys
    HighlightingRule keyRule;
    keyRule.pattern = rx(QStringLiteral("\"[^\"]+\"\\s*:"));
    keyRule.format = m_keywordFormat;
    m_rules.push_back(keyRule);

    // Strings
    HighlightingRule strRule;
    strRule.pattern = rx(QStringLiteral(":\\s*\"[^\"]*\""));
    strRule.format = m_stringFormat;
    m_rules.push_back(strRule);

    // Numbers & Booleans
    HighlightingRule numRule;
    numRule.pattern = rx(QStringLiteral("\\b(-?[0-9]+(\\.[0-9]+)?([eE][+-]?[0-9]+)?|true|false|null)\\b"));
    numRule.format = m_numberFormat;
    m_rules.push_back(numRule);
}

void SyntaxHighlighter::setupMarkdownRules() {
    // Headers #
    HighlightingRule hRule;
    hRule.pattern = rx(QStringLiteral("^#{1,6}\\s+.*$"));
    hRule.format = m_keywordFormat;
    m_rules.push_back(hRule);

    // Bold / Italic
    HighlightingRule boldRule;
    boldRule.pattern = rx(QStringLiteral("(\\*\\*.*?\\*\\*|__.*?__)"));
    boldRule.format = m_constantFormat;
    m_rules.push_back(boldRule);

    // Inline Code
    HighlightingRule codeRule;
    codeRule.pattern = rx(QStringLiteral("`[^`]+`"));
    codeRule.format = m_typeFormat;
    m_rules.push_back(codeRule);

    // Links [text](url)
    HighlightingRule linkRule;
    linkRule.pattern = rx(QStringLiteral("\\[.*?\\]\\(.*?\\)"));
    linkRule.format = m_functionFormat;
    m_rules.push_back(linkRule);
}

void SyntaxHighlighter::setupCMakeRules() {
    HighlightingRule cmdRule;
    cmdRule.pattern = rx(QStringLiteral("\\b[a-zA-Z_][a-zA-Z0-9_]*(?=\\s*\\()"), QRegularExpression::CaseInsensitiveOption);
    cmdRule.format = m_keywordFormat;
    m_rules.push_back(cmdRule);

    HighlightingRule varRule;
    varRule.pattern = rx(QStringLiteral("\\$\\{[a-zA-Z0-9_]+\\}"));
    varRule.format = m_typeFormat;
    m_rules.push_back(varRule);

    HighlightingRule strRule;
    strRule.pattern = rx(QStringLiteral("\".*?\""));
    strRule.format = m_stringFormat;
    m_rules.push_back(strRule);

    HighlightingRule commentRule;
    commentRule.pattern = rx(QStringLiteral("#[^\n]*"));
    commentRule.format = m_singleLineCommentFormat;
    m_rules.push_back(commentRule);
}

void SyntaxHighlighter::setupShellRules() {
    HighlightingRule varRule;
    varRule.pattern = rx(QStringLiteral("\\$[a-zA-Z0-9_]+|\\$\\{[a-zA-Z0-9_]+\\}"));
    varRule.format = m_typeFormat;
    m_rules.push_back(varRule);

    static const QStringList shKeywords = {
        "if", "then", "else", "elif", "fi", "case", "esac", "for", "while",
        "until", "do", "done", "in", "function", "select", "return", "exit", "echo"
    };

    addWords(shKeywords, m_keywordFormat);

    HighlightingRule strRule;
    strRule.pattern = rx(QStringLiteral("(\"[^\"]*\"|'[^']*')"));
    strRule.format = m_stringFormat;
    m_rules.push_back(strRule);

    HighlightingRule commentRule;
    commentRule.pattern = rx(QStringLiteral("#[^\n]*"));
    commentRule.format = m_singleLineCommentFormat;
    m_rules.push_back(commentRule);
}

void SyntaxHighlighter::highlightBlock(const QString& fullText) {
    const bool clipped = fullText.size() > kMaxHighlightedLine;
    const QString text = clipped ? fullText.left(kMaxHighlightedLine) : fullText;

    for (const auto& rule : m_rules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            const QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // Handle multi-line comments
    if (m_commentStartPattern.pattern().isEmpty()) return;

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        startIndex = text.indexOf(m_commentStartPattern);
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch;
        const int endIndex = text.indexOf(m_commentEndPattern, startIndex, &endMatch);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }
        setFormat(startIndex, commentLength, m_multiLineCommentFormat);
        startIndex = text.indexOf(m_commentStartPattern, startIndex + commentLength);
    }
}

} // namespace liteforge::editor
