#include "liteforge/editor/TabManager.hpp"
#include "liteforge/core/FileUtils.hpp"
#include "liteforge/core/Config.hpp"
#include <QFileInfo>
#include <QFileDialog>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QTabBar>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>

namespace liteforge::editor {

namespace {
constexpr qint64 kHardFileLimit = 256ll * 1024 * 1024; // refuse to load > 256 MiB into an editor
constexpr const char* kPinned = "lf_pinned";
constexpr const char* kUntitledTitle = "lf_untitled_title";

// Strip trailing blanks / ensure final newline as ONE undo step, without
// touching lines that don't need it.
void normalizeForSave(CodeEditor* ed, bool trim, bool finalNewline) {
    QTextDocument* doc = ed->document();
    QTextCursor edit(doc);
    edit.beginEditBlock();
    if (trim) {
        for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
            const QString t = b.text();
            int keep = t.size();
            while (keep > 0 && (t[keep - 1] == QLatin1Char(' ') || t[keep - 1] == QLatin1Char('\t'))) --keep;
            if (keep != t.size()) {
                QTextCursor c(b);
                c.setPosition(b.position() + keep);
                c.setPosition(b.position() + t.size(), QTextCursor::KeepAnchor);
                c.removeSelectedText();
            }
        }
    }
    if (finalNewline && doc->characterCount() > 1) {
        const QTextBlock last = doc->lastBlock();
        if (!last.text().isEmpty()) {
            QTextCursor c(doc);
            c.movePosition(QTextCursor::End);
            c.insertText(QStringLiteral("\n"));
        }
    }
    edit.endEditBlock();
}
} // namespace

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

int TabManager::previewIndex() const {
    return m_previewEditor ? indexOf(m_previewEditor) : -1;
}

bool TabManager::loadInto(CodeEditor* editor, const QString& path) {
    const QFileInfo fi(path);
    if (fi.size() > kHardFileLimit) {
        QMessageBox::warning(this, "File Too Large",
            QString("\"%1\" is %2, which is above the 256 MiB editor limit.")
                .arg(fi.fileName(), core::FileUtils::formatFileSize(fi.size())));
        return false;
    }
    bool ok = false, crlf = false, bom = false;
    const QString content = core::FileUtils::readFileText(path, &ok, &crlf, &bom);
    if (!ok) return false;

    auto doc = std::make_shared<core::Document>(path);
    doc->setLineEnding(crlf ? core::LineEnding::CRLF : core::LineEnding::LF);
    doc->setHasBom(bom);

    m_loading = true;
    editor->setPlainText(content);
    editor->setDocumentModel(doc);
    doc->setModified(false);
    m_loading = false;
    return true;
}

// One handler per editor. It always asks the editor for its CURRENT document
// model, so replacing the model (preview-tab reuse, reload) can't leave a stale
// document marked dirty while the live one silently loses edits.
void TabManager::wireEditor(CodeEditor* editor) {
    connect(editor->document(), &QTextDocument::contentsChanged, this, [this, editor]() {
        if (m_loading) return;
        auto doc = editor->documentModel();
        if (!doc) return;
        doc->setModified(true);
        const int i = indexOf(editor);
        if (i != -1) {
            if (editor == m_previewEditor) promotePreviewTab(i);
            updateTabTitle(i);
        }
        emit documentEdited(editor);
    });
}

CodeEditor* TabManager::openFile(const QString& filePath) {
    const int existingIdx = findTabIndexByPath(filePath);
    if (existingIdx != -1) {
        if (existingIdx == previewIndex()) promotePreviewTab(existingIdx);
        setCurrentIndex(existingIdx);
        return editorAt(existingIdx);
    }

    auto* editor = new CodeEditor(this);
    if (!loadInto(editor, filePath)) {
        delete editor;
        return nullptr;
    }

    const int idx = addTab(editor, editor->documentModel()->fileName());
    setCurrentIndex(idx);
    wireEditor(editor);
    updateTabTitle(idx);

    emit fileOpened(filePath);
    return editor;
}

