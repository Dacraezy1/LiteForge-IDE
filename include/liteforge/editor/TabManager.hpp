#pragma once

#include "liteforge/editor/CodeEditor.hpp"
#include <QTabWidget>
#include <QPointer>

namespace liteforge::editor {

class TabManager : public QTabWidget {
    Q_OBJECT

public:
    explicit TabManager(QWidget* parent = nullptr);
    ~TabManager() override = default;

    CodeEditor* openFile(const QString& filePath);
    CodeEditor* openFilePreview(const QString& filePath);
    CodeEditor* newUntitledDocument();

    CodeEditor* currentEditor() const;
    CodeEditor* editorAt(int index) const;
    CodeEditor* editorForPath(const QString& path) const;

    bool closeFile(int index);
    bool closeAllFiles();
    bool closeOtherFiles(int index);
    bool saveCurrentFile();
    bool saveFileAt(int index);          // untitled documents prompt for a path
    bool saveCurrentFileAs();
    bool saveFileAsAt(int index);
    bool saveAllFiles();

    // Saves every modified document that already has a path. Never prompts.
    int autoSaveModified();
    // Reloads clean documents changed on disk; asks before touching dirty ones.
    void checkExternalChanges();

    void togglePinTab(int index);
    bool isTabPinned(int index) const;
    void promotePreviewTab(int index);

    void applyTheme(const core::ThemeColors& theme);
    void updateFontSettings(const QString& family, int size);
    void refreshEditorSettings();

    QStringList openFilePaths() const;
    void closeActiveTab();

signals:
    void activeEditorChanged(CodeEditor* editor);
    void fileOpened(const QString& filePath);
    void fileSaved(const QString& filePath);
    void documentEdited(CodeEditor* editor);
    void allFilesClosed();
    void splitRequested(Qt::Orientation orientation);

private slots:
    void onTabCloseRequested(int index);
    void onCurrentChanged(int index);
    void showContextMenu(const QPoint& pos);

private:
    void updateTabTitle(int index);
    int findTabIndexByPath(const QString& filePath) const;
    int previewIndex() const;
    void wireEditor(CodeEditor* editor);
    bool writeEditor(CodeEditor* editor, int index, bool normalize = true);
    void reloadFromDisk(CodeEditor* editor);
    bool loadInto(CodeEditor* editor, const QString& path);

    QPointer<CodeEditor> m_previewEditor;
    bool m_checkingExternal{false};
    bool m_loading{false};   // programmatic (re)load in progress: not a user edit
};

} // namespace liteforge::editor
