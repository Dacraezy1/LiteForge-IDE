#pragma once

#include "liteforge/ui/DiagnosticsPanel.hpp"
#include <QObject>
#include <QProcess>
#include <memory>

namespace liteforge::build {

enum class BuildTarget {
    Build,
    Run,
    Clean,
    Rebuild
};

class BuildSystem : public QObject {
    Q_OBJECT

public:
    explicit BuildSystem(QObject* parent = nullptr);
    ~BuildSystem() override;

    bool isRunning() const;
    void startBuild(const QString& projectRoot, const QString& activeFile, BuildTarget target);
    void stopBuild();

signals:
    void outputReceived(const QString& text, bool isError);
    void diagnosticFound(ui::DiagnosticSeverity severity, const QString& message, const QString& file, int line, int column);
    void buildStarted(const QString& targetDescription);
    void buildFinished(bool success, int exitCode);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void parseOutputLine(const QString& line);

    QProcess* m_process{nullptr};
    QString m_projectRoot;
    QString m_activeFile;
    BuildTarget m_currentTarget{BuildTarget::Build};
};

} // namespace liteforge::build
