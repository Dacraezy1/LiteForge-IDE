#pragma once

#include "liteforge/git/GitService.hpp"
#include <QWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <memory>

namespace liteforge::ui {

class GitPanel : public QWidget {
    Q_OBJECT

public:
    explicit GitPanel(git::GitService* gitService, QWidget* parent = nullptr);
    ~GitPanel() override = default;

public slots:
    void refresh();

signals:
    void fileSelected(const QString& relativePath);

private slots:
    void onStatusUpdated();
    void onBranchChanged(const QString& branch);
    void onCommitClicked();
    void onStageAllClicked();
    void onUnstageAllClicked();
    void onPullClicked();
    void onPushClicked();
    void onBranchSelected(int index);
    void onNewBranchClicked();
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void showContextMenu(const QPoint& pos);

private:
    void setupUi();
    void updateFileList();

    git::GitService* m_gitService{nullptr};

    QComboBox* m_branchCombo{nullptr};
    QPushButton* m_newBranchBtn{nullptr};
    QPushButton* m_pullBtn{nullptr};
    QPushButton* m_pushBtn{nullptr};
    QPushButton* m_refreshBtn{nullptr};

    QLineEdit* m_commitInput{nullptr};
    QPushButton* m_commitBtn{nullptr};

    QTreeWidget* m_treeWidget{nullptr};
    QTreeWidgetItem* m_stagedHeader{nullptr};
    QTreeWidgetItem* m_changesHeader{nullptr};

    QLabel* m_statusMsgLabel{nullptr};
};

} // namespace liteforge::ui
