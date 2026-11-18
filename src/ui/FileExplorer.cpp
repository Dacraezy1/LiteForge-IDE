#include "liteforge/ui/FileExplorer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QHeaderView>
#include <QDir>
#include <QFile>

namespace liteforge::ui {

FileExplorer::FileExplorer(QWidget* parent)
    : QWidget(parent) {
    setupUi();
}

void FileExplorer::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_searchFilter = new QLineEdit(this);
    m_searchFilter->setPlaceholderText("Filter files...");
    m_searchFilter->setClearButtonEnabled(true);
    layout->addWidget(m_searchFilter);

    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_fileModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setRecursiveFilteringEnabled(true);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_proxyModel);
    m_treeView->setHeaderHidden(true);

    // Hide size, type, date columns - only show name column
    for (int col = 1; col < m_fileModel->columnCount(); ++col) {
        m_treeView->hideColumn(col);
    }

    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_treeView);

    connect(m_searchFilter, &QLineEdit::textChanged, this, &FileExplorer::onFilterChanged);
    connect(m_treeView, &QTreeView::doubleClicked, this, &FileExplorer::onItemDoubleClicked);
    connect(m_treeView, &QWidget::customContextMenuRequested, this, &FileExplorer::showContextMenu);
}

void FileExplorer::setRootPath(const QString& path) {
    m_rootPath = path;
    QModelIndex rootIndex = m_fileModel->setRootPath(path);
    QModelIndex proxyRoot = m_proxyModel->mapFromSource(rootIndex);
    m_treeView->setRootIndex(proxyRoot);
}

void FileExplorer::onItemDoubleClicked(const QModelIndex& index) {
    QModelIndex srcIndex = m_proxyModel->mapToSource(index);
    if (!m_fileModel->isDir(srcIndex)) {
        QString filePath = m_fileModel->filePath(srcIndex);
        emit fileSelected(filePath);
    }
}

void FileExplorer::onFilterChanged(const QString& filterText) {
    m_proxyModel->setFilterFixedString(filterText);
}

QString FileExplorer::selectedFilePath() const {
    QModelIndex cur = m_treeView->currentIndex();
    if (!cur.isValid()) return m_rootPath;
    QModelIndex srcIndex = m_proxyModel->mapToSource(cur);
    return m_fileModel->filePath(srcIndex);
}

void FileExplorer::showContextMenu(const QPoint& pos) {
    QMenu menu(this);
    QAction* newFileAct = menu.addAction("New File...");
    QAction* newFolderAct = menu.addAction("New Folder...");
    menu.addSeparator();
    QAction* renameAct = menu.addAction("Rename...");
    QAction* deleteAct = menu.addAction("Delete");
    menu.addSeparator();
    QAction* copyPathAct = menu.addAction("Copy Path");
    QAction* revealAct = menu.addAction("Reveal in File Manager");

    QAction* selected = menu.exec(m_treeView->mapToGlobal(pos));
    if (selected == newFileAct) {
        createNewFile();
    } else if (selected == newFolderAct) {
        createNewFolder();
    } else if (selected == renameAct) {
        renameSelected();
    } else if (selected == deleteAct) {
        deleteSelected();
    } else if (selected == copyPathAct) {
        copyPathSelected();
    } else if (selected == revealAct) {
        revealInFileManager();
    }
}

void FileExplorer::createNewFile() {
    QString targetDir = selectedFilePath();
    QFileInfo fi(targetDir);
    if (!fi.isDir()) {
        targetDir = fi.absolutePath();
    }

    bool ok = false;
    QString fileName = QInputDialog::getText(this, "New File", "File name:", QLineEdit::Normal, "", &ok);
    if (ok && !fileName.isEmpty()) {
        QString fullPath = targetDir + "/" + fileName;
        QFile file(fullPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            emit fileSelected(fullPath);
        } else {
            QMessageBox::warning(this, "Error", "Could not create file: " + file.errorString());
        }
    }
}

void FileExplorer::createNewFolder() {
    QString targetDir = selectedFilePath();
    QFileInfo fi(targetDir);
    if (!fi.isDir()) {
        targetDir = fi.absolutePath();
    }

    bool ok = false;
    QString folderName = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, "", &ok);
    if (ok && !folderName.isEmpty()) {
        QDir dir(targetDir);
        if (!dir.mkdir(folderName)) {
            QMessageBox::warning(this, "Error", "Could not create folder.");
        }
    }
}

void FileExplorer::renameSelected() {
    QString path = selectedFilePath();
    if (path.isEmpty() || path == m_rootPath) return;

    QFileInfo fi(path);
    bool ok = false;
    QString newName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal, fi.fileName(), &ok);
    if (ok && !newName.isEmpty() && newName != fi.fileName()) {
        QString newPath = fi.absolutePath() + "/" + newName;
        if (!QFile::rename(path, newPath)) {
            QMessageBox::warning(this, "Error", "Failed to rename file.");
        }
    }
}

void FileExplorer::deleteSelected() {
    QString path = selectedFilePath();
    if (path.isEmpty() || path == m_rootPath) return;

    QFileInfo fi(path);
    QMessageBox::StandardButton res = QMessageBox::question(
        this, "Delete",
        QString("Are you sure you want to permanently delete \"%1\"?").arg(fi.fileName()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (res == QMessageBox::Yes) {
        if (fi.isDir()) {
            QDir dir(path);
            dir.removeRecursively();
        } else {
            QFile::remove(path);
        }
    }
}

void FileExplorer::copyPathSelected() {
    QString path = selectedFilePath();
    if (!path.isEmpty()) {
        QApplication::clipboard()->setText(path);
    }
}

void FileExplorer::revealInFileManager() {
    QString path = selectedFilePath();
    if (path.isEmpty()) path = m_rootPath;
    QFileInfo fi(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.isDir() ? path : fi.absolutePath()));
}

} // namespace liteforge::ui
