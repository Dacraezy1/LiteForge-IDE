#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QLabel>
#include <QProgressBar>

namespace liteforge::ui {

class FindInFilesDialog : public QDialog {
    Q_OBJECT

public:
    explicit FindInFilesDialog(QWidget* parent = nullptr);
    ~FindInFilesDialog() override = default;

    void setProjectRoot(const QString& rootPath);
    void showDialog(const QString& initialText = QString());

signals:
    void resultSelected(const QString& filePath, int lineNumber);

private slots:
    void startSearch();
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);

private:
    void setupUi();

    QString m_projectRoot;

    QLineEdit* m_searchEdit;
    QLineEdit* m_filterEdit;
    QCheckBox* m_caseSensitiveCheck;
    QCheckBox* m_wholeWordCheck;
    QCheckBox* m_regexCheck;

    QPushButton* m_searchButton;
    QPushButton* m_cancelButton;

    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QTreeWidget* m_resultsTree;
};

} // namespace liteforge::ui
