#include "liteforge/ui/GitPanel.hpp"
#include "liteforge/ui/DiffViewerDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>

namespace liteforge::ui {

GitPanel::GitPanel(git::GitService* gitService, QWidget* parent)
    : QWidget(parent), m_gitService(gitService) {
    setupUi();

    if (m_gitService) {
        connect(m_gitService, &git::GitService::statusUpdated, this, &GitPanel::onStatusUpdated);
        connect(m_gitService, &git::GitService::branchChanged, this, &GitPanel::onBranchChanged);
        connect(m_gitService, &git::GitService::operationFinished, this, [this](const QString&, bool success, const QString& msg) {
            if (m_statusMsgLabel) {
                m_statusMsgLabel->setText(success ? msg.left(80) : "Failed: " + msg.left(80));
            }
        });
    }
}

void GitPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // --- Branch & Action Bar ---
    auto* topBar = new QHBoxLayout();
    topBar->setSpacing(4);

    auto* branchLabel = new QLabel("Branch:", this);
    topBar->addWidget(branchLabel);

    m_branchCombo = new QComboBox(this);
    m_branchCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(m_branchCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GitPanel::onBranchSelected);
    topBar->addWidget(m_branchCombo);

    m_newBranchBtn = new QPushButton("+", this);
    m_newBranchBtn->setToolTip("Create New Branch");
    m_newBranchBtn->setFixedWidth(28);
    connect(m_newBranchBtn, &QPushButton::clicked, this, &GitPanel::onNewBranchClicked);
    topBar->addWidget(m_newBranchBtn);

    m_refreshBtn = new QPushButton("Refresh", this);
    m_refreshBtn->setToolTip("Refresh Git Status");
    m_refreshBtn->setFixedWidth(28);
    connect(m_refreshBtn, &QPushButton::clicked, this, &GitPanel::refresh);
    topBar->addWidget(m_refreshBtn);

    mainLayout->addLayout(topBar);

    // --- Sync Bar (Pull / Push) ---
    auto* syncBar = new QHBoxLayout();
    syncBar->setSpacing(6);

    m_pullBtn = new QPushButton("Pull", this);
    connect(m_pullBtn, &QPushButton::clicked, this, &GitPanel::onPullClicked);
    syncBar->addWidget(m_pullBtn);

    m_pushBtn = new QPushButton("Push", this);
    connect(m_pushBtn, &QPushButton::clicked, this, &GitPanel::onPushClicked);
    syncBar->addWidget(m_pushBtn);

    mainLayout->addLayout(syncBar);

    // --- Tree Widget (Staged / Changes) ---
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &GitPanel::onItemDoubleClicked);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &GitPanel::showContextMenu);

    m_stagedHeader = new QTreeWidgetItem(m_treeWidget);
    m_stagedHeader->setText(0, "Staged Changes (0)");
    m_stagedHeader->setExpanded(true);
    QFont f = m_stagedHeader->font(0);
    f.setBold(true);
    m_stagedHeader->setFont(0, f);

    m_changesHeader = new QTreeWidgetItem(m_treeWidget);
    m_changesHeader->setText(0, "Changes (0)");
    m_changesHeader->setExpanded(true);
    m_changesHeader->setFont(0, f);

    mainLayout->addWidget(m_treeWidget);

    // --- Commit Section ---
    auto* commitLayout = new QVBoxLayout();
    commitLayout->setSpacing(4);

    m_commitInput = new QLineEdit(this);
    m_commitInput->setPlaceholderText("Commit message (Ctrl+Enter to commit)...");
    connect(m_commitInput, &QLineEdit::returnPressed, this, &GitPanel::onCommitClicked);
    commitLayout->addWidget(m_commitInput);

    auto* commitBtnLayout = new QHBoxLayout();
    m_commitBtn = new QPushButton("Commit", this);
    connect(m_commitBtn, &QPushButton::clicked, this, &GitPanel::onCommitClicked);
    commitBtnLayout->addWidget(m_commitBtn);

    auto* stageAllBtn = new QPushButton("Stage All", this);
    connect(stageAllBtn, &QPushButton::clicked, this, &GitPanel::onStageAllClicked);
    commitBtnLayout->addWidget(stageAllBtn);

    auto* unstageAllBtn = new QPushButton("Unstage All", this);
    connect(unstageAllBtn, &QPushButton::clicked, this, &GitPanel::onUnstageAllClicked);
    commitBtnLayout->addWidget(unstageAllBtn);

    commitLayout->addLayout(commitBtnLayout);

    m_statusMsgLabel = new QLabel(this);
    m_statusMsgLabel->setStyleSheet("color: #888888; font-size: 11px;");
    commitLayout->addWidget(m_statusMsgLabel);

    mainLayout->addLayout(commitLayout);
}

void GitPanel::refresh() {
    if (m_gitService) {
        m_gitService->refreshBranches();
        m_gitService->refreshStatus();
    }
}

void GitPanel::onBranchChanged(const QString& branch) {
    if (!m_gitService) return;

    m_branchCombo->blockSignals(true);
    m_branchCombo->clear();
    const QStringList branches = m_gitService->availableBranches();
    for (const QString& b : branches) {
        m_branchCombo->addItem(b);
    }
    int idx = m_branchCombo->findText(branch);
    if (idx != -1) {
        m_branchCombo->setCurrentIndex(idx);
    }
    m_branchCombo->blockSignals(false);
}

