#include "liteforge/core/ProjectTemplates.hpp"
#include "liteforge/core/FileUtils.hpp"
#include <QDir>
#include <QFile>

namespace liteforge::core {

QStringList ProjectTemplates::availableTemplates() {
    return QStringList{
        "C++20 CMake Application",
        "Rust Cargo Binary",
        "Python Application",
        "Web Minimal (HTML/CSS/JS)"
    };
}

bool ProjectTemplates::createProject(TemplateType type, const QString& destinationDir, const QString& projectName, QString* error) {
    QDir dest(destinationDir);
    if (!dest.exists()) {
        if (!dest.mkpath(".")) {
            if (error) *error = "Failed to create directory: " + destinationDir;
            return false;
        }
    }

    QString projName = projectName.isEmpty() ? dest.dirName() : projectName;

    switch (type) {
    case TemplateType::CppCMake: {
        dest.mkpath("src");
        dest.mkpath("include");

        QString cmakeContent = QString(
            "cmake_minimum_required(VERSION 3.20)\n"
            "project(%1 LANGUAGES CXX)\n\n"
            "set(CMAKE_CXX_STANDARD 20)\n"
            "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n"
            "add_executable(%1 src/main.cpp)\n"
            "target_include_directories(%1 PRIVATE include)\n"
        ).arg(projName);
        FileUtils::writeFileUtf8(dest.absoluteFilePath("CMakeLists.txt"), cmakeContent);

        QString mainCpp = 
            "#include <iostream>\n\n"
            "int main() {\n"
            "    std::cout << \"Hello from LiteForge C++20!\" << std::endl;\n"
            "    return 0;\n"
            "}\n";
        FileUtils::writeFileUtf8(dest.absoluteFilePath("src/main.cpp"), mainCpp);

        QString gitignore = "build/\n.cache/\n*.o\n*.a\n";
        FileUtils::writeFileUtf8(dest.absoluteFilePath(".gitignore"), gitignore);
        break;
    }

    case TemplateType::RustCargo: {
        dest.mkpath("src");

        QString cargoToml = QString(
            "[package]\n"
            "name = \"%1\"\n"
            "version = \"0.1.0\"\n"
            "edition = \"2021\"\n\n"
            "[dependencies]\n"
        ).arg(projName.toLower().replace(' ', '-'));
        FileUtils::writeFileUtf8(dest.absoluteFilePath("Cargo.toml"), cargoToml);

        QString mainRs = 
            "fn main() {\n"
            "    println!(\"Hello from LiteForge Rust!\");\n"
            "}\n";
        FileUtils::writeFileUtf8(dest.absoluteFilePath("src/main.rs"), mainRs);

        QString gitignore = "/target\nCargo.lock\n**/*.rs.bk\n";
        FileUtils::writeFileUtf8(dest.absoluteFilePath(".gitignore"), gitignore);
        break;
    }

    case TemplateType::PythonApp: {
        QString mainPy = 
            "def main():\n"
            "    print(\"Hello from LiteForge Python!\")\n\n"
            "if __name__ == \"__main__\":\n"
            "    main()\n";
        FileUtils::writeFileUtf8(dest.absoluteFilePath("main.py"), mainPy);
        FileUtils::writeFileUtf8(dest.absoluteFilePath("requirements.txt"), "# Add dependencies here\n");

        QString gitignore = "__pycache__/\n*.py[cod]\n*$py.class\n.venv/\nenv/\n";
        FileUtils::writeFileUtf8(dest.absoluteFilePath(".gitignore"), gitignore);
        break;
    }

    case TemplateType::WebMinimal: {
        QString indexHtml = QString(
            "<!DOCTYPE html>\n"
            "<html lang=\"en\">\n"
            "<head>\n"
            "    <meta charset=\"UTF-8\">\n"
            "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
            "    <title>%1</title>\n"
            "    <link rel=\"stylesheet\" href=\"style.css\">\n"
            "</head>\n"
            "<body>\n"
            "    <h1>Welcome to %1</h1>\n"
            "    <script src=\"app.js\"></script>\n"
            "</body>\n"
            "</html>\n"
        ).arg(projName);
        FileUtils::writeFileUtf8(dest.absoluteFilePath("index.html"), indexHtml);

        QString styleCss = 
            "body {\n"
            "    font-family: system-ui, -apple-system, sans-serif;\n"
            "    margin: 40px;\n"
            "    background: #18181b;\n"
            "    color: #e4e4e7;\n"
            "}\n";
        FileUtils::writeFileUtf8(dest.absoluteFilePath("style.css"), styleCss);

        QString appJs = "console.log('LiteForge Web App Initialized.');\n";
        FileUtils::writeFileUtf8(dest.absoluteFilePath("app.js"), appJs);
        break;
    }
    }

    return true;
}

} // namespace liteforge::core
