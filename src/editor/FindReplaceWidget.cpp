#include "liteforge/editor/FindReplaceWidget.hpp"
#include "liteforge/editor/CodeEditor.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QRegularExpression>

namespace liteforge::editor {

FindReplaceWidget::FindReplaceWidget(CodeEditor* editor, QWidget* parent)
    : QWidget(parent), m_editor(editor) {
    setupUi();
    hide();
}

void FindReplaceWidget::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(R"(
        FindReplaceWidget {
            background-color: #21252b;
            border: 1px solid #181a1f;
            border-radius: 6px;
        }
        QLineEdit {
            background-color: #1b1d23;
            color: #d7dae0;
            border: 1px solid #3b4048;
            border-radius: 3px;
            padding: 3px 6px;
            font-size: 12px;
        }
        QLineEdit:focus {
            border: 1px solid #528bff;
        }
        QPushButton {
            background-color: #2c313a;
            color: #abb2bf;
            border: 1px solid #3b4048;
            border-radius: 3px;
            padding: 3px 8px;
            font-size: 11px;
        }
        QPushButton:hover {
            background-color: #3e4451;
            color: #ffffff;
        }
        QCheckBox {
            color: #abb2bf;
            font-size: 11px;
        }
    )");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(4);

    // Row 1: Find Row
    auto* findRow = new QWidget(this);
    auto* findLayout = new QHBoxLayout(findRow);
    findLayout->setContentsMargins(0, 0, 0, 0);
    findLayout->setSpacing(4);

    m_findEdit = new QLineEdit(this);
    m_findEdit->setPlaceholderText("Find...");
    m_findEdit->setMinimumWidth(180);

    m_matchCountLabel = new QLabel("No matches", this);
    m_matchCountLabel->setStyleSheet("color: #71717a; font-size: 11px;");

    m_prevButton = new QPushButton("<", this);
    m_prevButton->setToolTip("Previous match (Shift+Enter)");
    m_nextButton = new QPushButton(">", this);
    m_nextButton->setToolTip("Next match (Enter)");

    m_caseSensitiveCheck = new QCheckBox("Aa", this);
    m_caseSensitiveCheck->setToolTip("Match Case");

    m_wholeWordCheck = new QCheckBox("W", this);
    m_wholeWordCheck->setToolTip("Match Whole Word");

    m_regexCheck = new QCheckBox(".*", this);
    m_regexCheck->setToolTip("Regular Expression");

    m_closeButton = new QPushButton("x", this);
    m_closeButton->setToolTip("Close (Esc)");
    m_closeButton->setFixedWidth(24);

    findLayout->addWidget(m_findEdit);
    findLayout->addWidget(m_matchCountLabel);
    findLayout->addWidget(m_prevButton);
    findLayout->addWidget(m_nextButton);
    findLayout->addWidget(m_caseSensitiveCheck);
    findLayout->addWidget(m_wholeWordCheck);
    findLayout->addWidget(m_regexCheck);
    findLayout->addWidget(m_closeButton);

    mainLayout->addWidget(findRow);

    // Row 2: Replace Row
    m_replaceRow = new QWidget(this);
    auto* replaceLayout = new QHBoxLayout(m_replaceRow);
    replaceLayout->setContentsMargins(0, 0, 0, 0);
    replaceLayout->setSpacing(4);

    m_replaceEdit = new QLineEdit(this);
    m_replaceEdit->setPlaceholderText("Replace...");
    m_replaceEdit->setMinimumWidth(180);

    m_replaceButton = new QPushButton("Replace", this);
    m_replaceAllButton = new QPushButton("Replace All", this);

    replaceLayout->addWidget(m_replaceEdit);
    replaceLayout->addWidget(m_replaceButton);
    replaceLayout->addWidget(m_replaceAllButton);
    replaceLayout->addStretch();

    mainLayout->addWidget(m_replaceRow);

    // Connections
    connect(m_findEdit, &QLineEdit::textChanged, this, [this]() {
        updateMatchCount();
        findNext();
    });
    connect(m_findEdit, &QLineEdit::returnPressed, this, &FindReplaceWidget::findNext);
    connect(m_replaceEdit, &QLineEdit::returnPressed, this, &FindReplaceWidget::replaceOne);

