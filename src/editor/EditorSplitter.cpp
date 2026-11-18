#include "liteforge/editor/EditorSplitter.hpp"
#include <QVBoxLayout>

namespace liteforge::editor {

EditorSplitter::EditorSplitter(QWidget* parent)
    : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    layout->addWidget(m_splitter);

    m_primaryTabManager = new TabManager(m_splitter);
    m_splitter->addWidget(m_primaryTabManager);
    m_activeTabManager = m_primaryTabManager;

    setupPaneConnections(m_primaryTabManager);
}

void EditorSplitter::setupPaneConnections(TabManager* tm) {
    connect(tm, &TabManager::activeEditorChanged, this, [this, tm](CodeEditor* ed) {
        m_activeTabManager = tm;
        emit activeEditorChanged(ed);
    });

    connect(tm, &TabManager::fileOpened, this, &EditorSplitter::fileOpened);

    connect(tm, &TabManager::splitRequested, this, [this](Qt::Orientation ori) {
        split(ori);
    });

    connect(tm, &TabManager::allFilesClosed, this, [this, tm]() {
        if (tm == m_secondaryTabManager) {
            unsplit();
        } else if (countAllTabs() == 0) {
            emit allFilesClosed();
        }
    });
}

CodeEditor* EditorSplitter::openFile(const QString& filePath) {
    return activeTabManager()->openFile(filePath);
}

CodeEditor* EditorSplitter::openFilePreview(const QString& filePath) {
    return activeTabManager()->openFilePreview(filePath);
}

CodeEditor* EditorSplitter::newUntitledDocument() {
    return activeTabManager()->newUntitledDocument();
}

CodeEditor* EditorSplitter::currentEditor() const {
    return activeTabManager()->currentEditor();
}

TabManager* EditorSplitter::activeTabManager() const {
    return m_activeTabManager ? m_activeTabManager : m_primaryTabManager;
}

void EditorSplitter::split(Qt::Orientation orientation) {
    if (m_secondaryTabManager) {
        // Change orientation if already split
        m_splitter->setOrientation(orientation);
        return;
    }

    m_splitter->setOrientation(orientation);
    m_secondaryTabManager = new TabManager(m_splitter);
    m_splitter->addWidget(m_secondaryTabManager);
    setupPaneConnections(m_secondaryTabManager);

    // If current pane has an active editor, open same file in the second pane
    CodeEditor* cur = m_primaryTabManager->currentEditor();
    if (cur && cur->documentModel() && !cur->documentModel()->isUntitled()) {
        m_secondaryTabManager->openFile(cur->documentModel()->filePath());
    }

    m_splitter->setSizes(QList<int>() << width() / 2 << width() / 2);
}

void EditorSplitter::unsplit() {
    if (!m_secondaryTabManager) return;

    m_secondaryTabManager->closeAllFiles();
    m_secondaryTabManager->deleteLater();
    m_secondaryTabManager = nullptr;
    m_activeTabManager = m_primaryTabManager;

    emit activeEditorChanged(m_primaryTabManager->currentEditor());
}

bool EditorSplitter::saveCurrentFile() {
    return activeTabManager()->saveCurrentFile();
}

bool EditorSplitter::saveAllFiles() {
    bool ok1 = m_primaryTabManager->saveAllFiles();
    bool ok2 = m_secondaryTabManager ? m_secondaryTabManager->saveAllFiles() : true;
    return ok1 && ok2;
}

bool EditorSplitter::closeActiveTab() {
    activeTabManager()->closeActiveTab();
    return true;
}

bool EditorSplitter::closeAllFiles() {
    bool ok1 = m_primaryTabManager->closeAllFiles();
    bool ok2 = m_secondaryTabManager ? m_secondaryTabManager->closeAllFiles() : true;
    if (m_secondaryTabManager) unsplit();
    return ok1 && ok2;
}

QStringList EditorSplitter::openFilePaths() const {
    QStringList paths = m_primaryTabManager->openFilePaths();
    if (m_secondaryTabManager) {
        paths.append(m_secondaryTabManager->openFilePaths());
        paths.removeDuplicates();
    }
    return paths;
}

void EditorSplitter::applyTheme(const core::ThemeColors& theme) {
    m_primaryTabManager->applyTheme(theme);
    if (m_secondaryTabManager) {
        m_secondaryTabManager->applyTheme(theme);
    }
}

void EditorSplitter::updateFontSettings(const QString& family, int size) {
    m_primaryTabManager->updateFontSettings(family, size);
    if (m_secondaryTabManager) {
        m_secondaryTabManager->updateFontSettings(family, size);
    }
}

int EditorSplitter::countAllTabs() const {
    int total = m_primaryTabManager->count();
    if (m_secondaryTabManager) total += m_secondaryTabManager->count();
    return total;
}

} // namespace liteforge::editor
