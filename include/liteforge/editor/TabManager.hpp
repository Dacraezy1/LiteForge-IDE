#pragma once

#include "liteforge/editor/CodeEditor.hpp"
#include <QTabWidget>
#include <vector>
#include <memory>

namespace liteforge::editor {

class TabManager : public QTabWidget {
    Q_OBJECT

public:
    explicit TabManager(QWidget* parent = nullptr);
    ~TabManager() override = default;

    CodeEditor* openFile(const QString& filePath);
    CodeEditor* newUntitledDocument();

    CodeEditor* currentEditor() const;
    CodeEditor* editorAt(int index) const;

    bool closeFile(int index);
    bool closeAllFiles();
    bool closeOtherFiles(int index);
    bool saveCurrentFile();
    bool saveFileAt(int index);
    bool saveAllFiles();

    void applyTheme(const core::ThemeColors& theme);
    void updateFontSettings(const QString& family, int size);

    QStringList openFilePaths() const;
    void closeActiveTab();

signals:
    void activeEditorChanged(CodeEditor* editor);
    void fileOpened(const QString& filePath);
    void allFilesClosed();

private slots:
    void onTabCloseRequested(int index);
    void onCurrentChanged(int index);
    void showContextMenu(const QPoint& pos);

private:
    void updateTabTitle(int index);
    int findTabIndexByPath(const QString& filePath) const;
};

} // namespace liteforge::editor
