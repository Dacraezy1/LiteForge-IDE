#pragma once

#include "liteforge/build/Debugger.hpp"
#include <QWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QPushButton>
#include <QPlainTextEdit>

namespace liteforge::ui {

class DebuggerPanel : public QWidget {
    Q_OBJECT

public:
    explicit DebuggerPanel(build::Debugger* debugger, QWidget* parent = nullptr);
    ~DebuggerPanel() override = default;

signals:
    void jumpToSourceRequested(const QString& file, int line, int col);

private slots:
    void onSessionStarted();
    void onSessionStopped();
    void onStoppedAt(const QString& file, int line);
    void onStackTraceReady(const std::vector<build::StackFrame>& frames);
    void onVariablesReady(const std::vector<build::VariableInfo>& variables);
    void onStackFrameDoubleClicked(QListWidgetItem* item);

private:
    void setupUi();

    build::Debugger* m_debugger{nullptr};

    QPushButton* m_continueBtn{nullptr};
    QPushButton* m_stepOverBtn{nullptr};
    QPushButton* m_stepIntoBtn{nullptr};
    QPushButton* m_stepOutBtn{nullptr};
    QPushButton* m_pauseBtn{nullptr};
    QPushButton* m_stopBtn{nullptr};

    QListWidget* m_callStackList{nullptr};
    QTreeWidget* m_variablesTree{nullptr};
    QPlainTextEdit* m_consoleOutput{nullptr};
};

} // namespace liteforge::ui