CodeEditor* TabManager::openFilePreview(const QString& filePath) {
    const int existingIdx = findTabIndexByPath(filePath);
    if (existingIdx != -1) {
        setCurrentIndex(existingIdx);
        return editorAt(existingIdx);
    }

    // Reuse the existing clean preview tab (keeps RAM flat while browsing files).
    CodeEditor* reuse = m_previewEditor;
    if (reuse && reuse->documentModel() && !reuse->documentModel()->isModified()) {
        if (loadInto(reuse, filePath)) {
            const int i = indexOf(reuse);
            updateTabTitle(i);
            setCurrentIndex(i);
            emit fileOpened(filePath);
            return reuse;
        }
    }

    CodeEditor* ed = openFile(filePath);
    if (ed) {
        m_previewEditor = ed;
        updateTabTitle(indexOf(ed));
    }
    return ed;
}

void TabManager::promotePreviewTab(int index) {
    if (index >= 0 && editorAt(index) == m_previewEditor) {
        m_previewEditor = nullptr;
        updateTabTitle(index);
    }
}

CodeEditor* TabManager::newUntitledDocument() {
    static int s_untitledCount = 1;
    auto doc = std::make_shared<core::Document>();
    auto* editor = new CodeEditor(this);
    editor->setDocumentModel(doc);

    const QString title = QString("Untitled-%1").arg(s_untitledCount++);
    editor->setProperty(kUntitledTitle, title);
    const int idx = addTab(editor, title);
    setCurrentIndex(idx);
    wireEditor(editor);
    return editor;
}

CodeEditor* TabManager::currentEditor() const {
    return qobject_cast<CodeEditor*>(currentWidget());
}

CodeEditor* TabManager::editorForPath(const QString& path) const {
    const int i = findTabIndexByPath(path);
    return i < 0 ? nullptr : editorAt(i);
}

CodeEditor* TabManager::editorAt(int index) const {
    return qobject_cast<CodeEditor*>(widget(index));
}

