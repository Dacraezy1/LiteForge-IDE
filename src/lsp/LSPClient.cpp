#include "liteforge/lsp/LSPClient.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace liteforge::lsp {

namespace {
inline QJsonObject dc_obj(const QJsonValue& v) { return v.toObject(); }
}

LSPClient::LSPClient(QObject* parent)
    : QObject(parent) {
}

LSPClient::~LSPClient() {
    stopServer();
}

bool LSPClient::isRunning() const {
    return m_process && m_process->state() == QProcess::Running;
}

bool LSPClient::startServer(const QString& serverExecutable, const QStringList& args, const QString& rootPath) {
    if (isRunning()) {
        stopServer();
    }

    m_serverExecutable = serverExecutable;
    m_serverArgs = args;
    m_rootPath = rootPath;

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

    // Send initialize request with comprehensive client capabilities
    QJsonObject initParams;
    initParams["processId"] = static_cast<int>(m_process->processId());
    initParams["rootUri"] = LSPProtocol::pathToUri(rootPath);

    QJsonObject clientCaps;
    QJsonObject textDocCaps;
    textDocCaps["publishDiagnostics"] = QJsonObject{{"relatedInformation", true}};
    textDocCaps["completion"] = QJsonObject{{"completionItem", QJsonObject{{"snippetSupport", false}}}};
    textDocCaps["hover"] = QJsonObject{{"contentFormat", QJsonArray{"plaintext", "markdown"}}};
    textDocCaps["definition"] = QJsonObject{{"dynamicRegistration", false}};
    textDocCaps["references"] = QJsonObject{{"dynamicRegistration", false}};
    textDocCaps["rename"] = QJsonObject{{"dynamicRegistration", false}};
    textDocCaps["formatting"] = QJsonObject{{"dynamicRegistration", false}};
    textDocCaps["documentSymbol"] = QJsonObject{{"hierarchicalDocumentSymbolSupport", true}};

    clientCaps["textDocument"] = textDocCaps;
    initParams["capabilities"] = clientCaps;

    sendRequest("initialize", initParams, [this](const QJsonObject&) {
        m_initialized = true;
        m_restartAttempts = 0;
        sendNotification("initialized", QJsonObject{});
        emit initialized();
    });

    return true;
}

void LSPClient::stopServer() {
    if (!isRunning()) return;

    m_initialized = false;
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

    m_pendingRequests.clear();
    emit serverStopped();
}

void LSPClient::didOpen(const QString& filePath, const QString& languageId, int version, const QString& text) {
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = LSPProtocol::pathToUri(filePath);
    doc["languageId"] = languageId;
    doc["version"] = version;
    doc["text"] = text;
    params["textDocument"] = doc;

    sendNotification("textDocument/didOpen", params);
}

void LSPClient::didChange(const QString& filePath, int version, const QString& text) {
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = LSPProtocol::pathToUri(filePath);
    doc["version"] = version;
    params["textDocument"] = doc;

    QJsonArray changes;
    QJsonObject change;
    change["text"] = text;
    changes.append(change);
    params["contentChanges"] = changes;

    sendNotification("textDocument/didChange", params);
}

void LSPClient::didClose(const QString& filePath) {
    if (!m_initialized) return;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = LSPProtocol::pathToUri(filePath);
    params["textDocument"] = doc;

    sendNotification("textDocument/didClose", params);
}

int LSPClient::requestCompletion(const QString& filePath, int line, int character, std::function<void(const std::vector<CompletionItem>&)> callback) {
    if (!m_initialized) return -1;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = LSPProtocol::pathToUri(filePath);
    params["textDocument"] = doc;
    params["position"] = Position{line - 1, character - 1}.toJson();

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
        if (callback) callback(items);
    });
}

int LSPClient::requestDefinition(const QString& filePath, int line, int character, std::function<void(const std::vector<Location>&)> callback) {
    if (!m_initialized) return -1;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = LSPProtocol::pathToUri(filePath);
    params["textDocument"] = doc;
    params["position"] = Position{line - 1, character - 1}.toJson();

    return sendRequest("textDocument/definition", params, [callback](const QJsonObject& res) {
        std::vector<Location> locs;
        if (res.contains("result")) {
            QJsonValue val = res["result"];
            if (val.isArray()) {
                for (const auto& v : val.toArray()) {
                    locs.push_back(Location::fromJson(v.toObject()));
                }
            } else if (val.isObject()) {
                locs.push_back(Location::fromJson(val.toObject()));
            }
        }
        if (callback) callback(locs);
    });
}

int LSPClient::requestReferences(const QString& filePath, int line, int character, std::function<void(const std::vector<Location>&)> callback) {
    if (!m_initialized) return -1;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = LSPProtocol::pathToUri(filePath);
    params["textDocument"] = doc;
    params["position"] = Position{line - 1, character - 1}.toJson();
    params["context"] = QJsonObject{{"includeDeclaration", true}};

    return sendRequest("textDocument/references", params, [callback](const QJsonObject& res) {
        std::vector<Location> locs;
        if (res.contains("result") && res["result"].isArray()) {
            for (const auto& v : res["result"].toArray()) {
                locs.push_back(Location::fromJson(v.toObject()));
            }
        }
        if (callback) callback(locs);
    });
}

