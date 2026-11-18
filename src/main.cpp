#include "liteforge/ui/MainWindow.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QDir>
#include <iostream>

int main(int argc, char* argv[]) {
    // Enable High DPI pixmaps and scaling
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("LiteForge");
    app.setApplicationDisplayName("LiteForge IDE");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("LiteForge");
    app.setOrganizationDomain("liteforge.org");

    QCommandLineParser parser;
    parser.setApplicationDescription("LiteForge - The featherweight, zero-bloat native C++ IDE");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("path", "File or directory to open.");

    parser.process(app);

    liteforge::ui::MainWindow mainWindow;
    mainWindow.show();

    const QStringList positionalArgs = parser.positionalArguments();
    if (!positionalArgs.isEmpty()) {
        QString path = positionalArgs.first();
        QFileInfo fi(path);
        if (fi.isDir()) {
            mainWindow.openProject(fi.absoluteFilePath());
        } else if (fi.isFile()) {
            mainWindow.openFile(fi.absoluteFilePath());
        }
    }

    return app.exec();
}
