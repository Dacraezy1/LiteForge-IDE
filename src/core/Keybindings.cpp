#include "liteforge/core/Keybindings.hpp"
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace liteforge::core {

namespace {
QString clean(QString s) {
    s.remove(QLatin1Char('&'));
    if (s.endsWith(QStringLiteral("..."))) s.chop(3);
    return s.trimmed();
}
} // namespace

QString Keybindings::makeId(const QString& menuTitle, const QString& actionText) {
    return clean(menuTitle) + QLatin1Char('/') + clean(actionText);
}

QMap<QString, QString> Keybindings::load(const QString& path) {
    QMap<QString, QString> out;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return out;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return out;
    const QJsonObject o = doc.object();
    for (auto it = o.begin(); it != o.end(); ++it)
        if (it.value().isString()) out.insert(it.key(), it.value().toString());
    return out;
}

bool Keybindings::save(const QString& path, const QMap<QString, QString>& bindings) {
    QJsonObject o;
    for (auto it = bindings.cbegin(); it != bindings.cend(); ++it) o.insert(it.key(), it.value());
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    return f.commit();
}

} // namespace liteforge::core
