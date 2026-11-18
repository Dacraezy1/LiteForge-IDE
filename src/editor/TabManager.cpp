#include "liteforge/editor/TabManager.hpp"
#include "liteforge/core/FileUtils.hpp"
#include <QFileInfo>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QTabBar>

namespace liteforge::editor {

TabManager::TabManager(QWidget* parent)
    : QTabWidget(parent) {
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QTabWidget::tabCloseRequested, this, &TabManager::onTabCloseRequested);
    connect(this, &QTabWidget::currentChanged, this, &TabManager::onCurrentChanged);
    connect(this, &QWidget::customContextMenuRequested, this, &TabManager::showContextMenu);
}

CodeEditor* TabManager::openFile(const QString& filePath) {
    int existingIdx = findTabIndexByPath(filePath);
    if (existingIdx != -1) {
        if (existingIdx == m_previewTabIndex) {
            promotePreviewTab(existingIdx);
        }
        setCurrentIndex(existingIdx);
        return editorAt(existingIdx);
    }

    bool ok = false;
    QString content = core::FileUtils::readFileUtf8(filePath, &ok);
    if (!ok) {
        return nullptr;
    }

    auto doc = std::make_shared<core::Document>(filePath);
    auto* editor = new CodeEditor(this);
    editor->setPlainText(content);
    editor->setDocumentModel(doc);
    doc->setModified(false);

    int idx = addTab(editor, doc->fileName());
    setCurrentIndex(idx);

    connect(editor->document(), &QTextDocument::contentsChanged, this, [this, doc, editor]() {
        doc->setModified(true);
        int i = indexOf(editor);
        if (i != -1) {
            if (i == m_previewTabIndex) {
                promotePreviewTab(i);
            }
            updateTabTitle(i);
        }
    });

    emit fileOpened(filePath);
    return editor;
}

CodeEditor* TabManager::openFilePreview(const QString& filePath) {
    int existingIdx = findTabIndexByPath(filePath);
    if (existingIdx != -1) {
        setCurrentIndex(existingIdx);
        return editorAt(existingIdx);
    }

    // Reuse existing unmodified preview tab if present
    if (m_previewTabIndex >= 0 && m_previewTabIndex < count()) {
        CodeEditor* existing = editorAt(m_previewTabIndex);
        if (existing && existing->documentModel() && !existing->documentModel()->isModified()) {
            bool ok = false;
            QString content = core::FileUtils::readFileUtf8(filePath, &ok);
            if (ok) {
                auto doc = std::make_shared<core::Document>(filePath);
                existing->setPlainText(content);
                existing->setDocumentModel(doc);
                doc->setModified(false);
                updateTabTitle(m_previewTabIndex);
                setCurrentIndex(m_previewTabIndex);
                emit fileOpened(filePath);
                return existing;
            }
        }
    }

    CodeEditor* ed = openFile(filePath);
    if (ed) {
        m_previewTabIndex = indexOf(ed);
        updateTabTitle(m_previewTabIndex);
    }
    return ed;
}

void TabManager::promotePreviewTab(int index) {
    if (index == m_previewTabIndex) {
        m_previewTabIndex = -1;
        updateTabTitle(index);
    }
}

CodeEditor* TabManager::newUntitledDocument() {
    static int s_untitledCount = 1;
    auto doc = std::make_shared<core::Document>();
    auto* editor = new CodeEditor(this);
    editor->setDocumentModel(doc);

    QString title = QString("Untitled-%1").arg(s_untitledCount++);
    int idx = addTab(editor, title);
    setCurrentIndex(idx);

    connect(editor->document(), &QTextDocument::contentsChanged, this, [this, doc, editor]() {
        doc->setModified(true);
        int i = indexOf(editor);
        if (i != -1) updateTabTitle(i);
    });

    return editor;
}

CodeEditor* TabManager::currentEditor() const {
    return dynamic_cast<CodeEditor*>(currentWidget());
}

CodeEditor* TabManager::editorAt(int index) const {
    return dynamic_cast<CodeEditor*>(widget(index));
}

