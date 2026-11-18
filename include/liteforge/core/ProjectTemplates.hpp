#pragma once

#include <QString>
#include <QStringList>

namespace liteforge::core {

enum class TemplateType {
    CppCMake,
    RustCargo,
    PythonApp,
    WebMinimal
};

struct TemplateInfo {
    TemplateType type;
    QString name;
    QString description;
};

class ProjectTemplates {
public:
    static QStringList availableTemplates();
    static bool createProject(TemplateType type, const QString& destinationDir, const QString& projectName, QString* error = nullptr);
};

} // namespace liteforge::core
