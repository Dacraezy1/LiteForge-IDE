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

    connect(editor->document(), &QTextDocument::contentsChanged, this, [this, doc, idx]() {
        doc->setModified(true);
        updateTabTitle(indexOf(dynamic_cast<QWidget*>(sender()->parent())));
    });

    emit fileOpened(filePath);
    return editor;
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

    removeTab(index);
    delete editor;

    if (count() == 0) {
        emit allFilesClosed();
    }
    return true;
}

void TabManager::closeActiveTab() {
    if (currentIndex() != -1) {
        closeFile(currentIndex());
    }
}

bool TabManager::closeAllFiles() {
    while (count() > 0) {
        if (!closeFile(0)) return false;
    }
    return true;
}

bool TabManager::closeOtherFiles(int index) {
    for (int i = count() - 1; i >= 0; --i) {
        if (i != index) {
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

    bool success = core::FileUtils::writeFileUtf8(doc->filePath(), editor->toPlainText());
    if (success) {
        doc->setModified(false);
        updateTabTitle(index);
    }
    return success;
}

bool TabManager::saveAllFiles() {
    bool allSuccess = true;
    for (int i = 0; i < count(); ++i) {
        if (!saveFileAt(i)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

void TabManager::updateTabTitle(int index) {
    CodeEditor* editor = editorAt(index);
    if (!editor) return;

    auto doc = editor->documentModel();
    if (!doc) return;

    QString title = doc->fileName();
    if (doc->isModified()) {
        title += " •";
    }
    setTabText(index, title);
    setTabToolTip(index, doc->filePath());
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

void TabManager::applyTheme(const core::ThemeColors& theme) {
    for (int i = 0; i < count(); ++i) {
        CodeEditor* ed = editorAt(i);
        if (ed) {
            ed->applyTheme(theme);
        }
    }
}

void TabManager::updateFontSettings(const QString& family, int size) {
    for (int i = 0; i < count(); ++i) {
        CodeEditor* ed = editorAt(i);
        if (ed) {
            ed->updateFontSettings(family, size);
        }
    }
}

void TabManager::onTabCloseRequested(int index) {
    closeFile(index);
}

void TabManager::onCurrentChanged(int index) {
    emit activeEditorChanged(editorAt(index));
}

void TabManager::showContextMenu(const QPoint& pos) {
    int tabIndex = tabBar()->tabAt(pos);
    if (tabIndex == -1) return;

    QMenu menu(this);
    QAction* closeAct = menu.addAction("Close");
    QAction* closeOthersAct = menu.addAction("Close Others");
    QAction* closeAllAct = menu.addAction("Close All");
    menu.addSeparator();
    QAction* copyPathAct = menu.addAction("Copy File Path");

    QAction* selected = menu.exec(mapToGlobal(pos));
    if (selected == closeAct) {
        closeFile(tabIndex);
    } else if (selected == closeOthersAct) {
        closeOtherFiles(tabIndex);
    } else if (selected == closeAllAct) {
        closeAllFiles();
    } else if (selected == copyPathAct) {
        CodeEditor* ed = editorAt(tabIndex);
        if (ed && ed->documentModel()) {
            QApplication::clipboard()->setText(ed->documentModel()->filePath());
        }
    }
}

} // namespace liteforge::editor
