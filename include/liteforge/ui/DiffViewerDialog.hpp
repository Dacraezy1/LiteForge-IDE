#pragma once

#include <QDialog>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>

namespace liteforge::ui {

class DiffViewerDialog : public QDialog {
    Q_OBJECT

public:
    explicit DiffViewerDialog(const QString& filePath, const QString& diffText, QWidget* parent = nullptr);
    ~DiffViewerDialog() override = default;

private:
    void setupUi(const QString& filePath, const QString& diffText);
    void applyDiffStyling(const QString& diffText);

    QPlainTextEdit* m_diffEditor{nullptr};
};

} // namespace liteforge::ui
