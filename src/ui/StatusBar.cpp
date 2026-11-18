#include "liteforge/ui/StatusBar.hpp"
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <sys/time.h>
#include <sys/resource.h>

namespace liteforge::ui {

StatusBar::StatusBar(QWidget* parent)
    : QStatusBar(parent) {
    setupUi();
    updatePerformanceStats();
}

void StatusBar::setupUi() {
    setSizeGripEnabled(false);

    m_gitBtn = new QPushButton("⎇ No Git", this);
    m_gitBtn->setToolTip("Current Git Branch");
    addWidget(m_gitBtn);

    m_buildStatusLabel = new QLabel("Ready", this);
    m_buildStatusLabel->setStyleSheet("color: #34d399; font-weight: 500;");
    addWidget(m_buildStatusLabel);

    // Right aligned permanent widgets
    m_cursorLabel = new QLabel("Ln 1, Col 1", this);
    m_cursorLabel->setMinimumWidth(90);
    addPermanentWidget(m_cursorLabel);

    m_fileInfoLabel = new QLabel("0 lines", this);
    addPermanentWidget(m_fileInfoLabel);

    m_tabInfoLabel = new QLabel("Spaces: 4", this);
    addPermanentWidget(m_tabInfoLabel);

    m_encodingBtn = new QPushButton("UTF-8", this);
    addPermanentWidget(m_encodingBtn);

    m_lineEndingBtn = new QPushButton("LF", this);
    addPermanentWidget(m_lineEndingBtn);

    m_languageBtn = new QPushButton("Plain Text", this);
    addPermanentWidget(m_languageBtn);

    m_perfLabel = new QLabel("RAM: ~35 MB | Native C++", this);
    m_perfLabel->setStyleSheet("color: #38bdf8; font-weight: bold;");
    m_perfLabel->setToolTip("Featherweight zero-bloat native C++ performance");
    addPermanentWidget(m_perfLabel);

    connect(m_gitBtn, &QPushButton::clicked, this, &StatusBar::branchClicked);
    connect(m_languageBtn, &QPushButton::clicked, this, &StatusBar::languageClicked);
    connect(m_lineEndingBtn, &QPushButton::clicked, this, &StatusBar::lineEndingClicked);
}

void StatusBar::setGitBranch(const QString& branch) {
    if (branch.isEmpty()) {
        m_gitBtn->setText("⎇ No Git");
    } else {
        m_gitBtn->setText("⎇ " + branch);
    }
}

void StatusBar::setCursorPosition(int line, int col, int totalLines, int totalChars) {
    m_cursorLabel->setText(QString("Ln %1, Col %2").arg(line).arg(col));
    m_fileInfoLabel->setText(QString("%1 lines, %2 chars").arg(totalLines).arg(totalChars));
}

void StatusBar::setEncoding(const QString& encoding) {
    m_encodingBtn->setText(encoding);
}

void StatusBar::setLineEnding(const QString& le) {
    m_lineEndingBtn->setText(le);
}

void StatusBar::setLanguage(const QString& lang) {
    m_languageBtn->setText(lang);
}

void StatusBar::setTabInfo(int tabWidth, bool spaces) {
    m_tabInfoLabel->setText(QString("%1: %2").arg(spaces ? "Spaces" : "Tab").arg(tabWidth));
}

void StatusBar::setBuildStatus(const QString& status, bool isError, bool isBusy) {
    m_buildStatusLabel->setText(status);
    if (isBusy) {
        m_buildStatusLabel->setStyleSheet("color: #fbbf24; font-weight: 500;");
    } else if (isError) {
        m_buildStatusLabel->setStyleSheet("color: #f87171; font-weight: 500;");
    } else {
        m_buildStatusLabel->setStyleSheet("color: #34d399; font-weight: 500;");
    }
}

void StatusBar::updatePerformanceStats() {
    int vmRssKb = 0;

#if defined(__linux__)
    QFile file("/proc/self/status");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("VmRSS:")) {
                QString val = line.section(':', 1).trimmed().section(' ', 0, 0);
                vmRssKb = val.toInt();
                break;
            }
        }
    }
#endif

    // Fallback for FreeBSD, OpenBSD, and POSIX systems
    if (vmRssKb == 0) {
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
#if defined(__APPLE__)
            vmRssKb = static_cast<int>(usage.ru_maxrss / 1024);
#else
            vmRssKb = static_cast<int>(usage.ru_maxrss);
#endif
        }
    }

    if (vmRssKb > 0) {
        double mb = vmRssKb / 1024.0;
        m_perfLabel->setText(QString("RAM: %1 MB | Native C++").arg(mb, 0, 'f', 1));
    } else {
        m_perfLabel->setText("RAM: ~35 MB | Native C++");
    }
}

} // namespace liteforge::ui