    connect(m_prevButton, &QPushButton::clicked, this, &FindReplaceWidget::findPrevious);
    connect(m_nextButton, &QPushButton::clicked, this, &FindReplaceWidget::findNext);
    connect(m_replaceButton, &QPushButton::clicked, this, &FindReplaceWidget::replaceOne);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &FindReplaceWidget::replaceAll);
    connect(m_closeButton, &QPushButton::clicked, this, [this]() {
        hide();
        emit closed();
        m_editor->setFocus();
    });

    connect(m_caseSensitiveCheck, &QCheckBox::toggled, this, &FindReplaceWidget::findNext);
    connect(m_wholeWordCheck, &QCheckBox::toggled, this, &FindReplaceWidget::findNext);
    connect(m_regexCheck, &QCheckBox::toggled, this, &FindReplaceWidget::findNext);
}

void FindReplaceWidget::showFind(const QString& initialText) {
    m_replaceRow->hide();
    if (!initialText.isEmpty()) {
        m_findEdit->setText(initialText);
    }
    show();
    m_findEdit->setFocus();
    m_findEdit->selectAll();
    updateMatchCount();
}

void FindReplaceWidget::showReplace(const QString& initialText) {
    m_replaceRow->show();
    if (!initialText.isEmpty()) {
        m_findEdit->setText(initialText);
    }
    show();
    m_findEdit->setFocus();
    m_findEdit->selectAll();
    updateMatchCount();
}

void FindReplaceWidget::findNext() {
    if (m_findEdit->text().isEmpty()) return;
    m_editor->findText(m_findEdit->text(), true, 
                       m_caseSensitiveCheck->isChecked(),
                       m_wholeWordCheck->isChecked(),
                       m_regexCheck->isChecked());
    updateMatchCount();
}

void FindReplaceWidget::findPrevious() {
    if (m_findEdit->text().isEmpty()) return;
    m_editor->findText(m_findEdit->text(), false,
                       m_caseSensitiveCheck->isChecked(),
                       m_wholeWordCheck->isChecked(),
                       m_regexCheck->isChecked());
    updateMatchCount();
}

void FindReplaceWidget::replaceOne() {
    if (m_findEdit->text().isEmpty()) return;
    QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection()) {
        cursor.insertText(m_replaceEdit->text());
    }
    findNext();
}

void FindReplaceWidget::replaceAll() {
    if (m_findEdit->text().isEmpty()) return;
    int count = m_editor->replaceAll(m_findEdit->text(), m_replaceEdit->text(),
                                     m_caseSensitiveCheck->isChecked(),
                                     m_wholeWordCheck->isChecked(),
                                     m_regexCheck->isChecked());
    m_matchCountLabel->setText(QString("Replaced %1").arg(count));
}

void FindReplaceWidget::updateMatchCount() {
    QString text = m_findEdit->text();
    if (text.isEmpty()) {
        m_matchCountLabel->setText("No matches");
        return;
    }

    QString docText = m_editor->toPlainText();
    int count = 0;

    if (m_regexCheck->isChecked()) {
        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
        if (!m_caseSensitiveCheck->isChecked()) {
            options |= QRegularExpression::CaseInsensitiveOption;
        }
        QRegularExpression regex(text, options);
        if (regex.isValid()) {
            auto it = regex.globalMatch(docText);
            while (it.hasNext()) {
                it.next();
                count++;
            }
        }
    } else {
        Qt::CaseSensitivity cs = m_caseSensitiveCheck->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
        int idx = 0;
        while ((idx = docText.indexOf(text, idx, cs)) != -1) {
            count++;
            idx += text.length();
        }
    }

    if (count == 0) {
        m_matchCountLabel->setText("No matches");
    } else {
        m_matchCountLabel->setText(QString("%1 matches").arg(count));
    }
}

void FindReplaceWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        hide();
        emit closed();
        m_editor->setFocus();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

} // namespace liteforge::editor