int LSPClient::requestHover(const QString& filePath, int line, int character, std::function<void(const HoverInfo&)> callback) {
    if (!m_initialized) return -1;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = LSPProtocol::pathToUri(filePath);
    params["textDocument"] = doc;
    params["position"] = Position{line - 1, character - 1}.toJson();

    return sendRequest("textDocument/hover", params, [callback](const QJsonObject& res) {
        HoverInfo hover;
        if (res.contains("result") && res["result"].isObject()) {
            hover = HoverInfo::fromJson(res["result"].toObject());
        }
        if (callback) callback(hover);
    });
}

int LSPClient::requestRename(const QString& filePath, int line, int character, const QString& newName, std::function<void(const std::vector<TextEdit>&)> callback) {
    if (!m_initialized) return -1;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = LSPProtocol::pathToUri(filePath);
    params["textDocument"] = doc;
    params["position"] = Position{line - 1, character - 1}.toJson();
    params["newName"] = newName;

    return sendRequest("textDocument/rename", params, [filePath, callback](const QJsonObject& res) {
        std::vector<TextEdit> edits;
        if (res.contains("result") && res["result"].isObject()) {
            QJsonObject result = res["result"].toObject();
            if (result.contains("changes")) {
                QJsonObject changes = result["changes"].toObject();
                QString uri = LSPProtocol::pathToUri(filePath);
                if (changes.contains(uri) && changes[uri].isArray()) {
                    for (const auto& editVal : changes[uri].toArray()) {
                        edits.push_back(TextEdit::fromJson(editVal.toObject()));
                    }
                }
            }
            // clangd, rust-analyzer, pyright... answer with documentChanges instead.
            if (result.contains("documentChanges")) {
                const QString uri = LSPProtocol::pathToUri(filePath);
                for (const auto& dcVal : result["documentChanges"].toArray()) {
                    const QJsonObject dc = dc_obj(dcVal);
                    if (dc["textDocument"].toObject()["uri"].toString() != uri) continue;
                    for (const auto& editVal : dc["edits"].toArray())
                        edits.push_back(TextEdit::fromJson(editVal.toObject()));
                }
            }
        }
        if (callback) callback(edits);
    });
}

int LSPClient::requestFormatting(const QString& filePath, std::function<void(const std::vector<TextEdit>&)> callback) {
    if (!m_initialized) return -1;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = LSPProtocol::pathToUri(filePath);
    params["textDocument"] = doc;
    params["options"] = QJsonObject{{"tabSize", 4}, {"insertSpaces", true}};

    return sendRequest("textDocument/formatting", params, [callback](const QJsonObject& res) {
        std::vector<TextEdit> edits;
        if (res.contains("result") && res["result"].isArray()) {
            for (const auto& editVal : res["result"].toArray()) {
                edits.push_back(TextEdit::fromJson(editVal.toObject()));
            }
        }
        if (callback) callback(edits);
    });
}

int LSPClient::requestDocumentSymbols(const QString& filePath, std::function<void(const std::vector<DocumentSymbol>&)> callback) {
    if (!m_initialized) return -1;

    QJsonObject params;
    QJsonObject doc;
    doc["uri"] = LSPProtocol::pathToUri(filePath);
    params["textDocument"] = doc;

    return sendRequest("textDocument/documentSymbol", params, [callback](const QJsonObject& res) {
        std::vector<DocumentSymbol> symbols;
        if (res.contains("result") && res["result"].isArray()) {
            for (const auto& sVal : res["result"].toArray()) {
                symbols.push_back(DocumentSymbol::fromJson(sVal.toObject()));
            }
        }
        if (callback) callback(symbols);
    });
}

int LSPClient::requestWorkspaceSymbols(const QString& query, std::function<void(const std::vector<DocumentSymbol>&)> callback) {
    if (!m_initialized) return -1;

    QJsonObject params;
    params["query"] = query;

    return sendRequest("workspace/symbol", params, [callback](const QJsonObject& res) {
        std::vector<DocumentSymbol> symbols;
        if (res.contains("result") && res["result"].isArray()) {
            for (const auto& sVal : res["result"].toArray()) {
                symbols.push_back(DocumentSymbol::fromJson(sVal.toObject()));
            }
        }
        if (callback) callback(symbols);
    });
}

void LSPClient::cancelRequest(int requestId) {
    if (!isRunning()) return;

    auto it = m_pendingRequests.find(requestId);
    if (it != m_pendingRequests.end()) {
        m_pendingRequests.erase(it);
    }

    QJsonObject params;
    params["id"] = requestId;
    sendNotification("$/cancelRequest", params);
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
            QString localPath = LSPProtocol::uriToPath(uri);
            QJsonArray diagsArray = params["diagnostics"].toArray();

            std::vector<Diagnostic> diags;
            for (const auto& dVal : diagsArray) {
                diags.push_back(Diagnostic::fromJson(dVal.toObject()));
            }

            emit diagnosticsReceived(localPath, diags);
        }
    }
}

void LSPClient::onErrorOccurred(QProcess::ProcessError) {
    if (m_process) {
        emit serverError(m_process->errorString());
    }
}

void LSPClient::onFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_initialized = false;
    emit serverStopped();

    // Crash recovery: if exited abnormally, attempt restart
    if (exitStatus == QProcess::CrashExit && m_restartAttempts < 3 && !m_serverExecutable.isEmpty()) {
        m_restartAttempts++;
        startServer(m_serverExecutable, m_serverArgs, m_rootPath);
    }
}

} // namespace liteforge::lsp
