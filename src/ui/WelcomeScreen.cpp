#include "liteforge/ui/WelcomeScreen.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileInfo>

namespace liteforge::ui {

WelcomeScreen::WelcomeScreen(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void WelcomeScreen::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setContentsMargins(40, 40, 40, 40);

    auto* container = new QWidget(this);
    container->setMaximumWidth(680);
    auto* containerLayout = new QVBoxLayout(container);
    containerLayout->setSpacing(20);

    // Header
    auto* titleLabel = new QLabel("LiteForge IDE", container);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(26);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    containerLayout->addWidget(titleLabel);

    auto* subtitleLabel = new QLabel("Featherweight, Native C++20 & Qt6 Development Environment", container);
    subtitleLabel->setStyleSheet("color: #888888; font-size: 13px;");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    containerLayout->addWidget(subtitleLabel);

    containerLayout->addSpacing(10);

    // Two-column layout: Start & Recent
    auto* columnsLayout = new QHBoxLayout();
    columnsLayout->setSpacing(30);

    // Column 1: Start
    auto* startCol = new QVBoxLayout();
    startCol->setSpacing(10);

    auto* startHeader = new QLabel("Start", container);
    QFont headerFont = startHeader->font();
    headerFont.setPointSize(14);
    headerFont.setBold(true);
    startHeader->setFont(headerFont);
    startCol->addWidget(startHeader);

    auto* newFileBtn = new QPushButton("New File", container);
    newFileBtn->setStyleSheet("text-align: left; padding: 8px 12px;");
    connect(newFileBtn, &QPushButton::clicked, this, &WelcomeScreen::newFileRequested);
    startCol->addWidget(newFileBtn);

    auto* openFileBtn = new QPushButton("Open File...", container);
    openFileBtn->setStyleSheet("text-align: left; padding: 8px 12px;");
    connect(openFileBtn, &QPushButton::clicked, this, &WelcomeScreen::openFileRequested);
    startCol->addWidget(openFileBtn);

    auto* openProjBtn = new QPushButton("Open Project Folder...", container);
    openProjBtn->setStyleSheet("text-align: left; padding: 8px 12px;");
    connect(openProjBtn, &QPushButton::clicked, this, &WelcomeScreen::openProjectRequested);
    startCol->addWidget(openProjBtn);

    startCol->addStretch();
    columnsLayout->addLayout(startCol, 1);

    // Column 2: Recent Projects
    auto* recentCol = new QVBoxLayout();
    recentCol->setSpacing(10);

    auto* recentHeader = new QLabel("Recent Projects", container);
    recentHeader->setFont(headerFont);
    recentCol->addWidget(recentHeader);

    m_recentList = new QListWidget(container);
    m_recentList->setFrameShape(QFrame::NoFrame);
    m_recentList->setStyleSheet("QListWidget::item { padding: 8px; border-radius: 4px; }"
                                "QListWidget::item:hover { background-color: rgba(255, 255, 255, 0.08); }");
    connect(m_recentList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (item) {
            emit projectSelected(item->data(Qt::UserRole).toString());
        }
    });
    recentCol->addWidget(m_recentList);

    columnsLayout->addLayout(recentCol, 1);

    containerLayout->addLayout(columnsLayout);

    mainLayout->addWidget(container);
}

void WelcomeScreen::updateRecentProjects(const QStringList& projects) {
    m_recentList->clear();
    if (projects.isEmpty()) {
        auto* empty = new QListWidgetItem("No recent projects", m_recentList);
        empty->setFlags(Qt::NoItemFlags);
        return;
    }

    for (const QString& path : projects) {
        QFileInfo fi(path);
        auto* item = new QListWidgetItem(fi.fileName() + "\n" + path, m_recentList);
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
    }
}

} // namespace liteforge::ui
