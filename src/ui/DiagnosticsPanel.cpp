#include "liteforge/ui/DiagnosticsPanel.hpp"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFileInfo>

namespace liteforge::ui {

DiagnosticsPanel::DiagnosticsPanel(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void DiagnosticsPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels(QStringList() << "Type" << "Message" << "File" << "Line");
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setVisible(false);

    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::cellDoubleClicked, this, &DiagnosticsPanel::onCellDoubleClicked);
}

void DiagnosticsPanel::clearDiagnostics() {
    m_items.clear();
    m_table->setRowCount(0);
    m_errorCount = 0;
    m_warningCount = 0;
}

void DiagnosticsPanel::removeForFile(const QString& file) {
    std::vector<DiagnosticItem> keep;
    keep.reserve(m_items.size());
    bool removed = false;
    for (const auto& it : m_items) {
        if (it.file == file) removed = true; else keep.push_back(it);
    }
    if (removed) setDiagnostics(keep);
}

void DiagnosticsPanel::addDiagnostic(DiagnosticSeverity severity, const QString& message, const QString& file, int line, int column) {
    m_items.push_back({severity, message, file, line, column});
    if (severity == DiagnosticSeverity::Error) {
        m_errorCount++;
    } else if (severity == DiagnosticSeverity::Warning) {
        m_warningCount++;
    }

    int row = m_table->rowCount();
    m_table->insertRow(row);

    QString sevStr;
    QColor sevColor;
    if (severity == DiagnosticSeverity::Error) {
        sevStr = "Error";
        sevColor = QColor("#f87171");
    } else if (severity == DiagnosticSeverity::Warning) {
        sevStr = "Warning";
        sevColor = QColor("#fbbf24");
    } else {
        sevStr = "Info";
        sevColor = QColor("#38bdf8");
    }

    auto* sevItem = new QTableWidgetItem(sevStr);
    sevItem->setForeground(sevColor);

    auto* msgItem = new QTableWidgetItem(message);
    auto* fileItem = new QTableWidgetItem(QFileInfo(file).fileName());
    fileItem->setToolTip(file);
    auto* lineItem = new QTableWidgetItem(QString("%1:%2").arg(line).arg(column));

    m_table->setItem(row, 0, sevItem);
    m_table->setItem(row, 1, msgItem);
    m_table->setItem(row, 2, fileItem);
    m_table->setItem(row, 3, lineItem);
}

void DiagnosticsPanel::setDiagnostics(const std::vector<DiagnosticItem>& items) {
    clearDiagnostics();
    for (const auto& it : items) {
        addDiagnostic(it.severity, it.message, it.file, it.line, it.column);
    }
}

void DiagnosticsPanel::onCellDoubleClicked(int row, int /* column */) {
    if (row >= 0 && row < static_cast<int>(m_items.size())) {
        const auto& item = m_items[row];
        emit itemSelected(item.file, item.line, item.column);
    }
}

} // namespace liteforge::ui
