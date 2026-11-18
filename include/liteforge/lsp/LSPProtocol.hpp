#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>
#include <vector>
#include <map>

namespace liteforge::lsp {

struct Position {
    int line{0};
    int character{0};

    QJsonObject toJson() const {
        return QJsonObject{{"line", line}, {"character", character}};
    }

    static Position fromJson(const QJsonObject& obj) {
        Position p;
        p.line = obj["line"].toInt(0);
        p.character = obj["character"].toInt(0);
        return p;
    }
};

struct Range {
    Position start;
    Position end;

    QJsonObject toJson() const {
        return QJsonObject{{"start", start.toJson()}, {"end", end.toJson()}};
    }

    static Range fromJson(const QJsonObject& obj) {
        Range r;
        if (obj.contains("start")) r.start = Position::fromJson(obj["start"].toObject());
        if (obj.contains("end")) r.end = Position::fromJson(obj["end"].toObject());
        return r;
    }
};

struct Location {
    QString uri;
    Range range;

    QString localPath() const {
        return QUrl(uri).toLocalFile();
    }

    static Location fromJson(const QJsonObject& obj) {
        Location loc;
        loc.uri = obj["uri"].toString();
        loc.range = Range::fromJson(obj["range"].toObject());
        return loc;
    }
};

struct Diagnostic {
    Range range;
    int severity{1}; // 1 = Error, 2 = Warning, 3 = Info, 4 = Hint
    QString message;
    QString source;

    static Diagnostic fromJson(const QJsonObject& obj) {
        Diagnostic d;
        d.range = Range::fromJson(obj["range"].toObject());
        d.severity = obj["severity"].toInt(1);
        d.message = obj["message"].toString();
        d.source = obj["source"].toString();
        return d;
    }
};

struct CompletionItem {
    QString label;
    QString detail;
    QString documentation;
    QString insertText;
    int kind{0};

    static CompletionItem fromJson(const QJsonObject& obj) {
        CompletionItem item;
        item.label = obj["label"].toString();
        item.detail = obj["detail"].toString();
        item.insertText = obj.contains("insertText") ? obj["insertText"].toString() : item.label;
        if (obj.contains("documentation")) {
            if (obj["documentation"].isString()) {
                item.documentation = obj["documentation"].toString();
            } else if (obj["documentation"].isObject()) {
                item.documentation = obj["documentation"].toObject()["value"].toString();
            }
        }
        item.kind = obj["kind"].toInt(0);
        return item;
    }
};

struct HoverInfo {
    QString contents;
    Range range;
    bool valid{false};

    static HoverInfo fromJson(const QJsonObject& obj) {
        HoverInfo h;
        h.valid = true;
        if (obj.contains("range")) {
            h.range = Range::fromJson(obj["range"].toObject());
        }
        if (obj.contains("contents")) {
            auto val = obj["contents"];
            if (val.isString()) {
                h.contents = val.toString();
            } else if (val.isObject()) {
                h.contents = val.toObject()["value"].toString();
            } else if (val.isArray()) {
                QStringList parts;
                for (const auto& item : val.toArray()) {
                    if (item.isString()) parts.append(item.toString());
                    else if (item.isObject()) parts.append(item.toObject()["value"].toString());
                }
                h.contents = parts.join("\n\n");
            }
        }
        return h;
    }
};

struct TextEdit {
    Range range;
    QString newText;

    static TextEdit fromJson(const QJsonObject& obj) {
        TextEdit te;
        te.range = Range::fromJson(obj["range"].toObject());
        te.newText = obj["newText"].toString();
        return te;
    }
};

struct DocumentSymbol {
    QString name;
    QString detail;
    int kind{0};
    Range range;
    Range selectionRange;
    std::vector<DocumentSymbol> children;

    static DocumentSymbol fromJson(const QJsonObject& obj) {
        DocumentSymbol s;
        s.name = obj["name"].toString();
        s.detail = obj["detail"].toString();
        s.kind = obj["kind"].toInt(0);
        s.range = Range::fromJson(obj["range"].toObject());
        s.selectionRange = Range::fromJson(obj["selectionRange"].toObject());
        if (obj.contains("children") && obj["children"].isArray()) {
            for (const auto& child : obj["children"].toArray()) {
                s.children.push_back(fromJson(child.toObject()));
            }
        }
        return s;
    }
};

class LSPProtocol {
public:
    static QString pathToUri(const QString& localPath) {
        return QUrl::fromLocalFile(localPath).toString();
    }

    static QString uriToPath(const QString& uri) {
        return QUrl(uri).toLocalFile();
    }

    static QByteArray encodeMessage(const QJsonObject& json) {
        QJsonDocument doc(json);
        QByteArray body = doc.toJson(QJsonDocument::Compact);
        QByteArray header = QString("Content-Length: %1\r\n\r\n").arg(body.size()).toUtf8();
        return header + body;
    }
};

} // namespace liteforge::lsp
