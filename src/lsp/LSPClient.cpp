#include "liteforge/lsp/LSPClient.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace liteforge::lsp {

LSPClient::LSPClient(QObject* parent)
    : QObject(parent) {
}

LSPClient::~LSPClient() {
    stopServer();
}

bool LSPClient::isRunning() const {
    return m_process && m_process->state() == QProcess::Running;
}

bool LSPClient::startServer(const QString& serverExecutable, const QStringList& args, const QString& rootUri) {
    if (isRunning()) {
        stopServer();
    }

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &LSPClient::onReadyRead);
    connect(m_process, &QProcess::errorOccurred, this, &LSPClient::onErrorOccurred);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &LSPClient::onFinished);

    m_process->start(serverExecutable, args);
    if (!m_process->waitForStarted(2000)) {
        emit serverError("Failed to start LSP server: " + serverExecutable);
        return false;
    }

    emit serverStarted();

    // Send initialize request
    QJsonObject initParams;
    initParams["processId"] = static_cast<int>(m_process->processId());
    initParams["rootUri"] = rootUri.startsWith("file://") ? rootUri : "file://" + rootUri;

    QJsonObject clientCaps;
    QJsonObject textDocCaps;
    textDocCaps["publishDiagnostics"] = QJsonObject{{"relatedInformation", true}};
    clientCaps["textDocument"] = textDocCaps;
    initParams["capabilities"] = clientCaps;

    sendRequest("initialize", initParams, [this](const QJsonObject&) {
        m_initialized = true;
        sendNotification("initialized", QJsonObject{});
    });

    return true;
}

void LSPClient::stopServer() {
    if (!isRunning()) return;

    sendRequest("shutdown", QJsonObject{}, [this](const QJsonObject&) {
        sendNotification("exit", QJsonObject{});
    });

    if (m_process->waitForFinished(1000)) {
        m_process->deleteLater();
        m_process = nullptr;
    } else {
        m_process->kill();
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_initialized = false;
    emit serverStopped();
}

void LSPClient::didOpen(const QString& uri, const QString& languageId, int version, const QString& text) {
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = uri.startsWith("file://") ? uri : "file://" + uri;
    doc["languageId"] = languageId;
    doc["version"] = version;
    doc["text"] = text;
    params["textDocument"] = doc;

    sendNotification("textDocument/didOpen", params);
}

void LSPClient::didChange(const QString& uri, int version, const QString& text) {
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = uri.startsWith("file://") ? uri : "file://" + uri;
    doc["version"] = version;
    params["textDocument"] = doc;

    QJsonArray changes;
    QJsonObject change;
    change["text"] = text;
    changes.append(change);
    params["contentChanges"] = changes;

    sendNotification("textDocument/didChange", params);
}

void LSPClient::didClose(const QString& uri) {
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = uri.startsWith("file://") ? uri : "file://" + uri;
    params["textDocument"] = doc;

    sendNotification("textDocument/didClose", params);
}

int LSPClient::requestCompletion(const QString& uri, int line, int character, std::function<void(const std::vector<CompletionItem>&)> callback) {
    if (!m_initialized) return -1;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = uri.startsWith("file://") ? uri : "file://" + uri;
    params["textDocument"] = doc;
    params["position"] = Position{line, character}.toJson();

    return sendRequest("textDocument/completion", params, [callback](const QJsonObject& res) {
        std::vector<CompletionItem> items;
        if (res.contains("result")) {
            QJsonValue val = res["result"];
            QJsonArray arr;
            if (val.isArray()) {
                arr = val.toArray();
            } else if (val.isObject() && val.toObject().contains("items")) {
                arr = val.toObject()["items"].toArray();
            }
            for (const auto& itemVal : arr) {
                items.push_back(CompletionItem::fromJson(itemVal.toObject()));
            }
        }
        if (callback) {
            callback(items);
        }
    });
}

void LSPClient::sendNotification(const QString& method, const QJsonObject& params) {
    if (!isRunning()) return;

    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["method"] = method;
    msg["params"] = params;

    QByteArray data = LSPProtocol::encodeMessage(msg);
    m_process->write(data);
}

int LSPClient::sendRequest(const QString& method, const QJsonObject& params, std::function<void(const QJsonObject&)> callback) {
    if (!isRunning()) return -1;

    int reqId = m_nextRequestId++;
    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["id"] = reqId;
    msg["method"] = method;
    msg["params"] = params;

    if (callback) {
        m_pendingRequests[reqId] = callback;
    }

    QByteArray data = LSPProtocol::encodeMessage(msg);
    m_process->write(data);
    return reqId;
}

void LSPClient::onReadyRead() {
    m_incomingBuffer.append(m_process->readAllStandardOutput());
    processIncomingBuffer();
}

void LSPClient::processIncomingBuffer() {
    while (true) {
        int headerEnd = m_incomingBuffer.indexOf("\r\n\r\n");
        if (headerEnd == -1) break;

        QByteArray header = m_incomingBuffer.left(headerEnd);
        int contentLength = 0;

        for (const QByteArray& line : header.split('\n')) {
            QByteArray trimmed = line.trimmed();
            if (trimmed.startsWith("Content-Length:")) {
                contentLength = trimmed.mid(15).trimmed().toInt();
                break;
            }
        }

        if (contentLength <= 0) {
            m_incomingBuffer.remove(0, headerEnd + 4);
            continue;
        }

        int totalMessageLength = headerEnd + 4 + contentLength;
        if (m_incomingBuffer.size() < totalMessageLength) {
            break;
        }

        QByteArray body = m_incomingBuffer.mid(headerEnd + 4, contentLength);
        m_incomingBuffer.remove(0, totalMessageLength);

        QJsonDocument doc = QJsonDocument::fromJson(body);
        if (doc.isObject()) {
            handleMessage(doc.object());
        }
    }
}

void LSPClient::handleMessage(const QJsonObject& message) {
    if (message.contains("id") && !message.contains("method")) {
        int id = message["id"].toInt();
        auto it = m_pendingRequests.find(id);
        if (it != m_pendingRequests.end()) {
            it->second(message);
            m_pendingRequests.erase(it);
        }
        return;
    }

    if (message.contains("method")) {
        QString method = message["method"].toString();
        if (method == "textDocument/publishDiagnostics") {
            QJsonObject params = message["params"].toObject();
            QString uri = params["uri"].toString();
            QJsonArray diagsArray = params["diagnostics"].toArray();

            std::vector<Diagnostic> diags;
            for (const auto& dVal : diagsArray) {
                diags.push_back(Diagnostic::fromJson(dVal.toObject()));
            }

            emit diagnosticsReceived(uri, diags);
        }
    }
}

void LSPClient::onErrorOccurred(QProcess::ProcessError) {
    if (m_process) {
        emit serverError(m_process->errorString());
    }
}

void LSPClient::onFinished(int, QProcess::ExitStatus) {
    emit serverStopped();
}

} // namespace liteforge::lsp
