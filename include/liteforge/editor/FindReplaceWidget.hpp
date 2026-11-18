#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>

namespace liteforge::editor {

class CodeEditor;

class FindReplaceWidget : public QWidget {
    Q_OBJECT

public:
    explicit FindReplaceWidget(CodeEditor* editor, QWidget* parent = nullptr);
    ~FindReplaceWidget() override = default;

    void showFind(const QString& initialText = QString());
    void showReplace(const QString& initialText = QString());

    void findNext();
    void findPrevious();
    void replaceOne();
    void replaceAll();

signals:
    void closed();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupUi();
    void updateMatchCount();

    CodeEditor* m_editor;

    QLineEdit* m_findEdit;
    QLineEdit* m_replaceEdit;
    QLabel* m_matchCountLabel;
    QPushButton* m_prevButton;
    QPushButton* m_nextButton;
    QPushButton* m_replaceButton;
    QPushButton* m_replaceAllButton;
    QPushButton* m_closeButton;

    QCheckBox* m_caseSensitiveCheck;
    QCheckBox* m_wholeWordCheck;
    QCheckBox* m_regexCheck;

    QWidget* m_replaceRow;
};

} // namespace liteforge::editor