bool TabManager::closeFile(int index) {
    if (isTabPinned(index)) return false; // unpin first

    CodeEditor* editor = editorAt(index);
    if (!editor) return false;

    auto doc = editor->documentModel();
    if (doc && doc->isModified()) {
        const auto res = QMessageBox::warning(
            this, "Unsaved Changes",
            QString("Save changes to \"%1\" before closing?")
                .arg(doc->isUntitled() ? tabText(index).remove('&') : doc->fileName()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);

        if (res == QMessageBox::Save) {
            if (!saveFileAt(index)) return false;
        } else if (res == QMessageBox::Cancel) {
            return false;
        }
    }

    if (editor == m_previewEditor) m_previewEditor = nullptr;

    removeTab(index);
    editor->deleteLater();

    if (count() == 0) emit allFilesClosed();
    return true;
}

bool TabManager::closeAllFiles() {
    for (int i = count() - 1; i >= 0; --i) {
        if (!isTabPinned(i) && !closeFile(i)) return false;
    }
    return true;
}

bool TabManager::closeOtherFiles(int index) {
    CodeEditor* keep = editorAt(index);
    for (int i = count() - 1; i >= 0; --i) {
        if (editorAt(i) != keep && !isTabPinned(i) && !closeFile(i)) return false;
    }
    return true;
}

bool TabManager::saveCurrentFile() { return saveFileAt(currentIndex()); }
bool TabManager::saveCurrentFileAs() { return saveFileAsAt(currentIndex()); }

bool TabManager::writeEditor(CodeEditor* editor, int index, bool normalize) {
    auto doc = editor->documentModel();
    if (!doc || doc->isUntitled()) return false;

    const auto& cfg = core::Config::instance().editor();
    const bool markdown = doc->language() == core::Language::Markdown; // trailing double-space is a line break there
    const bool trim = cfg.trimTrailingWhitespaceOnSave && !markdown;
    if (normalize && (trim || cfg.insertFinalNewline)) normalizeForSave(editor, trim, cfg.insertFinalNewline);

    const bool ok = core::FileUtils::writeFileText(doc->filePath(), editor->toPlainText(),
                                                   doc->lineEnding() == core::LineEnding::CRLF, doc->hasBom());
    if (!ok) {
        QMessageBox::warning(this, "Save Failed", QString("Could not write \"%1\".").arg(doc->filePath()));
        return false;
    }
    doc->setModified(false);
    doc->refreshDiskTimestamp();
    updateTabTitle(index);
    emit fileSaved(doc->filePath());
    return true;
}

bool TabManager::saveFileAt(int index) {
    CodeEditor* editor = editorAt(index);
    if (!editor || !editor->documentModel()) return false;
    if (editor->documentModel()->isUntitled()) return saveFileAsAt(index);
    return writeEditor(editor, index);
}

bool TabManager::saveFileAsAt(int index) {
    CodeEditor* editor = editorAt(index);
    if (!editor || !editor->documentModel()) return false;
    auto doc = editor->documentModel();

    const QString start = doc->isUntitled() ? QString() : doc->filePath();
    const QString path = QFileDialog::getSaveFileName(this, "Save File As", start);
    if (path.isEmpty()) return false;

    doc->setFilePath(path); // updates language + highlighter
    return writeEditor(editor, index);
}

bool TabManager::saveAllFiles() {
    bool allSaved = true;
    for (int i = 0; i < count(); ++i) {
        CodeEditor* ed = editorAt(i);
        if (ed && ed->documentModel() && ed->documentModel()->isModified() && !saveFileAt(i)) allSaved = false;
    }
    return allSaved;
}

int TabManager::autoSaveModified() {
    int saved = 0;
    for (int i = 0; i < count(); ++i) {
        CodeEditor* ed = editorAt(i);
        if (!ed || !ed->documentModel()) continue;
        const auto doc = ed->documentModel();
        if (doc->isModified() && !doc->isUntitled() && writeEditor(ed, i, false)) ++saved;
    }
    return saved;
}

void TabManager::reloadFromDisk(CodeEditor* editor) {
    const auto doc = editor->documentModel();
    if (!doc) return;
    const int oldPos = editor->textCursor().position();
    const int vScroll = editor->verticalScrollBar()->value();

    bool ok = false, crlf = false, bom = false;
    const QString content = core::FileUtils::readFileText(doc->filePath(), &ok, &crlf, &bom);
    if (!ok) return;

    m_loading = true;
    editor->setPlainText(content);
    m_loading = false;
    doc->setLineEnding(crlf ? core::LineEnding::CRLF : core::LineEnding::LF);
    doc->setHasBom(bom);
    doc->setModified(false);
    doc->refreshDiskTimestamp();

    QTextCursor c(editor->document());
    c.setPosition(qMin(oldPos, qMax(0, editor->document()->characterCount() - 1)));
    editor->setTextCursor(c);
    editor->verticalScrollBar()->setValue(vScroll);

    const int i = indexOf(editor);
    if (i != -1) updateTabTitle(i);
}

void TabManager::checkExternalChanges() {
    if (m_checkingExternal) return;
    m_checkingExternal = true;
    for (int i = 0; i < count(); ++i) {
        CodeEditor* ed = editorAt(i);
        if (!ed || !ed->documentModel()) continue;
        const auto doc = ed->documentModel();
        if (doc->isUntitled() || !doc->checkDiskModified()) continue;

        if (!doc->isModified()) {
            reloadFromDisk(ed);
        } else {
            const auto res = QMessageBox::question(
                this, "File Changed on Disk",
                QString("\"%1\" was changed outside LiteForge and you have unsaved edits.\n\n"
                        "Reload from disk and discard your edits?").arg(doc->fileName()),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (res == QMessageBox::Yes) reloadFromDisk(ed);
            else doc->refreshDiskTimestamp(); // keep my version; don't nag again until it changes again
        }
    }
    m_checkingExternal = false;
}

void TabManager::togglePinTab(int index) {
    CodeEditor* ed = editorAt(index);
    if (!ed) return;
    const bool pin = !ed->property(kPinned).toBool();
    ed->setProperty(kPinned, pin);
    if (pin && ed == m_previewEditor) m_previewEditor = nullptr;
    updateTabTitle(index);
}

bool TabManager::isTabPinned(int index) const {
    const CodeEditor* ed = editorAt(index);
    return ed && ed->property(kPinned).toBool();
}

void TabManager::applyTheme(const core::ThemeColors& theme) {
    for (int i = 0; i < count(); ++i)
        if (CodeEditor* ed = editorAt(i)) ed->applyTheme(theme);
}

void TabManager::updateFontSettings(const QString& family, int size) {
    for (int i = 0; i < count(); ++i)
        if (CodeEditor* ed = editorAt(i)) ed->updateFontSettings(family, size);
}

void TabManager::refreshEditorSettings() {
    const auto& cfg = core::Config::instance().editor();
    for (int i = 0; i < count(); ++i) {
        if (CodeEditor* ed = editorAt(i)) {
            ed->updateFontSettings(cfg.fontFamily, cfg.fontSize);
            ed->setStickyScrollEnabled(cfg.stickyScroll);
            if (!ed->isLargeFile()) ed->setMinimapEnabled(cfg.minimap);
            ed->setWordWrap(cfg.wordWrap);
        }
    }
}

QStringList TabManager::openFilePaths() const {
    QStringList paths;
    for (int i = 0; i < count(); ++i) {
        CodeEditor* ed = editorAt(i);
        if (ed && ed->documentModel() && !ed->documentModel()->isUntitled())
            paths.append(ed->documentModel()->filePath());
    }
    return paths;
}

void TabManager::closeActiveTab() { closeFile(currentIndex()); }
void TabManager::onTabCloseRequested(int index) { closeFile(index); }
void TabManager::onCurrentChanged(int index) { emit activeEditorChanged(editorAt(index)); }

void TabManager::updateTabTitle(int index) {
    if (index < 0 || index >= count()) return;
    CodeEditor* ed = editorAt(index);
    if (!ed || !ed->documentModel()) return;
    const auto doc = ed->documentModel();

    QString name = doc->isUntitled() ? ed->property(kUntitledTitle).toString() : doc->fileName();
    if (name.isEmpty()) name = doc->fileName();
    name.replace('&', QStringLiteral("&&")); // '&' would otherwise become a mnemonic

    QString prefix;
    if (isTabPinned(index)) prefix += QStringLiteral("+ ");
    if (doc->isModified()) prefix += QStringLiteral("* ");
    if (ed == m_previewEditor) name = QStringLiteral("[") + name + QStringLiteral("]");

    setTabText(index, prefix + name);
    setTabToolTip(index, doc->filePath());
}

int TabManager::findTabIndexByPath(const QString& filePath) const {
    for (int i = 0; i < count(); ++i) {
        CodeEditor* ed = editorAt(i);
        if (ed && ed->documentModel() && ed->documentModel()->filePath() == filePath) return i;
    }
    return -1;
}

void TabManager::showContextMenu(const QPoint& pos) {
    const int tabIndex = tabBar()->tabAt(pos);
    if (tabIndex == -1) return;
    CodeEditor* target = editorAt(tabIndex);

    QMenu menu(this);
    const bool pinned = isTabPinned(tabIndex);

    menu.addAction(pinned ? "Unpin Tab" : "Pin Tab", [this, target]() {
        togglePinTab(indexOf(target));
    });
    menu.addSeparator();
    menu.addAction("Split Right (Vertical)", [this]() { emit splitRequested(Qt::Horizontal); });
    menu.addAction("Split Down (Horizontal)", [this]() { emit splitRequested(Qt::Vertical); });
    menu.addSeparator();
    menu.addAction("Close", [this, target]() { closeFile(indexOf(target)); });
    menu.addAction("Close Others", [this, target]() { closeOtherFiles(indexOf(target)); });
    menu.addAction("Copy Full Path", [target]() {
        if (target && target->documentModel())
            QApplication::clipboard()->setText(target->documentModel()->filePath());
    });

    menu.exec(mapToGlobal(pos));
}

} // namespace liteforge::editor
