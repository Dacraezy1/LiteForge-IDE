#include "liteforge/ui/DebuggerPanel.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QLabel>

namespace liteforge::ui {

DebuggerPanel::DebuggerPanel(build::Debugger* debugger, QWidget* parent)
    : QWidget(parent), m_debugger(debugger) {
    setupUi();

    if (m_debugger) {
        connect(m_debugger, &build::Debugger::sessionStarted, this, &DebuggerPanel::onSessionStarted);
        connect(m_debugger, &build::Debugger::sessionStopped, this, &DebuggerPanel::onSessionStopped);
        connect(m_debugger, &build::Debugger::stoppedAt, this, &DebuggerPanel::onStoppedAt);
        connect(m_debugger, &build::Debugger::stackTraceReady, this, &DebuggerPanel::onStackTraceReady);
        connect(m_debugger, &build::Debugger::variablesReady, this, &DebuggerPanel::onVariablesReady);
        connect(m_debugger, &build::Debugger::consoleOutput, this, [this](const QString& text) {
            m_consoleOutput->appendPlainText(text);
        });
    }

    onSessionStopped();
}

void DebuggerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(4);

    m_continueBtn = new QPushButton("▶ Continue", this);
    connect(m_continueBtn, &QPushButton::clicked, this, [this]() { if (m_debugger) m_debugger->continueExecution(); });
    toolbar->addWidget(m_continueBtn);

    m_stepOverBtn = new QPushButton("↷ Step Over", this);
    connect(m_stepOverBtn, &QPushButton::clicked, this, [this]() { if (m_debugger) m_debugger->stepOver(); });
    toolbar->addWidget(m_stepOverBtn);

    m_stepIntoBtn = new QPushButton("⇲ Step Into", this);
    connect(m_stepIntoBtn, &QPushButton::clicked, this, [this]() { if (m_debugger) m_debugger->stepInto(); });
    toolbar->addWidget(m_stepIntoBtn);

    m_stepOutBtn = new QPushButton("⇱ Step Out", this);
    connect(m_stepOutBtn, &QPushButton::clicked, this, [this]() { if (m_debugger) m_debugger->stepOut(); });
    toolbar->addWidget(m_stepOutBtn);

    m_pauseBtn = new QPushButton("⏸ Pause", this);
    connect(m_pauseBtn, &QPushButton::clicked, this, [this]() { if (m_debugger) m_debugger->pauseExecution(); });
    toolbar->addWidget(m_pauseBtn);

    m_stopBtn = new QPushButton("⏹ Stop", this);
    connect(m_stopBtn, &QPushButton::clicked, this, [this]() { if (m_debugger) m_debugger->stopDebugging(); });
    toolbar->addWidget(m_stopBtn);

    toolbar->addStretch();
    mainLayout->addLayout(toolbar);

    // Splitter: Call Stack (Left) | Variables (Center) | Console (Right)
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Call stack
    auto* stackWidget = new QWidget(splitter);
    auto* stackLayout = new QVBoxLayout(stackWidget);
    stackLayout->setContentsMargins(0, 0, 0, 0);
    stackLayout->addWidget(new QLabel("<b>Call Stack</b>", stackWidget));
    m_callStackList = new QListWidget(stackWidget);
    connect(m_callStackList, &QListWidget::itemDoubleClicked, this, &DebuggerPanel::onStackFrameDoubleClicked);
    stackLayout->addWidget(m_callStackList);
    splitter->addWidget(stackWidget);

    // Variables
    auto* varWidget = new QWidget(splitter);
    auto* varLayout = new QVBoxLayout(varWidget);
    varLayout->setContentsMargins(0, 0, 0, 0);
    varLayout->addWidget(new QLabel("<b>Variables</b>", varWidget));
    m_variablesTree = new QTreeWidget(varWidget);
    m_variablesTree->setHeaderLabels(QStringList{"Name", "Value", "Type"});
    m_variablesTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    varLayout->addWidget(m_variablesTree);
    splitter->addWidget(varWidget);

    // Console
    auto* consoleWidget = new QWidget(splitter);
    auto* consoleLayout = new QVBoxLayout(consoleWidget);
    consoleLayout->setContentsMargins(0, 0, 0, 0);
    consoleLayout->addWidget(new QLabel("<b>GDB Console</b>", consoleWidget));
    m_consoleOutput = new QPlainTextEdit(consoleWidget);
    m_consoleOutput->setReadOnly(true);
    consoleLayout->addWidget(m_consoleOutput);
    splitter->addWidget(consoleWidget);

    mainLayout->addWidget(splitter);
}

void DebuggerPanel::onSessionStarted() {
    m_continueBtn->setEnabled(true);
    m_stepOverBtn->setEnabled(true);
    m_stepIntoBtn->setEnabled(true);
    m_stepOutBtn->setEnabled(true);
    m_pauseBtn->setEnabled(true);
    m_stopBtn->setEnabled(true);
    m_consoleOutput->clear();
    m_callStackList->clear();
    m_variablesTree->clear();
}

void DebuggerPanel::onSessionStopped() {
    m_continueBtn->setEnabled(false);
    m_stepOverBtn->setEnabled(false);
    m_stepIntoBtn->setEnabled(false);
    m_stepOutBtn->setEnabled(false);
    m_pauseBtn->setEnabled(false);
    m_stopBtn->setEnabled(false);
}

void DebuggerPanel::onStoppedAt(const QString& file, int line) {
    emit jumpToSourceRequested(file, line, 1);
}

void DebuggerPanel::onStackTraceReady(const std::vector<build::StackFrame>& frames) {
    m_callStackList->clear();
    for (const auto& sf : frames) {
        QString text = QString("#%1 %2 in %3:%4").arg(sf.level).arg(sf.function).arg(sf.file).arg(sf.line);
        auto* item = new QListWidgetItem(text, m_callStackList);
        item->setData(Qt::UserRole, sf.file);
        item->setData(Qt::UserRole + 1, sf.line);
    }
}

void DebuggerPanel::onVariablesReady(const std::vector<build::VariableInfo>& variables) {
    m_variablesTree->clear();
    for (const auto& vi : variables) {
        auto* item = new QTreeWidgetItem(m_variablesTree);
        item->setText(0, vi.name);
        item->setText(1, vi.value);
        item->setText(2, vi.type);
    }
}

void DebuggerPanel::onStackFrameDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    QString file = item->data(Qt::UserRole).toString();
    int line = item->data(Qt::UserRole + 1).toInt();
    if (!file.isEmpty() && line > 0) {
        emit jumpToSourceRequested(file, line, 1);
    }
}

} // namespace liteforge::ui
