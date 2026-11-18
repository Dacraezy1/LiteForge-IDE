#include "liteforge/editor/CodeEditor.hpp"
#include "liteforge/editor/LineNumberGutter.hpp"
#include "liteforge/editor/SyntaxHighlighter.hpp"
#include "liteforge/editor/FindReplaceWidget.hpp"
#include "liteforge/core/Config.hpp"

#include <QPainter>
#include <QTextBlock>
#include <QKeyEvent>
#include <QMouseEvent>
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
    digits = std::max(digits, 3);
    // Breakpoint area (14px) + digits + folding icon area (14px) + padding
    int space = 30 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberGutterWidth(int /* newBlockCount */) {
    setViewportMargins(lineNumberGutterWidth(), 0, m_minimapEnabled ? 70 : 0, 0);
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
            int lineNum = blockNumber + 1;

            // 1. Breakpoint Indicator
            if (hasBreakpoint(lineNum)) {
                painter.setBrush(QColor(230, 45, 45));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(4, top + (fontMetrics().height() - 10) / 2, 9, 9);
            }

            // 2. Line Number
            QString number = QString::number(lineNum);
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

            int numWidth = m_lineNumberGutter->width() - 20;
            painter.drawText(14, top, numWidth, fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);

            // 3. Folding Chevron
            QString lineText = block.text().trimmed();
            if (lineText.endsWith('{') || lineText.endsWith(':')) {
                painter.setPen(theme.lineNumberNormal);
                bool folded = isBlockFolded(blockNumber);
                QString chevron = folded ? "▶" : "▼";
                painter.drawText(m_lineNumberGutter->width() - 14, top, 12, fontMetrics().height(),
                                 Qt::AlignCenter, chevron);
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::gutterMousePressEvent(QMouseEvent* event) {
    int y = event->pos().y();
    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= m_lineNumberGutter->height()) {
        if (block.isVisible() && y >= top && y <= bottom) {
            int lineNum = block.blockNumber() + 1;

            // Click on chevron area (right 16px of gutter) -> Fold/Unfold
            if (event->pos().x() >= m_lineNumberGutter->width() - 16) {
                toggleFold(block.blockNumber());
                return;
            }

            // Click on line number / breakpoint area -> Toggle Breakpoint
            toggleBreakpoint(lineNum);
            m_lineNumberGutter->update();
            return;
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }
}

void CodeEditor::toggleBreakpoint(int lineNumber) {
    if (m_breakpoints.count(lineNumber)) {
        m_breakpoints.erase(lineNumber);
        emit breakpointToggled(lineNumber, false);
    } else {
        m_breakpoints.insert(lineNumber);
        emit breakpointToggled(lineNumber, true);
    }
    m_lineNumberGutter->update();
}

bool CodeEditor::hasBreakpoint(int lineNumber) const {
    return m_breakpoints.count(lineNumber) > 0;
}

void CodeEditor::toggleFold(int blockNumber) {
    int endBlock = findMatchingBraceBlock(blockNumber);
    if (endBlock <= blockNumber) return;

    bool folded = isBlockFolded(blockNumber);
    if (folded) {
        // Unfold
        m_foldedBlocks.erase(blockNumber);
        for (int i = blockNumber + 1; i <= endBlock; ++i) {
            QTextBlock b = document()->findBlockByNumber(i);
            b.setVisible(true);
        }
    } else {
        // Fold
        m_foldedBlocks.insert(blockNumber);
        for (int i = blockNumber + 1; i <= endBlock; ++i) {
            QTextBlock b = document()->findBlockByNumber(i);
            b.setVisible(false);
        }
    }

    document()->markContentsDirty(0, document()->characterCount());
    m_lineNumberGutter->update();
    viewport()->update();
}

bool CodeEditor::isBlockFolded(int blockNumber) const {
    return m_foldedBlocks.count(blockNumber) > 0;
}

void CodeEditor::foldAll() {
    for (int i = 0; i < blockCount(); ++i) {
        QTextBlock b = document()->findBlockByNumber(i);
        QString t = b.text().trimmed();
        if ((t.endsWith('{') || t.endsWith(':')) && !isBlockFolded(i)) {
            toggleFold(i);
        }
    }
}

void CodeEditor::unfoldAll() {
    m_foldedBlocks.clear();
    for (int i = 0; i < blockCount(); ++i) {
        QTextBlock b = document()->findBlockByNumber(i);
        b.setVisible(true);
    }
    document()->markContentsDirty(0, document()->characterCount());
    m_lineNumberGutter->update();
    viewport()->update();
}

int CodeEditor::findMatchingBraceBlock(int startBlockNumber) {
    int depth = 0;
    for (int i = startBlockNumber; i < blockCount(); ++i) {
        QString text = document()->findBlockByNumber(i).text();
        for (QChar c : text) {
            if (c == '{') depth++;
            else if (c == '}') {
                depth--;
                if (depth == 0) return i;
            }
        }
    }
    return startBlockNumber;
}

void CodeEditor::selectNextOccurrence() {
    QTextCursor cur = textCursor();
    QString targetText;

    if (cur.hasSelection()) {
        targetText = cur.selectedText();
    } else {
        cur.select(QTextCursor::WordUnderCursor);
        targetText = cur.selectedText();
        setTextCursor(cur);
        if (targetText.isEmpty()) return;
    }

    int searchStart = cur.selectionEnd();
    if (!m_extraCursors.isEmpty()) {
        searchStart = m_extraCursors.last().selectionEnd();
    }

    QTextDocument* doc = document();
    QTextCursor next = doc->find(targetText, searchStart);
    if (next.isNull()) {
        next = doc->find(targetText, 0); // Wrap around
    }

    if (!next.isNull() && next != cur) {
        m_extraCursors.append(next);
        highlightCurrentLineAndBrackets();
        viewport()->update();
    }
}

void CodeEditor::clearExtraCursors() {
    m_extraCursors.clear();
    highlightCurrentLineAndBrackets();
    viewport()->update();
}

void CodeEditor::setMinimapEnabled(bool enabled) {
    m_minimapEnabled = enabled;
    updateLineNumberGutterWidth(0);
    viewport()->update();
}

void CodeEditor::setStickyScrollEnabled(bool enabled) {
    m_stickyScrollEnabled = enabled;
    viewport()->update();
}

void CodeEditor::setWordWrap(bool wrap) {
    setLineWrapMode(wrap ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

bool CodeEditor::isWordWrap() const {
    return lineWrapMode() == QPlainTextEdit::WidgetWidth;
}

void CodeEditor::paintEvent(QPaintEvent* event) {
    QPlainTextEdit::paintEvent(event);

    QPainter painter(viewport());
    const auto& theme = core::ThemeManager::instance().currentTheme();

    // 1. Paint subtle indentation guide lines
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

    // 2. Draw Multi-Cursor Carets
    if (!m_extraCursors.isEmpty()) {
        painter.setBrush(theme.textNormal);
        painter.setPen(Qt::NoPen);
        for (const auto& cur : m_extraCursors) {
            QRect r = cursorRect(cur);
            painter.drawRect(r.left(), r.top(), 2, r.height());
        }
    }

    // 3. Paint Sticky Scroll Header
    if (m_stickyScrollEnabled) {
        paintStickyScroll(painter);
    }

    // 4. Paint Minimap
    if (m_minimapEnabled) {
        paintMinimap(painter);
    }
}

void CodeEditor::paintStickyScroll(QPainter& painter) {
    QTextBlock firstBlock = firstVisibleBlock();
    int firstBlockNum = firstBlock.blockNumber();
    if (firstBlockNum < 2) return;

    // Scan backwards to find the enclosing function/class/struct definition
    QString scopeHeader;
    for (int i = firstBlockNum; i >= 0; --i) {
        QString text = document()->findBlockByNumber(i).text().trimmed();
        if ((text.contains("class ") || text.contains("struct ") || text.contains("void ") ||
             text.contains("int ") || text.contains("bool ") || text.contains("def ") ||
             text.contains("fn ") || text.startsWith("namespace ")) && text.endsWith('{')) {
            scopeHeader = text;
            break;
        }
    }

    if (!scopeHeader.isEmpty()) {
        const auto& theme = core::ThemeManager::instance().currentTheme();
        QRect headerRect(0, 0, viewport()->width() - (m_minimapEnabled ? 70 : 0), fontMetrics().height() + 6);
        painter.fillRect(headerRect, theme.sidebarBg);
        painter.setPen(theme.border);
        painter.drawLine(0, headerRect.bottom(), headerRect.right(), headerRect.bottom());

        painter.setPen(theme.keyword);
        painter.setFont(font());
        painter.drawText(8, 0, headerRect.width() - 16, headerRect.height(),
                         Qt::AlignLeft | Qt::AlignVCenter, scopeHeader);
    }
}

void CodeEditor::paintMinimap(QPainter& painter) {
    int w = 65;
    int x = viewport()->width() - w;
    QRect minimapRect(x, 0, w, viewport()->height());

    const auto& theme = core::ThemeManager::instance().currentTheme();
    painter.fillRect(minimapRect, QColor(theme.sidebarBg.red(), theme.sidebarBg.green(), theme.sidebarBg.blue(), 180));
    painter.setPen(theme.border);
    painter.drawLine(x, 0, x, viewport()->height());

    // Draw scaled representation
    int totalBlocks = std::max(1, blockCount());
    double lineH = std::max(1.5, static_cast<double>(viewport()->height()) / totalBlocks);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(theme.textNormal.red(), theme.textNormal.green(), theme.textNormal.blue(), 100));

    for (int i = 0; i < totalBlocks; i += std::max(1, totalBlocks / 300)) {
        QString t = document()->findBlockByNumber(i).text();
        int len = std::min(w - 10, static_cast<int>(t.length() * 2));
        if (len > 0) {
            int y = static_cast<int>(i * lineH);
            painter.drawRect(x + 5, y, len, std::max(1, static_cast<int>(lineH - 0.5)));
        }
    }

    // Viewport indicator rect
    int firstNum = firstVisibleBlock().blockNumber();
    int visibleBlocks = viewport()->height() / fontMetrics().height();
    int vpY = static_cast<int>(firstNum * lineH);
    int vpH = static_cast<int>(visibleBlocks * lineH);
    painter.setBrush(QColor(255, 255, 255, 30));
    painter.setPen(QColor(255, 255, 255, 60));
    painter.drawRect(x + 2, vpY, w - 4, std::max(10, vpH));
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

    // Extra cursor highlights
    for (const auto& cur : m_extraCursors) {
        if (cur.hasSelection()) {
            QTextEdit::ExtraSelection sel;
            sel.format.setBackground(core::ThemeManager::instance().currentTheme().selectionBg);
            sel.cursor = cur;
            extraSelections.append(sel);
        }
    }

    // Bracket matching
    matchBrackets();

    setExtraSelections(extraSelections);
}

void CodeEditor::matchBrackets() {
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

    int spaces = 0;
    for (QChar c : text) {
        if (c == ' ') spaces++;
        else if (c == '\t') spaces += m_tabWidth;
        else break;
    }

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
    // 1. Multi-Cursor Key Replicating
    if (!m_extraCursors.isEmpty()) {
        if (event->key() == Qt::Key_Escape) {
            clearExtraCursors();
            event->accept();
            return;
        }

        QString typed = event->text();
        if (!typed.isEmpty() && typed[0].isPrint()) {
            for (auto& c : m_extraCursors) {
                c.insertText(typed);
            }
        } else if (event->key() == Qt::Key_Backspace) {
            for (auto& c : m_extraCursors) {
                c.deletePreviousChar();
            }
        } else if (event->key() == Qt::Key_Delete) {
            for (auto& c : m_extraCursors) {
                c.deleteChar();
            }
        }
    }

    // 2. Select Next Occurrence (Ctrl+D)
    if (event->key() == Qt::Key_D && (event->modifiers() & Qt::ControlModifier)) {
        selectNextOccurrence();
        event->accept();
        return;
    }

    // 3. LSP Shortcuts
    if (event->key() == Qt::Key_F12) {
        int line = textCursor().blockNumber() + 1;
        int col = textCursor().positionInBlock() + 1;
        if (event->modifiers() & Qt::ShiftModifier) {
            emit referencesRequested(line, col);
        } else {
            emit definitionRequested(line, col);
        }
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_F2) {
        emit renameRequested(textCursor().blockNumber() + 1, textCursor().positionInBlock() + 1);
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Space && (event->modifiers() & Qt::ControlModifier)) {
        QTextCursor cur = textCursor();
        cur.select(QTextCursor::WordUnderCursor);
        emit completionRequested(cur.blockNumber() + 1, cur.positionInBlock() + 1, cur.selectedText());
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_I && (event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::ShiftModifier)) {
        emit formatRequested();
        event->accept();
        return;
    }

    // 4. Auto-closing brackets
    if (handleAutoClosingBrackets(event)) {
        event->accept();
        return;
    }

    // 5. Auto Indent on Enter
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        autoIndent(event);
        return;
    }

    // 6. Tab key: Indent selection or insert spaces
    if (event->key() == Qt::Key_Tab) {
        if (textCursor().hasSelection()) {
            indentSelection();
            return;
        } else if (m_useSpaces) {
            textCursor().insertText(QString(m_tabWidth, ' '));
            return;
        }
    }

    // 7. Shift+Tab key: Unindent selection
    if (event->key() == Qt::Key_Backtab) {
        unindentSelection();
        return;
    }

    // 8. Backspace: delete both if inside pair
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

void CodeEditor::mouseMoveEvent(QMouseEvent* event) {
    QPlainTextEdit::mouseMoveEvent(event);

    // Hover tooltip request for LSP
    if (event->modifiers() & Qt::ControlModifier) {
        QTextCursor cur = cursorForPosition(event->pos());
        emit hoverRequested(cur.blockNumber() + 1, cur.positionInBlock() + 1, event->globalPosition().toPoint());
    }
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
            int idx = text.indexOf(commentToken.trimmed());
            cur.setPosition(block.position() + idx);
            for (int i = 0; i < commentToken.length(); ++i) {
                cur.deleteChar();
            }
        } else {
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
    cur.deleteChar();
    cur.endEditBlock();
}

void CodeEditor::moveLineUp() {
    QTextCursor cur = textCursor();
    int blockNum = cur.blockNumber();
    if (blockNum == 0) return;

    cur.beginEditBlock();
    QTextBlock currentBlock = cur.block();
    QString currentText = currentBlock.text();

    cur.select(QTextCursor::BlockUnderCursor);
    cur.removeSelectedText();
    cur.deleteChar();

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
