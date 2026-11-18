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

    bool startServer(const QString& serverExecutable, const QStringList& args, const QString& rootPath);
    void stopServer();
    bool isRunning() const;
    bool isInitialized() const { return m_initialized; }

    // Document sync
    void didOpen(const QString& filePath, const QString& languageId, int version, const QString& text);
    void didChange(const QString& filePath, int version, const QString& text);
    void didClose(const QString& filePath);

    // Language Intelligence requests
    int requestCompletion(const QString& filePath, int line, int character, std::function<void(const std::vector<CompletionItem>&)> callback);
    int requestDefinition(const QString& filePath, int line, int character, std::function<void(const std::vector<Location>&)> callback);
    int requestReferences(const QString& filePath, int line, int character, std::function<void(const std::vector<Location>&)> callback);
    int requestHover(const QString& filePath, int line, int character, std::function<void(const HoverInfo&)> callback);
    int requestRename(const QString& filePath, int line, int character, const QString& newName, std::function<void(const std::vector<TextEdit>&)> callback);
    int requestFormatting(const QString& filePath, std::function<void(const std::vector<TextEdit>&)> callback);
    int requestDocumentSymbols(const QString& filePath, std::function<void(const std::vector<DocumentSymbol>&)> callback);
    int requestWorkspaceSymbols(const QString& query, std::function<void(const std::vector<DocumentSymbol>&)> callback);

    void cancelRequest(int requestId);

signals:
    void diagnosticsReceived(const QString& filePath, const std::vector<Diagnostic>& diagnostics);
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

    QString m_serverExecutable;
    QStringList m_serverArgs;
    QString m_rootPath;
    int m_restartAttempts{0};

    std::map<int, std::function<void(const QJsonObject&)>> m_pendingRequests;
};

} // namespace liteforge::lsp
