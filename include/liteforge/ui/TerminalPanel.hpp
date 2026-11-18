#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QProcess>
#include <vector>

namespace liteforge::ui {

class TerminalSession : public QWidget {
    Q_OBJECT

public:
    explicit TerminalSession(const QString& workingDir = QString(), QWidget* parent = nullptr);
    ~TerminalSession() override;

    void setWorkingDirectory(const QString& dir);
    void executeCommand(const QString& cmd);
    void appendOutput(const QString& text);
    void clear();
    void restart();
    bool isRunning() const;

private slots:
    void onInputReturn();
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupUi();
    void startProcess();
    void parseAndAppendAnsi(const QString& rawText);

    QTextEdit* m_output{nullptr};
    QLineEdit* m_input{nullptr};
    QProcess* m_process{nullptr};
    QString m_workingDir;
};

class TerminalPanel : public QWidget {
    Q_OBJECT

public:
    explicit TerminalPanel(QWidget* parent = nullptr);
    ~TerminalPanel() override = default;

    void setWorkingDirectory(const QString& dir);

    void appendBuildOutput(const QString& text, bool isError = false);
    void clearBuildOutput();

    void switchToBuildTab();
    void switchToTerminalTab();

    TerminalSession* createNewTerminal();

signals:
    void errorLineClicked(const QString& file, int line, int col);

private slots:
    void onTabCloseRequested(int index);
    void onBuildOutputClicked();
    void onClearClicked();
    void onRestartClicked();

private:
    void setupUi();

    QTabWidget* m_tabWidget{nullptr};
    QTextEdit* m_buildOutput{nullptr};
    QPushButton* m_newTerminalBtn{nullptr};
    QPushButton* m_clearBtn{nullptr};
    QPushButton* m_restartBtn{nullptr};

    QString m_workingDir;
    int m_terminalCounter{1};
};

} // namespace liteforge::ui