bool TabManager::closeFile(int index) {
    if (isTabPinned(index)) return false; // Pinned tabs cannot be closed without unpinning

    CodeEditor* editor = editorAt(index);
    if (!editor) return false;

    auto doc = editor->documentModel();
    if (doc && doc->isModified()) {
        QMessageBox::StandardButton res = QMessageBox::warning(
            this, "Unsaved Changes",
            QString("Save changes to \"%1\" before closing?").arg(doc->fileName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save
        );

        if (res == QMessageBox::Save) {
            if (!saveFileAt(index)) return false;
        } else if (res == QMessageBox::Cancel) {
            return false;
        }
    }

    if (index == m_previewTabIndex) {
        m_previewTabIndex = -1;
    }
    m_pinnedTabs.erase(index);

    removeTab(index);
    editor->deleteLater();

    if (count() == 0) {
        emit allFilesClosed();
    }

    return true;
}

bool TabManager::closeAllFiles() {
    for (int i = count() - 1; i >= 0; --i) {
        if (!isTabPinned(i)) {
            if (!closeFile(i)) return false;
        }
    }
    return true;
}

bool TabManager::closeOtherFiles(int index) {
    for (int i = count() - 1; i >= 0; --i) {
        if (i != index && !isTabPinned(i)) {
            if (!closeFile(i)) return false;
        }
    }
    return true;
}

bool TabManager::saveCurrentFile() {
    return saveFileAt(currentIndex());
}

bool TabManager::saveFileAt(int index) {
    CodeEditor* editor = editorAt(index);
    if (!editor) return false;

    auto doc = editor->documentModel();
    if (!doc) return false;

    if (doc->isUntitled()) {
        return false;
    }

    bool ok = core::FileUtils::writeFileUtf8(doc->filePath(), editor->toPlainText());
    if (ok) {
        doc->setModified(false);
        updateTabTitle(index);
        return true;
    }
    return false;
}

bool TabManager::saveAllFiles() {
    bool allSaved = true;
    for (int i = 0; i < count(); ++i) {
        if (!saveFileAt(i)) {
            allSaved = false;
        }
    }
    return allSaved;
}

void TabManager::togglePinTab(int index) {
    if (m_pinnedTabs.count(index)) {
        m_pinnedTabs.erase(index);
    } else {
        m_pinnedTabs.insert(index);
        if (index == m_previewTabIndex) {
            m_previewTabIndex = -1;
        }
    }
    updateTabTitle(index);
}

bool TabManager::isTabPinned(int index) const {
    return m_pinnedTabs.count(index) > 0;
}

void TabManager::applyTheme(const core::ThemeColors& theme) {
    for (int i = 0; i < count(); ++i) {
        CodeEditor* ed = editorAt(i);
        if (ed) ed->applyTheme(theme);
    }
}

void TabManager::updateFontSettings(const QString& family, int size) {
    for (int i = 0; i < count(); ++i) {
        CodeEditor* ed = editorAt(i);
        if (ed) ed->updateFontSettings(family, size);
    }
}

QStringList TabManager::openFilePaths() const {
    QStringList paths;
    for (int i = 0; i < count(); ++i) {
        CodeEditor* ed = editorAt(i);
        if (ed && ed->documentModel() && !ed->documentModel()->isUntitled()) {
            paths.append(ed->documentModel()->filePath());
        }
    }
    return paths;
}

void TabManager::closeActiveTab() {
    closeFile(currentIndex());
}

void TabManager::onTabCloseRequested(int index) {
    closeFile(index);
}

void TabManager::onCurrentChanged(int index) {
    emit activeEditorChanged(editorAt(index));
}

void TabManager::onDocumentModified(int index) {
    updateTabTitle(index);
}

void TabManager::updateTabTitle(int index) {
    if (index < 0 || index >= count()) return;

    CodeEditor* ed = editorAt(index);
    if (!ed || !ed->documentModel()) return;

    QString name = ed->documentModel()->fileName();
    QString prefix;

    if (isTabPinned(index)) {
        prefix += "📌 ";
    }
    if (ed->documentModel()->isModified()) {
        prefix += "● ";
    }
    if (index == m_previewTabIndex) {
        name = "<i>" + name + "</i>";
    }

    setTabText(index, prefix + name);
    setTabToolTip(index, ed->documentModel()->filePath());
}

int TabManager::findTabIndexByPath(const QString& filePath) const {
    for (int i = 0; i < count(); ++i) {
        CodeEditor* ed = editorAt(i);
        if (ed && ed->documentModel() && ed->documentModel()->filePath() == filePath) {
            return i;
        }
    }
    return -1;
}

void TabManager::showContextMenu(const QPoint& pos) {
    int tabIndex = tabBar()->tabAt(pos);
    if (tabIndex == -1) return;

    QMenu menu(this);
    bool pinned = isTabPinned(tabIndex);

    menu.addAction(pinned ? "Unpin Tab" : "Pin Tab", [this, tabIndex]() {
        togglePinTab(tabIndex);
    });

    menu.addSeparator();

    menu.addAction("Split Right (Vertical)", [this]() {
        emit splitRequested(Qt::Horizontal);
    });

    menu.addAction("Split Down (Horizontal)", [this]() {
        emit splitRequested(Qt::Vertical);
    });

    menu.addSeparator();

    menu.addAction("Close", [this, tabIndex]() {
        closeFile(tabIndex);
    });

    menu.addAction("Close Others", [this, tabIndex]() {
        closeOtherFiles(tabIndex);
    });

    menu.addAction("Copy Full Path", [this, tabIndex]() {
        CodeEditor* ed = editorAt(tabIndex);
        if (ed && ed->documentModel()) {
            QApplication::clipboard()->setText(ed->documentModel()->filePath());
        }
    });

    menu.exec(mapToGlobal(pos));
}

} // namespace liteforge::editor
