#pragma once

#include "liteforge/core/Document.hpp"
#include "liteforge/core/Theme.hpp"
#include <QPlainTextEdit>
#include <memory>
#include <set>
#include <vector>

namespace liteforge::editor {

class LineNumberGutter;
class SyntaxHighlighter;
class FindReplaceWidget;

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget* parent = nullptr);
    ~CodeEditor() override;

    void setDocumentModel(std::shared_ptr<core::Document> doc);
    std::shared_ptr<core::Document> documentModel() const { return m_document; }

    void lineNumberGutterPaintEvent(QPaintEvent* event);
    void gutterMousePressEvent(QMouseEvent* event);
    int lineNumberGutterWidth();

    void applyTheme(const core::ThemeColors& theme);
    void updateFontSettings(const QString& family, int size);

    // Editing utilities
    void toggleComment();
    void duplicateLine();
    void deleteCurrentLine();
    void moveLineUp();
    void moveLineDown();
    void indentSelection();
    void unindentSelection();

    // Multi-cursor & Selection
    void selectNextOccurrence();
    void clearExtraCursors();
    bool hasExtraCursors() const { return !m_extraCursors.isEmpty(); }
    const QList<QTextCursor>& extraCursors() const { return m_extraCursors; }

    // Code Folding
    void toggleFold(int blockNumber);
    bool isBlockFolded(int blockNumber) const;
    void foldAll();
    void unfoldAll();

    // Breakpoints
    void toggleBreakpoint(int lineNumber);
    bool hasBreakpoint(int lineNumber) const;
    const std::set<int>& breakpoints() const { return m_breakpoints; }

    // Minimap & Sticky Scroll & Word Wrap
    void setMinimapEnabled(bool enabled);
    bool isMinimapEnabled() const { return m_minimapEnabled; }
    void setStickyScrollEnabled(bool enabled);
    bool isStickyScrollEnabled() const { return m_stickyScrollEnabled; }
    void setWordWrap(bool wrap);
    bool isWordWrap() const;

    // Find and Replace
    void showFindBar(bool replaceMode = false);
    void hideFindBar();
    bool findText(const QString& exp, bool forward, bool caseSensitive, bool wholeWord, bool isRegex);
    int replaceAll(const QString& findExp, const QString& replaceExp, bool caseSensitive, bool wholeWord, bool isRegex);

    // Zoom
    void zoomInStep();
    void zoomOutStep();
    void resetZoom();

    // Goto Line
    void gotoLine(int lineNumber, int column = 0);

signals:
    void cursorPositionChangedInfo(int line, int col, int totalLines, int totalChars);
    void breakpointToggled(int lineNumber, bool active);
    void definitionRequested(int line, int col);
    void referencesRequested(int line, int col);
    void hoverRequested(int line, int col, const QPoint& globalPos);
    void completionRequested(int line, int col, const QString& prefix);
    void formatRequested();
    void renameRequested(int line, int col);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void updateLineNumberGutterWidth(int newBlockCount);
    void highlightCurrentLineAndBrackets();
    void updateLineNumberGutter(const QRect& rect, int dy);
    void onCursorPositionChanged();

private:
    void matchBrackets();
    void autoIndent(QKeyEvent* event);
    bool handleAutoClosingBrackets(QKeyEvent* event);
    void paintMinimap(QPainter& painter);
    void paintStickyScroll(QPainter& painter);
    int findMatchingBraceBlock(int startBlockNumber);

    std::shared_ptr<core::Document> m_document;
    LineNumberGutter* m_lineNumberGutter{nullptr};
    SyntaxHighlighter* m_highlighter{nullptr};
    FindReplaceWidget* m_findReplaceWidget{nullptr};

    // Multi-cursors
    QList<QTextCursor> m_extraCursors;

    // Folding & Breakpoints
    std::set<int> m_foldedBlocks;
    std::set<int> m_breakpoints;

    // Features
    bool m_minimapEnabled{false};
    bool m_stickyScrollEnabled{true};
    int m_baseFontSize{13};
    int m_currentZoomLevel{0};
    bool m_autoCloseBrackets{true};
    bool m_highlightCurrentLine{true};
    int m_tabWidth{4};
    bool m_useSpaces{true};
};

} // namespace liteforge::editor
