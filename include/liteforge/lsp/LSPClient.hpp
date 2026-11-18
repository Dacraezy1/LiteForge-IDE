#pragma once

#include "liteforge/lsp/LSPProtocol.hpp"
#include <QObject>
#include <QProcess>
#include <QByteArray>
#include <map>
#include <functional>
#include <vector>

namespace liteforge::lsp {

class LSPClient : public QObject {
    Q_OBJECT

public:
    explicit LSPClient(QObject* parent = nullptr);
    ~LSPClient() override;

    bool startServer(const QString& serverExecutable, const QStringList& args, const QString& rootUri);
    void stopServer();
    bool isRunning() const;

    void didOpen(const QString& uri, const QString& languageId, int version, const QString& text);
    void didChange(const QString& uri, int version, const QString& text);
    void didClose(const QString& uri);

    int requestCompletion(const QString& uri, int line, int character, std::function<void(const std::vector<CompletionItem>&)> callback);

signals:
    void diagnosticsReceived(const QString& uri, const std::vector<Diagnostic>& diagnostics);
    void serverStarted();
    void serverStopped();
    void serverError(const QString& error);

private slots:
    void onReadyRead();
    void onErrorOccurred(QProcess::ProcessError error);
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void sendNotification(const QString& method, const QJsonObject& params);
    int sendRequest(const QString& method, const QJsonObject& params, std::function<void(const QJsonObject&)> callback);
    void processIncomingBuffer();
    void handleMessage(const QJsonObject& message);

    QProcess* m_process{nullptr};
    QByteArray m_incomingBuffer;
    int m_nextRequestId{1};
    bool m_initialized{false};

    std::map<int, std::function<void(const QJsonObject&)>> m_pendingRequests;
};

} // namespace liteforge::lsp
