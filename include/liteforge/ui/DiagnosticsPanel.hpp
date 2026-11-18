#pragma once

#include <QWidget>
#include <QTableWidget>
#include <vector>

namespace liteforge::ui {

enum class DiagnosticSeverity {
    Error,
    Warning,
    Information,
    Hint
};

struct DiagnosticItem {
    DiagnosticSeverity severity;
    QString message;
    QString file;
    int line{1};
    int column{1};
};

class DiagnosticsPanel : public QWidget {
    Q_OBJECT

public:
    explicit DiagnosticsPanel(QWidget* parent = nullptr);
    ~DiagnosticsPanel() override = default;

    void clearDiagnostics();
    void addDiagnostic(DiagnosticSeverity severity, const QString& message, const QString& file, int line, int column);
    void setDiagnostics(const std::vector<DiagnosticItem>& items);

    int errorCount() const { return m_errorCount; }
    int warningCount() const { return m_warningCount; }

    void updateTabTitle(QTabWidget* tabWidget, int tabIndex) {
        if (tabWidget && tabIndex >= 0 && tabIndex < tabWidget->count()) {
            tabWidget->setTabText(tabIndex, QString("Problems (%1)").arg(m_errorCount + m_warningCount));
        }
    }

signals:
    void itemSelected(const QString& file, int line, int column);

private slots:
    void onCellDoubleClicked(int row, int column);

private:
    void setupUi();

    QTableWidget* m_table;
    int m_errorCount{0};
    int m_warningCount{0};
    std::vector<DiagnosticItem> m_items;
};

} // namespace liteforge::ui
