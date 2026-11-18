#pragma once

#include <QWidget>
#include <QStringList>
#include <QListWidget>

namespace liteforge::ui {

class WelcomeScreen : public QWidget {
    Q_OBJECT

public:
    explicit WelcomeScreen(QWidget* parent = nullptr);
    ~WelcomeScreen() override = default;

    void updateRecentProjects(const QStringList& projects);

signals:
    void newFileRequested();
    void openFileRequested();
    void openProjectRequested();
    void projectSelected(const QString& path);

private:
    void setupUi();

    QListWidget* m_recentList{nullptr};
};

} // namespace liteforge::ui
