#pragma once

#include "liteforge/editor/TabManager.hpp"
#include <QWidget>
#include <QSplitter>

namespace liteforge::editor {

class EditorSplitter : public QWidget {
    Q_OBJECT

public:
    explicit EditorSplitter(QWidget* parent = nullptr);
    ~EditorSplitter() override = default;

    CodeEditor* openFile(const QString& filePath);
    CodeEditor* openFilePreview(const QString& filePath);
    CodeEditor* newUntitledDocument();

    CodeEditor* currentEditor() const;
    CodeEditor* editorForPath(const QString& path) const;
    TabManager* activeTabManager() const;

    void split(Qt::Orientation orientation);
    void unsplit();
    bool isSplit() const { return m_secondaryTabManager != nullptr; }

    bool saveCurrentFile();
    bool saveCurrentFileAs();
    bool saveAllFiles();
    int autoSaveModified();
    void checkExternalChanges();
    void refreshEditorSettings();
    bool closeActiveTab();
    bool closeAllFiles();

    QStringList openFilePaths() const;
    int countAllTabs() const;
    void applyTheme(const core::ThemeColors& theme);
    void updateFontSettings(const QString& family, int size);

signals:
    void activeEditorChanged(CodeEditor* editor);
    void fileOpened(const QString& filePath);
    void fileSaved(const QString& filePath);
    void documentEdited(CodeEditor* editor);
    void allFilesClosed();

private:
    void setupPaneConnections(TabManager* tm);

    QSplitter* m_splitter{nullptr};
    TabManager* m_primaryTabManager{nullptr};
    TabManager* m_secondaryTabManager{nullptr};
    TabManager* m_activeTabManager{nullptr};
};

} // namespace liteforge::editor
