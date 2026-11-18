#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

namespace liteforge::lsp {

struct Position {
    int line{0};
    int character{0};

    QJsonObject toJson() const {
        return QJsonObject{{"line", line}, {"character", character}};
    }
};

struct Range {
    Position start;
    Position end;

    static Range fromJson(const QJsonObject& obj) {
        Range r;
        if (obj.contains("start")) {
            auto s = obj["start"].toObject();
            r.start.line = s["line"].toInt();
            r.start.character = s["character"].toInt();
        }
        if (obj.contains("end")) {
            auto e = obj["end"].toObject();
            r.end.line = e["line"].toInt();
            r.end.character = e["character"].toInt();
        }
        return r;
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

class LSPProtocol {
public:
    static QByteArray encodeMessage(const QJsonObject& json) {
        QJsonDocument doc(json);
        QByteArray body = doc.toJson(QJsonDocument::Compact);
        QByteArray header = QString("Content-Length: %1\r\n\r\n").arg(body.size()).toUtf8();
        return header + body;
    }
};

} // namespace liteforge::lsp
