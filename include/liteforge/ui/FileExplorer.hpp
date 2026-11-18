#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QLineEdit>
#include <QSortFilterProxyModel>

namespace liteforge::ui {

class FileExplorer : public QWidget {
    Q_OBJECT

public:
    explicit FileExplorer(QWidget* parent = nullptr);
    ~FileExplorer() override = default;

    void setRootPath(const QString& path);
    QString rootPath() const { return m_rootPath; }

signals:
    void fileSelected(const QString& filePath);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void showContextMenu(const QPoint& pos);
    void onFilterChanged(const QString& filterText);

    void createNewFile();
    void createNewFolder();
    void renameSelected();
    void deleteSelected();
    void copyPathSelected();
    void revealInFileManager();

private:
    void setupUi();
    QString selectedFilePath() const;

    QString m_rootPath;
    QLineEdit* m_searchFilter;
    QTreeView* m_treeView;
    QFileSystemModel* m_fileModel;
    QSortFilterProxyModel* m_proxyModel;
};

} // namespace liteforge::ui
