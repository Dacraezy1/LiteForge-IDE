#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <vector>
#include <map>

namespace liteforge::build {

struct StackFrame {
    int level{0};
    QString function;
    QString file;
    int line{0};
    QString address;
};

struct VariableInfo {
    QString name;
    QString type;
    QString value;
};

struct BreakpointInfo {
    int id{0};
    QString file;
    int line{0};
    bool enabled{true};
};

class Debugger : public QObject {
    Q_OBJECT

public:
    explicit Debugger(QObject* parent = nullptr);
    ~Debugger() override;

    bool isRunning() const;

    bool startDebugging(const QString& executablePath, const QString& workingDir = QString());
    void stopDebugging();

    // Controls
    void continueExecution();
    void stepOver();
    void stepInto();
    void stepOut();
    void pauseExecution();

    // Breakpoints
    void addBreakpoint(const QString& file, int line);
    void removeBreakpoint(const QString& file, int line);
    void clearAllBreakpoints();

    // Queries
    void requestStackTrace();
    void requestVariables();
    void evaluateExpression(const QString& expr);

signals:
    void sessionStarted();
    void sessionStopped();
    void stoppedAt(const QString& file, int line);
    void stackTraceReady(const std::vector<StackFrame>& frames);
    void variablesReady(const std::vector<VariableInfo>& variables);
    void expressionEvaluated(const QString& expr, const QString& result);
    void consoleOutput(const QString& text);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void sendCommand(const QString& cmd);
    void parseMiOutput(const QString& line);

    QProcess* m_process{nullptr};
    QString m_executablePath;
    std::vector<BreakpointInfo> m_pendingBreakpoints;
    std::map<QString, int> m_breakpointIdMap; // "file:line" -> id
    int m_nextBreakpointId{1};
};

} // namespace liteforge::build
