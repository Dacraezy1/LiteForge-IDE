#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QStringList>
#include <functional>
#include <vector>

namespace liteforge::ui {

struct PaletteCommand {
    QString title;
    QString category;
    QString shortcut;
    std::function<void()> action;
};

class CommandPalette : public QDialog {
    Q_OBJECT

public:
    enum class Mode {
        Files,
        Commands
    };

    explicit CommandPalette(QWidget* parent = nullptr);
    ~CommandPalette() override = default;

    void registerCommand(const QString& category, const QString& title, const QString& shortcut, std::function<void()> action);
    void addActionItem(const QString& fullTitle, std::function<void()> action, const QString& shortcut = QString());
    void setProjectFiles(const QStringList& files);

    void showPalette(Mode mode);
    void showPalette();
    void showQuickOpen(const QStringList& files);

signals:
    void fileSelected(const QString& filePath);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onTextChanged(const QString& text);
    void onItemSelected(QListWidgetItem* item);

private:
    void updateList();
    void updateFileList(const QString& query);
    void updateCommandList(const QString& query);

    Mode m_mode{Mode::Files};
    QLineEdit* m_searchEdit;
    QListWidget* m_listWidget;

    QStringList m_projectFiles;
    std::vector<PaletteCommand> m_commands;
};

} // namespace liteforge::ui
