#include "liteforge/build/Debugger.hpp"
#include <QRegularExpression>
#include <QFileInfo>

namespace liteforge::build {

Debugger::Debugger(QObject* parent)
    : QObject(parent) {
}

Debugger::~Debugger() {
    stopDebugging();
}

bool Debugger::isRunning() const {
    return m_process && m_process->state() == QProcess::Running;
}

bool Debugger::startDebugging(const QString& executablePath, const QString& workingDir) {
    if (isRunning()) {
        stopDebugging();
    }

    if (!QFileInfo::exists(executablePath)) {
        emit consoleOutput("Executable does not exist: " + executablePath);
        return false;
    }

    m_executablePath = executablePath;
    m_process = new QProcess(this);
    if (!workingDir.isEmpty()) {
        m_process->setWorkingDirectory(workingDir);
    }

    connect(m_process, &QProcess::readyReadStandardOutput, this, &Debugger::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &Debugger::onReadyReadStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Debugger::onFinished);

    // Launch GDB with MI2 interpreter
    m_process->start("gdb", QStringList{"--interpreter=mi2", "-q", executablePath});
    if (!m_process->waitForStarted(2000)) {
        emit consoleOutput("Failed to start GDB. Ensure gdb is installed and in PATH.");
        m_process->deleteLater();
        m_process = nullptr;
        return false;
    }

    emit sessionStarted();

    // Re-apply pending breakpoints
    for (const auto& bp : m_pendingBreakpoints) {
        sendCommand(QString("-break-insert %1:%2").arg(bp.file).arg(bp.line));
    }

    // Start program execution
    sendCommand("-exec-run");
    return true;
}

void Debugger::stopDebugging() {
    if (!isRunning()) return;

    sendCommand("-gdb-exit");
    if (!m_process->waitForFinished(1000)) {
        m_process->kill();
    }
    m_process->deleteLater();
    m_process = nullptr;
    emit sessionStopped();
}

void Debugger::continueExecution() {
    sendCommand("-exec-continue");
}

void Debugger::stepOver() {
    sendCommand("-exec-next");
}

void Debugger::stepInto() {
    sendCommand("-exec-step");
}

void Debugger::stepOut() {
    sendCommand("-exec-finish");
}

void Debugger::pauseExecution() {
    sendCommand("-exec-interrupt");
}

void Debugger::addBreakpoint(const QString& file, int line) {
    QString key = QString("%1:%2").arg(file).arg(line);
    if (m_breakpointIdMap.find(key) != m_breakpointIdMap.end()) return;

    BreakpointInfo bp;
    bp.id = m_nextBreakpointId++;
    bp.file = file;
    bp.line = line;
    bp.enabled = true;
    m_pendingBreakpoints.push_back(bp);
    m_breakpointIdMap[key] = bp.id;

    if (isRunning()) {
        sendCommand(QString("-break-insert %1:%2").arg(file).arg(line));
    }
}

void Debugger::removeBreakpoint(const QString& file, int line) {
    QString key = QString("%1:%2").arg(file).arg(line);
    auto it = m_breakpointIdMap.find(key);
    if (it != m_breakpointIdMap.end()) {
        int id = it->second;
        m_breakpointIdMap.erase(it);

        for (auto bpIt = m_pendingBreakpoints.begin(); bpIt != m_pendingBreakpoints.end(); ++bpIt) {
            if (bpIt->id == id) {
                m_pendingBreakpoints.erase(bpIt);
                break;
            }
        }

        if (isRunning()) {
            sendCommand(QString("-break-delete %1").arg(id));
        }
    }
}

void Debugger::clearAllBreakpoints() {
    m_pendingBreakpoints.clear();
    m_breakpointIdMap.clear();
    if (isRunning()) {
        sendCommand("-break-delete");
    }
}

void Debugger::requestStackTrace() {
    sendCommand("-stack-list-frames");
}

void Debugger::requestVariables() {
    sendCommand("-stack-list-variables --simple-values");
}

void Debugger::evaluateExpression(const QString& expr) {
    sendCommand(QString("-data-evaluate-expression \"%1\"").arg(expr));
}

void Debugger::sendCommand(const QString& cmd) {
    if (!isRunning()) return;
    QByteArray line = (cmd + "\n").toUtf8();
    m_process->write(line);
}

void Debugger::onReadyReadStdout() {
    while (m_process && m_process->canReadLine()) {
        QString line = QString::fromUtf8(m_process->readLine()).trimmed();
        parseMiOutput(line);
    }
}

void Debugger::onReadyReadStderr() {
    if (m_process) {
        emit consoleOutput(QString::fromUtf8(m_process->readAllStandardError()));
    }
}

void Debugger::onFinished(int, QProcess::ExitStatus) {
    emit sessionStopped();
}

void Debugger::parseMiOutput(const QString& line) {
    if (line.isEmpty() || line == "(gdb)") return;

    emit consoleOutput(line);

    // *stopped,reason="breakpoint-hit",...,frame={...,file="...",line="..."}
    if (line.startsWith("*stopped")) {
        static QRegularExpression fileRe("fullname=\"([^\"]+)\"");
        static QRegularExpression lineRe("line=\"(\\d+)\"");

        auto fMatch = fileRe.match(line);
        auto lMatch = lineRe.match(line);

        if (fMatch.hasMatch() && lMatch.hasMatch()) {
            QString file = fMatch.captured(1);
            int lineNum = lMatch.captured(1).toInt();
            emit stoppedAt(file, lineNum);
            requestStackTrace();
            requestVariables();
        }
    }
    // ^done,stack=[frame={level="0",addr="0x...",func="...",file="...",fullname="...",line="..."}]
    else if (line.startsWith("^done,stack=")) {
        std::vector<StackFrame> frames;
        static QRegularExpression frameRe("frame=\\{level=\"(\\d+)\",addr=\"([^\"]+)\",func=\"([^\"]+)\"(?:,file=\"([^\"]+)\")?(?:,fullname=\"([^\"]+)\")?(?:,line=\"(\\d+)\")?");
        auto it = frameRe.globalMatch(line);
        while (it.hasNext()) {
            auto m = it.next();
            StackFrame sf;
            sf.level = m.captured(1).toInt();
            sf.address = m.captured(2);
            sf.function = m.captured(3);
            sf.file = m.captured(5).isEmpty() ? m.captured(4) : m.captured(5);
            sf.line = m.captured(6).toInt();
            frames.push_back(sf);
        }
        emit stackTraceReady(frames);
    }
    // ^done,variables=[{name="...",type="...",value="..."}]
    else if (line.startsWith("^done,variables=")) {
        std::vector<VariableInfo> vars;
        static QRegularExpression varRe("\\{name=\"([^\"]+)\",type=\"([^\"]+)\",value=\"([^\"]+)\"\\}");
        auto it = varRe.globalMatch(line);
        while (it.hasNext()) {
            auto m = it.next();
            VariableInfo vi;
            vi.name = m.captured(1);
            vi.type = m.captured(2);
            vi.value = m.captured(3);
            vars.push_back(vi);
        }
        emit variablesReady(vars);
    }
}

} // namespace liteforge::build
