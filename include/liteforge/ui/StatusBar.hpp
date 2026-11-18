#pragma once

#include <QStatusBar>
#include <QLabel>
#include <QPushButton>

namespace liteforge::ui {

class StatusBar : public QStatusBar {
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent = nullptr);
    ~StatusBar() override = default;

    void setGitBranch(const QString& branch);
    void setCursorPosition(int line, int col, int totalLines = 0, int totalChars = 0);
    void setEncoding(const QString& encoding);
    void setLineEnding(const QString& le);
    void setLanguage(const QString& lang);
    void setTabInfo(int tabWidth, bool spaces);
    void setBuildStatus(const QString& status, bool isError = false, bool isBusy = false);
    void updatePerformanceStats();

signals:
    void branchClicked();
    void languageClicked();
    void lineEndingClicked();

private:
    void setupUi();

    QPushButton* m_gitBtn;
    QLabel* m_cursorLabel;
    QLabel* m_fileInfoLabel;
    QPushButton* m_encodingBtn;
    QPushButton* m_lineEndingBtn;
    QPushButton* m_languageBtn;
    QLabel* m_tabInfoLabel;
    QLabel* m_buildStatusLabel;
    QLabel* m_perfLabel;
};

} // namespace liteforge::ui
