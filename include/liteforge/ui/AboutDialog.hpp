#pragma once

#include <QDialog>

namespace liteforge::ui {

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog() override = default;

private:
    void setupUi();
};

} // namespace liteforge::ui
