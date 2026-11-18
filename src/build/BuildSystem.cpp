#include "liteforge/build/BuildSystem.hpp"
#include "liteforge/core/Config.hpp"
#include <QRegularExpression>
#include <QFileInfo>

namespace liteforge::build {

BuildSystem::BuildSystem(QObject* parent)
    : QObject(parent) {
}

BuildSystem::~BuildSystem() {
    stopBuild();
}

bool BuildSystem::isRunning() const {
    return m_process && m_process->state() == QProcess::Running;
}

void BuildSystem::stopBuild() {
    if (m_process && m_process->state() == QProcess::Running) {
        emit outputReceived("\n[Build process stopped by user]\n", true);
        m_process->kill();
        m_process->waitForFinished(500);
    }
}

void BuildSystem::startBuild(const QString& projectRoot, const QString& activeFile, BuildTarget target) {
    if (isRunning()) {
        emit outputReceived("Build is already running!\n", true);
        return;
    }

    m_projectRoot = projectRoot;
    m_activeFile = activeFile;
    m_currentTarget = target;

    if (m_process) {
        m_process->deleteLater();
    }

    m_process = new QProcess(this);
    if (!m_projectRoot.isEmpty()) {
        m_process->setWorkingDirectory(m_projectRoot);
    } else if (!m_activeFile.isEmpty()) {
        m_process->setWorkingDirectory(QFileInfo(m_activeFile).absolutePath());
    }

    connect(m_process, &QProcess::readyReadStandardOutput, this, &BuildSystem::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &BuildSystem::onReadyReadStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &BuildSystem::onProcessFinished);

    QString program;
    QStringList args;

    const auto& customCfg = core::Config::instance().build();

    // Check if custom build command is provided
    if (target == BuildTarget::Build && !customCfg.customBuildCommand.isEmpty()) {
        program = "/bin/sh";
        args << "-c" << customCfg.customBuildCommand;
        emit buildStarted("Custom Build: " + customCfg.customBuildCommand);
        emit outputReceived("$ " + customCfg.customBuildCommand + "\n", false);
        m_process->start(program, args);
        return;
    }

    if (target == BuildTarget::Run && !customCfg.customRunCommand.isEmpty()) {
        program = "/bin/sh";
        args << "-c" << customCfg.customRunCommand;
        emit buildStarted("Custom Run: " + customCfg.customRunCommand);
        emit outputReceived("$ " + customCfg.customRunCommand + "\n", false);
        m_process->start(program, args);
        return;
    }

    // Auto-detect build system
    bool hasCMake = !m_projectRoot.isEmpty() && QFile::exists(m_projectRoot + "/CMakeLists.txt");
    bool hasMake = !m_projectRoot.isEmpty() && (QFile::exists(m_projectRoot + "/Makefile") || QFile::exists(m_projectRoot + "/GNUmakefile"));
    bool hasCargo = !m_projectRoot.isEmpty() && QFile::exists(m_projectRoot + "/Cargo.toml");

    if (hasCMake) {
        if (target == BuildTarget::Build) {
            program = "cmake";
            args << "--build" << "build";
            emit buildStarted("CMake Build");
        } else if (target == BuildTarget::Clean) {
            program = "cmake";
            args << "--build" << "build" << "--target" << "clean";
            emit buildStarted("CMake Clean");
        } else if (target == BuildTarget::Rebuild) {
            program = "sh";
            args << "-c" << "cmake -B build && cmake --build build --clean-first";
            emit buildStarted("CMake Rebuild");
        } else if (target == BuildTarget::Run) {
            program = "sh";
            args << "-c" << "cmake --build build && ctest --test-dir build || ./build/$(ls build | grep -v CMakeFiles | head -n 1)";
            emit buildStarted("CMake Run");
        }
    } else if (hasMake) {
        program = "make";
        if (target == BuildTarget::Clean) args << "clean";
        else if (target == BuildTarget::Rebuild) args << "clean" << "all";
        emit buildStarted("Make " + (args.isEmpty() ? QString("all") : args.join(" ")));
    } else if (hasCargo) {
        program = "cargo";
        if (target == BuildTarget::Run) args << "run";
        else if (target == BuildTarget::Clean) args << "clean";
        else args << "build";
        emit buildStarted("Cargo " + args.join(" "));
    } else if (!m_activeFile.isEmpty()) {
        // Single file build
        QFileInfo fi(m_activeFile);
        QString ext = fi.suffix().toLower();
        if (ext == "cpp" || ext == "cc" || ext == "cxx") {
            program = "g++";
            QString binName = fi.completeBaseName();
            args << "-std=c++20" << "-O2" << fi.fileName() << "-o" << binName;
            emit buildStarted("Compile " + fi.fileName());
        } else if (ext == "c") {
            program = "gcc";
            QString binName = fi.completeBaseName();
            args << "-O2" << fi.fileName() << "-o" << binName;
            emit buildStarted("Compile " + fi.fileName());
        } else if (ext == "py") {
            program = "python3";
            args << fi.fileName();
            emit buildStarted("Run Python: " + fi.fileName());
        } else if (ext == "sh") {
            program = "bash";
            args << fi.fileName();
            emit buildStarted("Run Bash: " + fi.fileName());
        }
    }

    if (program.isEmpty()) {
        emit outputReceived("No recognized project build system (CMake, Make, Cargo) or active compilable file found.\n", true);
        emit buildFinished(false, -1);
        return;
    }

    emit outputReceived(QString("$ %1 %2\n").arg(program, args.join(" ")), false);
    m_process->start(program, args);
}

void BuildSystem::onReadyReadStdout() {
    QByteArray data = m_process->readAllStandardOutput();
    QString text = QString::fromUtf8(data);
    emit outputReceived(text, false);

    QStringList lines = text.split('\n');
    for (const QString& line : lines) {
        parseOutputLine(line);
    }
}

void BuildSystem::onReadyReadStderr() {
    QByteArray data = m_process->readAllStandardError();
    QString text = QString::fromUtf8(data);
    emit outputReceived(text, true);

    QStringList lines = text.split('\n');
    for (const QString& line : lines) {
        parseOutputLine(line);
    }
}

void BuildSystem::parseOutputLine(const QString& line) {
    // Regex for standard compiler error/warning output:
    // /path/to/file.cpp:12:34: error: message
    static QRegularExpression re(QStringLiteral(R"(([a-zA-Z0-9_\-\./]+):(\d+):(\d+):\s*(error|warning|note):\s*(.*))"));
    QRegularExpressionMatch match = re.match(line);
    if (match.hasMatch()) {
        QString file = match.captured(1);
        int lineNum = match.captured(2).toInt();
        int colNum = match.captured(3).toInt();
        QString type = match.captured(4);
        QString message = match.captured(5);

        ui::DiagnosticSeverity sev = ui::DiagnosticSeverity::Information;
        if (type == "error") {
            sev = ui::DiagnosticSeverity::Error;
        } else if (type == "warning") {
            sev = ui::DiagnosticSeverity::Warning;
        }

        emit diagnosticFound(sev, message, file, lineNum, colNum);
    }
}

void BuildSystem::onProcessFinished(int exitCode, QProcess::ExitStatus) {
    bool ok = (exitCode == 0);
    if (ok) {
        emit outputReceived(QString("\n[Build completed successfully in %1]\n").arg(m_projectRoot), false);
    } else {
        emit outputReceived(QString("\n[Build failed with exit code %1]\n").arg(exitCode), true);
    }
    emit buildFinished(ok, exitCode);
}

} // namespace liteforge::build
