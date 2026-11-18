#include "liteforge/editor/LineNumberGutter.hpp"
#include "liteforge/editor/CodeEditor.hpp"

namespace liteforge::editor {

LineNumberGutter::LineNumberGutter(CodeEditor* editor)
    : QWidget(editor), m_editor(editor) {
}

QSize LineNumberGutter::sizeHint() const {
    return QSize(m_editor->lineNumberGutterWidth(), 0);
}

void LineNumberGutter::paintEvent(QPaintEvent* event) {
    m_editor->lineNumberGutterPaintEvent(event);
}

void LineNumberGutter::mousePressEvent(QMouseEvent* event) {
    m_editor->gutterMousePressEvent(event);
}

} // namespace liteforge::editor
