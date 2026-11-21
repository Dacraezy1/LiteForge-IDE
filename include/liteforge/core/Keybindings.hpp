#pragma once

#include <QString>
#include <QMap>

namespace liteforge::core {

// User keybinding overrides, stored as flat JSON: { "File/Save": "Ctrl+S", "Edit/Find": "" }
// Action ids are "<Menu>/<Action>" with '&' accelerators and trailing "..." removed.
// An empty string unbinds the action.
class Keybindings {
public:
    static QString makeId(const QString& menuTitle, const QString& actionText);

    static QMap<QString, QString> load(const QString& path);
    static bool save(const QString& path, const QMap<QString, QString>& bindings);
};

} // namespace liteforge::core
