#pragma once

#include <QWidget>

namespace liteforge::editor {

class CodeEditor;

class LineNumberGutter : public QWidget {
    Q_OBJECT

public:
    explicit LineNumberGutter(CodeEditor* editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    CodeEditor* m_editor;
};

} // namespace liteforge::editor
