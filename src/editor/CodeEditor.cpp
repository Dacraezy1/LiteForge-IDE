#include "liteforge/editor/CodeEditor.hpp"
#include "liteforge/editor/LineNumberGutter.hpp"
#include "liteforge/editor/SyntaxHighlighter.hpp"
#include "liteforge/editor/FindReplaceWidget.hpp"
#include "liteforge/core/Config.hpp"

#include <QPainter>
#include <QTextBlock>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QFontDatabase>
#include <QScrollBar>

namespace liteforge::editor {

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent) {
    m_lineNumberGutter = new LineNumberGutter(this);
    m_highlighter = new SyntaxHighlighter(document());
    m_findReplaceWidget = new FindReplaceWidget(this, this);

    // Default editor config
    const auto& cfg = core::Config::instance().editor();
    m_baseFontSize = cfg.fontSize;
    m_tabWidth = cfg.tabWidth;
    m_useSpaces = cfg.useSpacesForTabs;
    m_autoCloseBrackets = cfg.autoCloseBrackets;
    m_highlightCurrentLine = cfg.highlightCurrentLine;

    updateFontSettings(cfg.fontFamily, cfg.fontSize);

    setLineWrapMode(cfg.wordWrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberGutterWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberGutter);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::highlightCurrentLineAndBrackets);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::onCursorPositionChanged);

    updateLineNumberGutterWidth(0);
    highlightCurrentLineAndBrackets();

    applyTheme(core::ThemeManager::instance().currentTheme());
}

CodeEditor::~CodeEditor() = default;

void CodeEditor::setDocumentModel(std::shared_ptr<core::Document> doc) {
    m_document = doc;
    if (m_document) {
        m_highlighter->setLanguage(m_document->language());
        connect(m_document.get(), &core::Document::languageChanged, this, [this](core::Language lang) {
            m_highlighter->setLanguage(lang);
        });
    }
}

void CodeEditor::applyTheme(const core::ThemeColors& theme) {
    m_highlighter->updateTheme(theme);

    QPalette p = palette();
    p.setColor(QPalette::Base, theme.editorBg);
    p.setColor(QPalette::Text, theme.textNormal);
    p.setColor(QPalette::Highlight, theme.selectionBg);
    p.setColor(QPalette::HighlightedText, theme.textSelected);
    setPalette(p);

    highlightCurrentLineAndBrackets();
}

void CodeEditor::updateFontSettings(const QString& family, int size) {
    m_baseFontSize = size;
    int currentSize = m_baseFontSize + m_currentZoomLevel;
    if (currentSize < 6) currentSize = 6;

    QFont font(family, currentSize);
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    setFont(font);

    // Configure tab stop distance
    QFontMetrics metrics(font);
    setTabStopDistance(m_tabWidth * metrics.horizontalAdvance(' '));
    updateLineNumberGutterWidth(0);
}

