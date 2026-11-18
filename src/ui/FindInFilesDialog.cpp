#include "liteforge/ui/FindInFilesDialog.hpp"
#include "liteforge/core/FileUtils.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QHeaderView>

namespace liteforge::ui {

FindInFilesDialog::FindInFilesDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Find in Project Files");
    resize(700, 480);
    setupUi();
}

void FindInFilesDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // Search input row
    auto* searchRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Search query...");
    searchRow->addWidget(m_searchEdit);

    m_searchButton = new QPushButton("Search", this);
    m_searchButton->setDefault(true);
    searchRow->addWidget(m_searchButton);
    mainLayout->addLayout(searchRow);

    // Options row
    auto* optRow = new QHBoxLayout();
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText("File filter (e.g. *.cpp, *.hpp)");
    optRow->addWidget(m_filterEdit);

    m_caseSensitiveCheck = new QCheckBox("Match Case", this);
    m_wholeWordCheck = new QCheckBox("Whole Word", this);
    m_regexCheck = new QCheckBox("Regex", this);
    optRow->addWidget(m_caseSensitiveCheck);
    optRow->addWidget(m_wholeWordCheck);
    optRow->addWidget(m_regexCheck);
    mainLayout->addLayout(optRow);

    // Results Tree
    m_resultsTree = new QTreeWidget(this);
    m_resultsTree->setHeaderLabels(QStringList() << "File / Match" << "Line");
    m_resultsTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_resultsTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mainLayout->addWidget(m_resultsTree);

    // Status bar
    m_statusLabel = new QLabel("Ready", this);
    mainLayout->addWidget(m_statusLabel);

    connect(m_searchButton, &QPushButton::clicked, this, &FindInFilesDialog::startSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &FindInFilesDialog::startSearch);
    connect(m_resultsTree, &QTreeWidget::itemDoubleClicked, this, &FindInFilesDialog::onItemDoubleClicked);
}

void FindInFilesDialog::setProjectRoot(const QString& rootPath) {
    m_projectRoot = rootPath;
}

void FindInFilesDialog::showDialog(const QString& initialText) {
    if (!initialText.isEmpty()) {
        m_searchEdit->setText(initialText);
        m_searchEdit->selectAll();
    }
    show();
    m_searchEdit->setFocus();
}

void FindInFilesDialog::startSearch() {
    QString query = m_searchEdit->text();
    if (query.isEmpty() || m_projectRoot.isEmpty()) return;

    m_resultsTree->clear();
    m_statusLabel->setText("Searching...");

    QStringList filters;
    if (!m_filterEdit->text().trimmed().isEmpty()) {
        filters = m_filterEdit->text().split(',', Qt::SkipEmptyParts);
        for (QString& f : filters) f = f.trimmed();
    }

    QStringList files = core::FileUtils::findFiles(m_projectRoot, filters);

    int totalMatches = 0;
    int filesWithMatches = 0;

    QRegularExpression regex;
    if (m_regexCheck->isChecked()) {
        QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
        if (!m_caseSensitiveCheck->isChecked()) {
            options |= QRegularExpression::CaseInsensitiveOption;
        }
        regex.setPattern(query);
        regex.setPatternOptions(options);
    }

    Qt::CaseSensitivity cs = m_caseSensitiveCheck->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    for (const QString& filePath : files) {
        if (core::FileUtils::isBinaryFile(filePath)) continue;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QTextStream in(&file);
        int lineNum = 0;
        QTreeWidgetItem* fileItem = nullptr;

        while (!in.atEnd()) {
            QString line = in.readLine();
            lineNum++;
            bool match = false;

            if (m_regexCheck->isChecked()) {
                if (regex.isValid() && regex.match(line).hasMatch()) {
                    match = true;
                }
            } else if (m_wholeWordCheck->isChecked()) {
                QRegularExpression wordRegex(QString("\\b%1\\b").arg(QRegularExpression::escape(query)),
                    cs == Qt::CaseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
                match = wordRegex.match(line).hasMatch();
            } else {
                match = line.contains(query, cs);
            }

            if (match) {
                if (!fileItem) {
                    fileItem = new QTreeWidgetItem(m_resultsTree);
                    fileItem->setText(0, core::FileUtils::relativePath(m_projectRoot, filePath));
                    fileItem->setData(0, Qt::UserRole, filePath);
                    filesWithMatches++;
                }

                auto* matchItem = new QTreeWidgetItem(fileItem);
                matchItem->setText(0, line.trimmed());
                matchItem->setText(1, QString::number(lineNum));
                matchItem->setData(0, Qt::UserRole, filePath);
                matchItem->setData(1, Qt::UserRole, lineNum);
                totalMatches++;
            }
        }
        if (fileItem) {
            fileItem->setExpanded(true);
        }
    }

    m_statusLabel->setText(QString("Found %1 matches in %2 files.").arg(totalMatches).arg(filesWithMatches));
}

void FindInFilesDialog::onItemDoubleClicked(QTreeWidgetItem* item, int /* column */) {
    if (!item) return;

    QString filePath = item->data(0, Qt::UserRole).toString();
    int line = item->data(1, Qt::UserRole).toInt();

    if (!filePath.isEmpty() && line > 0) {
        emit resultSelected(filePath, line);
        hide();
    }
}

} // namespace liteforge::ui
