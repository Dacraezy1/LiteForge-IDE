#include "liteforge/ui/TerminalPanel.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTabBar>
#include <cstdlib>

namespace liteforge::ui {

TerminalSession::TerminalSession(const QString& workingDir, QWidget* parent)
    : QWidget(parent), m_workingDir(workingDir) {
    setupUi();
    startProcess();
}

TerminalSession::~TerminalSession() {
    if (m_process) {
        m_process->terminate();
        m_process->waitForFinished(400);
        m_process->kill();
    }
}

void TerminalSession::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    m_output = new QTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setFontFamily("Monospace");
    m_output->setFontPointSize(10);
    m_output->setStyleSheet("background-color: #121214; color: #d4d4d8; border: none;");

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Enter terminal command...");
    m_input->setFont(QFont("Monospace", 10));
    m_input->setStyleSheet("background-color: #18181b; color: #ffffff; border: 1px solid #27272a; padding: 4px;");

    layout->addWidget(m_output);
    layout->addWidget(m_input);

    connect(m_input, &QLineEdit::returnPressed, this, &TerminalSession::onInputReturn);
}

void TerminalSession::startProcess() {
    if (m_process) {
        m_process->deleteLater();
    }

    m_process = new QProcess(this);
    if (!m_workingDir.isEmpty()) {
        m_process->setWorkingDirectory(m_workingDir);
    }

    connect(m_process, &QProcess::readyReadStandardOutput, this, &TerminalSession::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &TerminalSession::onReadyReadStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &TerminalSession::onProcessFinished);

    const char* shellEnv = std::getenv("SHELL");
    QString shell = shellEnv ? QString::fromUtf8(shellEnv) : QString("/bin/sh");

    m_process->start(shell, QStringList() << "-i");
}

void TerminalSession::setWorkingDirectory(const QString& dir) {
    m_workingDir = dir;
    if (m_process && m_process->state() == QProcess::Running) {
        executeCommand(QString("cd \"%1\"").arg(dir));
    }
}

void TerminalSession::executeCommand(const QString& cmd) {
    if (m_process && m_process->state() == QProcess::Running) {
        appendOutput("$ " + cmd + "\n");
        m_process->write((cmd + "\n").toUtf8());
    } else {
        appendOutput("[Shell is not running. Click Restart]\n");
    }
}

void TerminalSession::onInputReturn() {
    QString text = m_input->text();
    if (text.isEmpty()) return;

    m_input->clear();
    executeCommand(text);
}

void TerminalSession::onReadyReadStdout() {
    QByteArray data = m_process->readAllStandardOutput();
    parseAndAppendAnsi(QString::fromUtf8(data));
}

void TerminalSession::onReadyReadStderr() {
    QByteArray data = m_process->readAllStandardError();
    parseAndAppendAnsi(QString::fromUtf8(data));
}

void TerminalSession::onProcessFinished(int exitCode, QProcess::ExitStatus) {
    appendOutput(QString("\n[Shell exited with status %1]\n").arg(exitCode));
}

void TerminalSession::clear() {
    m_output->clear();
}

void TerminalSession::restart() {
    appendOutput("\n[Restarting session...]\n");
    startProcess();
}

bool TerminalSession::isRunning() const {
    return m_process && m_process->state() == QProcess::Running;
}

void TerminalSession::appendOutput(const QString& text) {
    m_output->moveCursor(QTextCursor::End);
    m_output->insertPlainText(text);
    m_output->moveCursor(QTextCursor::End);
}

void TerminalSession::parseAndAppendAnsi(const QString& rawText) {
    static QRegularExpression ansiRegex(QStringLiteral("\x1B\\[[0-9;]*[a-zA-Z]"));
    QString clean = rawText;
    clean.remove(ansiRegex);

    m_output->moveCursor(QTextCursor::End);
    m_output->insertPlainText(clean);
    m_output->moveCursor(QTextCursor::End);
}

// -------------------------------------------------------------
// TerminalPanel Implementation
// -------------------------------------------------------------

TerminalPanel::TerminalPanel(QWidget* parent)
    : QWidget(parent) {
    setupUi();
    createNewTerminal();
}

void TerminalPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);

    auto* header = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 2, 4, 2);
    headerLayout->setSpacing(6);

    m_newTerminalBtn = new QPushButton("+ New Terminal", header);
    m_clearBtn = new QPushButton("Clear", header);
    m_restartBtn = new QPushButton("Restart", header);

    headerLayout->addWidget(m_newTerminalBtn);
    headerLayout->addStretch();
    headerLayout->addWidget(m_clearBtn);
    headerLayout->addWidget(m_restartBtn);

    mainLayout->addWidget(header);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);

    // Tab 0: Build Output (non-closable)
    m_buildOutput = new QTextEdit(this);
    m_buildOutput->setReadOnly(true);
    m_buildOutput->setFontFamily("Monospace");
    m_buildOutput->setFontPointSize(10);
    m_buildOutput->setStyleSheet("background-color: #121214; color: #d4d4d8; border: none;");

    m_tabWidget->addTab(m_buildOutput, "Build Output");

    // Hide close button on tab 0 (Build Output)
    if (auto* tb = m_tabWidget->tabBar()) {
        tb->setTabButton(0, QTabBar::RightSide, nullptr);
    }

    mainLayout->addWidget(m_tabWidget);

    connect(m_newTerminalBtn, &QPushButton::clicked, this, &TerminalPanel::createNewTerminal);
    connect(m_clearBtn, &QPushButton::clicked, this, &TerminalPanel::onClearClicked);
    connect(m_restartBtn, &QPushButton::clicked, this, &TerminalPanel::onRestartClicked);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &TerminalPanel::onTabCloseRequested);
    connect(m_buildOutput, &QTextEdit::selectionChanged, this, &TerminalPanel::onBuildOutputClicked);
}

TerminalSession* TerminalPanel::createNewTerminal() {
    auto* session = new TerminalSession(m_workingDir, this);
    QString title = QString("Terminal %1").arg(m_terminalCounter++);
    int idx = m_tabWidget->addTab(session, title);
    m_tabWidget->setCurrentIndex(idx);
    return session;
}

void TerminalPanel::setWorkingDirectory(const QString& dir) {
    m_workingDir = dir;
    for (int i = 1; i < m_tabWidget->count(); ++i) {
        if (auto* session = qobject_cast<TerminalSession*>(m_tabWidget->widget(i))) {
            session->setWorkingDirectory(dir);
        }
    }
}

void TerminalPanel::appendBuildOutput(const QString& text, bool isError) {
    m_buildOutput->moveCursor(QTextCursor::End);
    if (isError) {
        m_buildOutput->setTextColor(QColor("#f87171"));
    } else if (text.contains("warning:", Qt::CaseInsensitive)) {
        m_buildOutput->setTextColor(QColor("#fbbf24"));
    } else if (text.contains("Built target", Qt::CaseInsensitive) || text.contains("success", Qt::CaseInsensitive)) {
        m_buildOutput->setTextColor(QColor("#34d399"));
    } else {
        m_buildOutput->setTextColor(QColor("#d4d4d8"));
    }
    m_buildOutput->insertPlainText(text);
    m_buildOutput->moveCursor(QTextCursor::End);
}

void TerminalPanel::clearBuildOutput() {
    m_buildOutput->clear();
}

void TerminalPanel::switchToBuildTab() {
    m_tabWidget->setCurrentIndex(0);
}

void TerminalPanel::switchToTerminalTab() {
    if (m_tabWidget->count() > 1) {
        m_tabWidget->setCurrentIndex(1);
    } else {
        createNewTerminal();
    }
}

void TerminalPanel::onTabCloseRequested(int index) {
    if (index == 0) return;
    QWidget* w = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    delete w;
}

void TerminalPanel::onClearClicked() {
    int current = m_tabWidget->currentIndex();
    if (current == 0) {
        m_buildOutput->clear();
    } else if (auto* session = qobject_cast<TerminalSession*>(m_tabWidget->currentWidget())) {
        session->clear();
    }
}

void TerminalPanel::onRestartClicked() {
    int current = m_tabWidget->currentIndex();
    if (current > 0) {
        if (auto* session = qobject_cast<TerminalSession*>(m_tabWidget->currentWidget())) {
            session->restart();
        }
    }
}

void TerminalPanel::onBuildOutputClicked() {
    QTextCursor cur = m_buildOutput->textCursor();
    QString lineText = cur.block().text();

    static QRegularExpression errorRegex(QStringLiteral(R"(([a-zA-Z0-9_\-\./]+):(\d+):(\d+):)"));
    QRegularExpressionMatch match = errorRegex.match(lineText);
    if (match.hasMatch()) {
        QString file = match.captured(1);
        int line = match.captured(2).toInt();
        int col = match.captured(3).toInt();
        emit errorLineClicked(file, line, col);
    }
}

} // namespace liteforge::ui
