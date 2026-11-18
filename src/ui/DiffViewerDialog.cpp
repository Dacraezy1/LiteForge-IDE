#include "liteforge/ui/DiffViewerDialog.hpp"
#include "liteforge/core/Theme.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QTextBlock>

namespace liteforge::ui {

DiffViewerDialog::DiffViewerDialog(const QString& filePath, const QString& diffText, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Git Diff — " + filePath);
    resize(850, 600);
    setupUi(filePath, diffText);
}

void DiffViewerDialog::setupUi(const QString& filePath, const QString& diffText) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    auto* headerLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel("Diff: <b>" + filePath + "</b>", this);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    layout->addLayout(headerLayout);

    m_diffEditor = new QPlainTextEdit(this);
    m_diffEditor->setReadOnly(true);
    m_diffEditor->setLineWrapMode(QPlainTextEdit::NoWrap);

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(11);
    m_diffEditor->setFont(font);

    applyDiffStyling(diffText);
    layout->addWidget(m_diffEditor);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* closeBtn = new QPushButton("Close", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    layout->addLayout(btnLayout);
}

void DiffViewerDialog::applyDiffStyling(const QString& diffText) {
    m_diffEditor->setPlainText(diffText);

    QTextDocument* doc = m_diffEditor->document();
    QList<QTextEdit::ExtraSelection> selections;

    const auto& theme = core::ThemeManager::instance().currentTheme();
    QColor addBg(40, 140, 50, 60);
    QColor delBg(180, 40, 40, 60);
    QColor hdrBg(theme.selectionBg);

    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        QString text = block.text();
        if (text.startsWith('+') && !text.startsWith("+++")) {
            QTextEdit::ExtraSelection sel;
            sel.cursor = QTextCursor(block);
            sel.format.setBackground(addBg);
            sel.format.setProperty(QTextFormat::FullWidthSelection, true);
            selections.append(sel);
        } else if (text.startsWith('-') && !text.startsWith("---")) {
            QTextEdit::ExtraSelection sel;
            sel.cursor = QTextCursor(block);
            sel.format.setBackground(delBg);
            sel.format.setProperty(QTextFormat::FullWidthSelection, true);
            selections.append(sel);
        } else if (text.startsWith("@@") || text.startsWith("diff --git")) {
            QTextEdit::ExtraSelection sel;
            sel.cursor = QTextCursor(block);
            sel.format.setBackground(hdrBg);
            sel.format.setForeground(theme.keyword);
            sel.format.setProperty(QTextFormat::FullWidthSelection, true);
            selections.append(sel);
        }
    }

    m_diffEditor->setExtraSelections(selections);
}

} // namespace liteforge::ui