int CodeEditor::lineNumberGutterWidth() {
    int digits = 1;
    int maxLines = std::max(1, blockCount());
    while (maxLines >= 10) {
        maxLines /= 10;
        ++digits;
    }
    // Min 3 digits width + padding
    digits = std::max(digits, 3);
    int space = 18 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberGutterWidth(int /* newBlockCount */) {
    setViewportMargins(lineNumberGutterWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberGutter(const QRect& rect, int dy) {
    if (dy) {
        m_lineNumberGutter->scroll(0, dy);
    } else {
        m_lineNumberGutter->update(0, rect.y(), m_lineNumberGutter->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberGutterWidth(0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberGutter->setGeometry(QRect(cr.left(), cr.top(), lineNumberGutterWidth(), cr.height()));

    // Position Find/Replace widget at top right
    if (m_findReplaceWidget && m_findReplaceWidget->isVisible()) {
        int w = std::min(450, width() - 40);
        int h = m_findReplaceWidget->sizeHint().height();
        m_findReplaceWidget->setGeometry(width() - w - 20, 10, w, h);
        m_findReplaceWidget->raise();
    }
}

void CodeEditor::lineNumberGutterPaintEvent(QPaintEvent* event) {
    const auto& theme = core::ThemeManager::instance().currentTheme();
    QPainter painter(m_lineNumberGutter);
    painter.fillRect(event->rect(), theme.sidebarBg);

    // Right divider line
    painter.setPen(theme.border);
    painter.drawLine(m_lineNumberGutter->width() - 1, event->rect().top(),
                     m_lineNumberGutter->width() - 1, event->rect().bottom());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    int currentLine = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            if (blockNumber == currentLine) {
                painter.setPen(theme.lineNumberActive);
                QFont f = painter.font();
                f.setBold(true);
                painter.setFont(f);
            } else {
                painter.setPen(theme.lineNumberNormal);
                QFont f = painter.font();
                f.setBold(false);
                painter.setFont(f);
            }

            painter.drawText(0, top, m_lineNumberGutter->width() - 8, fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::paintEvent(QPaintEvent* event) {
    QPlainTextEdit::paintEvent(event);

    // Paint subtle indentation guide lines
    const auto& theme = core::ThemeManager::instance().currentTheme();
    QPainter painter(viewport());
    painter.setPen(QPen(theme.indentGuide, 1, Qt::DotLine));

    QFontMetrics fm = fontMetrics();
    int spaceWidth = fm.horizontalAdvance(' ');
    int indentPx = m_tabWidth * spaceWidth;

    QTextBlock block = firstVisibleBlock();
    while (block.isValid()) {
        QRectF rect = blockBoundingGeometry(block).translated(contentOffset());
        if (rect.top() > viewport()->rect().bottom()) break;

        if (block.isVisible() && rect.bottom() >= 0) {
            QString text = block.text();
            int spaces = 0;
            for (QChar c : text) {
                if (c == ' ') spaces++;
                else if (c == '\t') spaces += m_tabWidth;
                else break;
            }

            int guides = spaces / m_tabWidth;
            for (int i = 1; i <= guides; ++i) {
                int x = i * indentPx;
                painter.drawLine(x, rect.top(), x, rect.bottom());
            }
        }
        block = block.next();
    }
}

void CodeEditor::highlightCurrentLineAndBrackets() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly() && m_highlightCurrentLine) {
        QTextEdit::ExtraSelection selection;
        const auto& theme = core::ThemeManager::instance().currentTheme();
        selection.format.setBackground(theme.currentLineBg);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    // Bracket matching
    matchBrackets();

    setExtraSelections(extraSelections);
}

void CodeEditor::matchBrackets() {
    // Check if cursor is next to a bracket () [] {}
    QTextCursor cur = textCursor();
    int pos = cur.position();
    QString text = toPlainText();
    if (text.isEmpty() || pos < 0 || pos > text.length()) return;

    auto isBracket = [](QChar c) {
        return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}';
    };

    int bracketPos = -1;
    QChar bracketChar;

    if (pos < text.length() && isBracket(text[pos])) {
        bracketPos = pos;
        bracketChar = text[pos];
    } else if (pos > 0 && isBracket(text[pos - 1])) {
        bracketPos = pos - 1;
        bracketChar = text[pos - 1];
    }

    if (bracketPos == -1) return;

    QChar matchChar;
    bool forward = true;
    if (bracketChar == '(') { matchChar = ')'; forward = true; }
    else if (bracketChar == ')') { matchChar = '('; forward = false; }
    else if (bracketChar == '[') { matchChar = ']'; forward = true; }
    else if (bracketChar == ']') { matchChar = '['; forward = false; }
    else if (bracketChar == '{') { matchChar = '}'; forward = true; }
    else if (bracketChar == '}') { matchChar = '{'; forward = false; }

    int depth = 0;
    int matchPos = -1;

    if (forward) {
        for (int i = bracketPos + 1; i < text.length(); ++i) {
            if (text[i] == bracketChar) depth++;
            else if (text[i] == matchChar) {
                if (depth == 0) { matchPos = i; break; }
                depth--;
            }
        }
    } else {
        for (int i = bracketPos - 1; i >= 0; --i) {
            if (text[i] == bracketChar) depth++;
            else if (text[i] == matchChar) {
                if (depth == 0) { matchPos = i; break; }
                depth--;
            }
        }
    }

    if (matchPos != -1) {
        const auto& theme = core::ThemeManager::instance().currentTheme();
        QList<QTextEdit::ExtraSelection> extras = extraSelections();

        QTextEdit::ExtraSelection sel1;
        sel1.cursor = textCursor();
        sel1.cursor.setPosition(bracketPos);
        sel1.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        sel1.format.setBackground(theme.bracketMatchBg);
        extras.append(sel1);

        QTextEdit::ExtraSelection sel2;
        sel2.cursor = textCursor();
        sel2.cursor.setPosition(matchPos);
        sel2.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        sel2.format.setBackground(theme.bracketMatchBg);
        extras.append(sel2);

        setExtraSelections(extras);
    }
}

bool CodeEditor::handleAutoClosingBrackets(QKeyEvent* event) {
    if (!m_autoCloseBrackets) return false;

    QString text = event->text();
    if (text.isEmpty()) return false;
    QChar c = text[0];

    QTextCursor cur = textCursor();
    QString docText = toPlainText();
    int pos = cur.position();

    // Skip over closing bracket if typed directly in front of it
    if ((c == ')' || c == ']' || c == '}' || c == '"' || c == '\'') && pos < docText.length()) {
        if (docText[pos] == c) {
            cur.movePosition(QTextCursor::NextCharacter);
            setTextCursor(cur);
            return true;
        }
    }

    // Auto-close opening bracket or quote
    QChar closeChar;
    if (c == '(') closeChar = ')';
    else if (c == '[') closeChar = ']';
    else if (c == '{') closeChar = '}';
    else if (c == '"') closeChar = '"';
    else if (c == '\'') closeChar = '\'';
    else return false;

    if (cur.hasSelection()) {
        QString sel = cur.selectedText();
        cur.insertText(QString(c) + sel + QString(closeChar));
        return true;
    } else {
        cur.insertText(QString(c) + QString(closeChar));
        cur.movePosition(QTextCursor::PreviousCharacter);
        setTextCursor(cur);
        return true;
    }
}

void CodeEditor::autoIndent(QKeyEvent* event) {
    QTextCursor cur = textCursor();
    QTextBlock block = cur.block();
    QString text = block.text();

    // Calculate current indentation
    int spaces = 0;
    for (QChar c : text) {
        if (c == ' ') spaces++;
        else if (c == '\t') spaces += m_tabWidth;
        else break;
    }

    // Check if line ends with '{' or ':'
    QString trimmed = text.trimmed();
    bool shouldAddIndent = trimmed.endsWith('{') || trimmed.endsWith(':');

    QPlainTextEdit::keyPressEvent(event);

    int newIndent = spaces + (shouldAddIndent ? m_tabWidth : 0);
    if (newIndent > 0) {
        QString indentStr;
        if (m_useSpaces) {
            indentStr = QString(newIndent, ' ');
        } else {
            indentStr = QString(newIndent / m_tabWidth, '\t');
        }
        textCursor().insertText(indentStr);
    }
}

void CodeEditor::keyPressEvent(QKeyEvent* event) {
    // Auto-closing brackets
    if (handleAutoClosingBrackets(event)) {
        event->accept();
        return;
    }

    // Auto Indent on Enter
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        autoIndent(event);
        return;
    }

    // Tab key: Indent selection or insert spaces
    if (event->key() == Qt::Key_Tab) {
        if (textCursor().hasSelection()) {
            indentSelection();
            return;
        } else if (m_useSpaces) {
            textCursor().insertText(QString(m_tabWidth, ' '));
            return;
        }
    }

    // Shift+Tab key: Unindent selection
    if (event->key() == Qt::Key_Backtab) {
        unindentSelection();
        return;
    }

    // Backspace: if deleting inside empty auto-closed pair (), [], {}, delete both!
    if (event->key() == Qt::Key_Backspace && !textCursor().hasSelection()) {
        int pos = textCursor().position();
        QString t = toPlainText();
        if (pos > 0 && pos < t.length()) {
            QChar prev = t[pos - 1];
            QChar next = t[pos];
            if ((prev == '(' && next == ')') ||
                (prev == '[' && next == ']') ||
                (prev == '{' && next == '}') ||
                (prev == '"' && next == '"') ||
                (prev == '\'' && next == '\'')) {
                textCursor().deleteChar();
                textCursor().deletePreviousChar();
                return;
            }
        }
    }

    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomInStep();
        } else if (event->angleDelta().y() < 0) {
            zoomOutStep();
        }
        event->accept();
        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

void CodeEditor::onCursorPositionChanged() {
    QTextCursor cur = textCursor();
    int line = cur.blockNumber() + 1;
    int col = cur.positionInBlock() + 1;
    int totalLines = blockCount();
    int totalChars = toPlainText().length();
    emit cursorPositionChangedInfo(line, col, totalLines, totalChars);
}

void CodeEditor::toggleComment() {
    QTextCursor cur = textCursor();
    int start = cur.selectionStart();
    int end = cur.selectionEnd();

    cur.setPosition(start);
    int startBlock = cur.blockNumber();
    cur.setPosition(end);
    int endBlock = cur.blockNumber();

    QString commentToken = "// ";
    if (m_document && (m_document->language() == core::Language::Python ||
                       m_document->language() == core::Language::Shell ||
                       m_document->language() == core::Language::CMake)) {
        commentToken = "# ";
    }

    cur.beginEditBlock();
    for (int b = startBlock; b <= endBlock; ++b) {
        QTextBlock block = document()->findBlockByNumber(b);
        QString text = block.text();
        cur.setPosition(block.position());

        if (text.trimmed().startsWith(commentToken.trimmed())) {
            // Uncomment
            int idx = text.indexOf(commentToken.trimmed());
            cur.setPosition(block.position() + idx);
            for (int i = 0; i < commentToken.length(); ++i) {
                cur.deleteChar();
            }
        } else {
            // Comment
            cur.insertText(commentToken);
        }
    }
    cur.endEditBlock();
}

void CodeEditor::duplicateLine() {
    QTextCursor cur = textCursor();
    cur.beginEditBlock();
    if (cur.hasSelection()) {
        QString sel = cur.selectedText();
        int end = cur.selectionEnd();
        cur.setPosition(end);
        cur.insertText(sel);
    } else {
        QTextBlock block = cur.block();
        QString text = block.text();
        cur.movePosition(QTextCursor::EndOfBlock);
        cur.insertBlock();
        cur.insertText(text);
    }
    cur.endEditBlock();
}

void CodeEditor::deleteCurrentLine() {
    QTextCursor cur = textCursor();
    cur.beginEditBlock();
    cur.select(QTextCursor::BlockUnderCursor);
    cur.removeSelectedText();
    cur.deleteChar(); // delete newline
    cur.endEditBlock();
}

void CodeEditor::moveLineUp() {
    QTextCursor cur = textCursor();
    int blockNum = cur.blockNumber();
    if (blockNum == 0) return;

    cur.beginEditBlock();
    QTextBlock currentBlock = cur.block();
    QString currentText = currentBlock.text();

    // Delete current line
    cur.select(QTextCursor::BlockUnderCursor);
    cur.removeSelectedText();
    cur.deleteChar();

    // Insert above
    cur.movePosition(QTextCursor::PreviousBlock);
    cur.movePosition(QTextCursor::StartOfBlock);
    cur.insertText(currentText + "\n");
    cur.movePosition(QTextCursor::PreviousBlock);
    cur.endEditBlock();
    setTextCursor(cur);
}

void CodeEditor::moveLineDown() {
    QTextCursor cur = textCursor();
    int blockNum = cur.blockNumber();
    if (blockNum >= blockCount() - 1) return;

    cur.beginEditBlock();
    QTextBlock currentBlock = cur.block();
    QString currentText = currentBlock.text();

    cur.select(QTextCursor::BlockUnderCursor);
    cur.removeSelectedText();
    cur.deleteChar();

    cur.movePosition(QTextCursor::NextBlock);
    cur.movePosition(QTextCursor::EndOfBlock);
    cur.insertText("\n" + currentText);
    cur.endEditBlock();
    setTextCursor(cur);
}

void CodeEditor::indentSelection() {
    QTextCursor cur = textCursor();
    int start = cur.selectionStart();
    int end = cur.selectionEnd();

    cur.setPosition(start);
    int startBlock = cur.blockNumber();
    cur.setPosition(end);
    int endBlock = cur.blockNumber();

    QString indent = m_useSpaces ? QString(m_tabWidth, ' ') : "\t";

    cur.beginEditBlock();
    for (int b = startBlock; b <= endBlock; ++b) {
        QTextBlock block = document()->findBlockByNumber(b);
        cur.setPosition(block.position());
        cur.insertText(indent);
    }
    cur.endEditBlock();
}

void CodeEditor::unindentSelection() {
    QTextCursor cur = textCursor();
    int start = cur.selectionStart();
    int end = cur.selectionEnd();

    cur.setPosition(start);
    int startBlock = cur.blockNumber();
    cur.setPosition(end);
    int endBlock = cur.blockNumber();

    cur.beginEditBlock();
    for (int b = startBlock; b <= endBlock; ++b) {
        QTextBlock block = document()->findBlockByNumber(b);
        QString text = block.text();
        cur.setPosition(block.position());

        if (text.startsWith("\t")) {
            cur.deleteChar();
        } else if (text.startsWith(" ")) {
            int spaces = 0;
            while (spaces < m_tabWidth && spaces < text.length() && text[spaces] == ' ') {
                spaces++;
            }
            for (int i = 0; i < spaces; ++i) {
                cur.deleteChar();
            }
        }
    }
    cur.endEditBlock();
}

void CodeEditor::showFindBar(bool replaceMode) {
    if (!m_findReplaceWidget) return;

    QString initial = textCursor().hasSelection() ? textCursor().selectedText() : QString();
    if (replaceMode) {
        m_findReplaceWidget->showReplace(initial);
    } else {
        m_findReplaceWidget->showFind(initial);
    }

    int w = std::min(450, width() - 40);
    int h = m_findReplaceWidget->sizeHint().height();
    m_findReplaceWidget->setGeometry(width() - w - 20, 10, w, h);
    m_findReplaceWidget->raise();
}

void CodeEditor::hideFindBar() {
    if (m_findReplaceWidget) {
        m_findReplaceWidget->hide();
    }
}

bool CodeEditor::findText(const QString& exp, bool forward, bool caseSensitive, bool wholeWord, bool isRegex) {
    QTextDocument::FindFlags flags;
    if (!forward) flags |= QTextDocument::FindBackward;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWord) flags |= QTextDocument::FindWholeWords;

    bool found = false;
    if (isRegex) {
        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
        if (!caseSensitive) options |= QRegularExpression::CaseInsensitiveOption;
        QRegularExpression regex(exp, options);
        found = find(regex, flags);
    } else {
        found = find(exp, flags);
    }

    if (!found) {
        // Wrap around search
        QTextCursor cur = textCursor();
        cur.movePosition(forward ? QTextCursor::Start : QTextCursor::End);
        setTextCursor(cur);
        if (isRegex) {
            QRegularExpression regex(exp);
            found = find(regex, flags);
        } else {
            found = find(exp, flags);
        }
    }

    return found;
}

int CodeEditor::replaceAll(const QString& findExp, const QString& replaceExp, bool caseSensitive, bool wholeWord, bool isRegex) {
    QTextCursor cur = textCursor();
    cur.beginEditBlock();
    cur.movePosition(QTextCursor::Start);
    setTextCursor(cur);

    int count = 0;
    while (findText(findExp, true, caseSensitive, wholeWord, isRegex)) {
        textCursor().insertText(replaceExp);
        count++;
    }
    cur.endEditBlock();
    return count;
}

void CodeEditor::zoomInStep() {
    m_currentZoomLevel += 1;
    const auto& cfg = core::Config::instance().editor();
    updateFontSettings(cfg.fontFamily, m_baseFontSize);
}

void CodeEditor::zoomOutStep() {
    m_currentZoomLevel -= 1;
    const auto& cfg = core::Config::instance().editor();
    updateFontSettings(cfg.fontFamily, m_baseFontSize);
}

void CodeEditor::resetZoom() {
    m_currentZoomLevel = 0;
    const auto& cfg = core::Config::instance().editor();
    updateFontSettings(cfg.fontFamily, m_baseFontSize);
}

void CodeEditor::gotoLine(int lineNumber, int column) {
    if (lineNumber < 1) lineNumber = 1;
    if (lineNumber > blockCount()) lineNumber = blockCount();

    QTextBlock block = document()->findBlockByNumber(lineNumber - 1);
    QTextCursor cur(block);
    if (column > 1) {
        cur.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column - 1);
    }
    setTextCursor(cur);
    centerCursor();
    setFocus();
}

} // namespace liteforge::editor
