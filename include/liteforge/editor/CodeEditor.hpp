#pragma once

#include "liteforge/core/Document.hpp"
#include "liteforge/core/Theme.hpp"
#include <QPlainTextEdit>
#include <memory>

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

protected:
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
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

    std::shared_ptr<core::Document> m_document;
    LineNumberGutter* m_lineNumberGutter{nullptr};
    SyntaxHighlighter* m_highlighter{nullptr};
    FindReplaceWidget* m_findReplaceWidget{nullptr};

    int m_baseFontSize{13};
    int m_currentZoomLevel{0};
    bool m_autoCloseBrackets{true};
    bool m_highlightCurrentLine{true};
    int m_tabWidth{4};
    bool m_useSpaces{true};
};

} // namespace liteforge::editor
