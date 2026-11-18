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
    TabManager* activeTabManager() const;

    void split(Qt::Orientation orientation);
    void unsplit();
    bool isSplit() const { return m_secondaryTabManager != nullptr; }

    bool saveCurrentFile();
    bool saveAllFiles();
    bool closeActiveTab();
    bool closeAllFiles();

    QStringList openFilePaths() const;
    int countAllTabs() const;
    void applyTheme(const core::ThemeColors& theme);
    void updateFontSettings(const QString& family, int size);

signals:
    void activeEditorChanged(CodeEditor* editor);
    void fileOpened(const QString& filePath);
    void allFilesClosed();

private:
    void setupPaneConnections(TabManager* tm);

    QSplitter* m_splitter{nullptr};
    TabManager* m_primaryTabManager{nullptr};
    TabManager* m_secondaryTabManager{nullptr};
    TabManager* m_activeTabManager{nullptr};
};

} // namespace liteforge::editor