void GitPanel::onStatusUpdated() {
    updateFileList();
}

void GitPanel::updateFileList() {
    if (!m_gitService) return;

    // Clear child items
    qDeleteAll(m_stagedHeader->takeChildren());
    qDeleteAll(m_changesHeader->takeChildren());

    int stagedCount = 0;
    int changesCount = 0;

    const auto files = m_gitService->statusFiles();
    for (const auto& f : files) {
        QString badge;
        switch (f.status) {
        case git::FileGitStatus::Modified: badge = "[M] "; break;
        case git::FileGitStatus::Added:    badge = "[A] "; break;
        case git::FileGitStatus::Deleted:  badge = "[D] "; break;
        case git::FileGitStatus::Renamed:  badge = "[R] "; break;
        case git::FileGitStatus::Untracked:badge = "[U] "; break;
        default: badge = "[?] "; break;
        }

        auto* item = new QTreeWidgetItem();
        item->setText(0, badge + f.filePath);
        item->setData(0, Qt::UserRole, f.filePath);
        item->setData(0, Qt::UserRole + 1, f.isStaged);

        if (f.isStaged) {
            m_stagedHeader->addChild(item);
            stagedCount++;
        } else {
            m_changesHeader->addChild(item);
            changesCount++;
        }
    }

    m_stagedHeader->setText(0, QString("Staged Changes (%1)").arg(stagedCount));
    m_changesHeader->setText(0, QString("Changes (%1)").arg(changesCount));
}

void GitPanel::onCommitClicked() {
    if (!m_gitService) return;
    QString msg = m_commitInput->text().trimmed();
    if (msg.isEmpty()) {
        QMessageBox::warning(this, "Git Commit", "Please enter a commit message.");
        return;
    }

    m_gitService->commit(msg, [this](bool ok, const QString& result) {
        if (ok) {
            m_commitInput->clear();
        } else {
            QMessageBox::warning(this, "Git Commit Error", result);
        }
    });
}

void GitPanel::onStageAllClicked() {
    if (m_gitService) m_gitService->stageAll();
}

void GitPanel::onUnstageAllClicked() {
    if (m_gitService) m_gitService->unstageAll();
}

void GitPanel::onPullClicked() {
    if (m_gitService) {
        m_gitService->pull([this](bool ok, const QString& res) {
            if (!ok) {
                QMessageBox::warning(this, "Git Pull Error", res);
            }
        });
    }
}

void GitPanel::onPushClicked() {
    if (m_gitService) {
        m_gitService->push([this](bool ok, const QString& res) {
            if (!ok) {
                QMessageBox::warning(this, "Git Push Error", res);
            }
        });
    }
}

void GitPanel::onBranchSelected(int index) {
    if (!m_gitService || index < 0) return;
    QString branch = m_branchCombo->itemText(index);
    if (!branch.isEmpty() && branch != m_gitService->currentBranch()) {
        m_gitService->switchBranch(branch, [this](bool ok, const QString& res) {
            if (!ok) {
                QMessageBox::warning(this, "Branch Switch Error", res);
                refresh();
            }
        });
    }
}

void GitPanel::onNewBranchClicked() {
    if (!m_gitService) return;
    bool ok = false;
    QString branchName = QInputDialog::getText(this, "Create Branch", "New branch name:", QLineEdit::Normal, "", &ok);
    if (ok && !branchName.trimmed().isEmpty()) {
        m_gitService->createBranch(branchName.trimmed(), [this](bool success, const QString& res) {
            if (!success) {
                QMessageBox::warning(this, "Create Branch Error", res);
            }
        });
    }
}

void GitPanel::onItemDoubleClicked(QTreeWidgetItem* item, int) {
    if (!item || !m_gitService) return;
    QString path = item->data(0, Qt::UserRole).toString();
    if (path.isEmpty()) return;

    bool isStaged = item->data(0, Qt::UserRole + 1).toBool();

    m_gitService->getFileDiff(path, isStaged, [this, path](const QString& diff) {
        DiffViewerDialog dialog(path, diff, this);
        dialog.exec();
    });
}

void GitPanel::showContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    if (!item) return;

    QString path = item->data(0, Qt::UserRole).toString();
    if (path.isEmpty()) return;

    bool isStaged = item->data(0, Qt::UserRole + 1).toBool();

    QMenu menu(this);
    if (isStaged) {
        menu.addAction("Unstage File", [this, path]() {
            if (m_gitService) m_gitService->unstageFile(path);
        });
    } else {
        menu.addAction("Stage File", [this, path]() {
            if (m_gitService) m_gitService->stageFile(path);
        });
        menu.addAction("Discard Changes", [this, path]() {
            auto ans = QMessageBox::question(this, "Discard Changes",
                                             "Are you sure you want to discard changes in:\n" + path + "\nThis cannot be undone.",
                                             QMessageBox::Yes | QMessageBox::No);
            if (ans == QMessageBox::Yes && m_gitService) {
                m_gitService->discardChanges(path);
            }
        });
    }

    menu.addAction("View Diff", [this, path, isStaged]() {
        if (m_gitService) {
            m_gitService->getFileDiff(path, isStaged, [this, path](const QString& diff) {
                DiffViewerDialog dialog(path, diff, this);
                dialog.exec();
            });
        }
    });

    menu.exec(m_treeWidget->viewport()->mapToGlobal(pos));
}

} // namespace liteforge::ui
