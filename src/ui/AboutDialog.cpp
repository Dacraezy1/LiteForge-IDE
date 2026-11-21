#include "liteforge/ui/AboutDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace liteforge::ui {

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("About LiteForge IDE");
    setFixedSize(480, 400);
    setupUi();
}

void AboutDialog::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(14);
    layout->setContentsMargins(20, 20, 20, 20);

    auto* titleLabel = new QLabel(QStringLiteral("LiteForge IDE  ") + QStringLiteral(LITEFORGE_VERSION), this);
    titleLabel->setStyleSheet("font-size: 22px; font-weight: bold;");
    layout->addWidget(titleLabel);

    auto* subTitleLabel = new QLabel("The Featherweight, Zero-Bloat Native C++ IDE", this);
    subTitleLabel->setStyleSheet("font-size: 13px; font-style: italic;");
    layout->addWidget(subTitleLabel);

    auto* descLabel = new QLabel(
        "A small, fast code editor and IDE written in C++20 and Qt 6.\n\n"
        "- Every panel (terminal, git, debugger, dialogs) is built on first use\n"
        "- Themes, keybindings and settings are plain JSON files\n"
        "- Integrated terminal, build runner, diagnostics and LSP client\n"
        "- Fuzzy file finder and command palette\n"
        "- Native Wayland and X11\n\n"
        "Open source under the GNU General Public License v3 (GPLv3).", this
    );
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("font-size: 12px;");
    layout->addWidget(descLabel);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* closeBtn = new QPushButton("Close", this);
    closeBtn->setDefault(true);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

} // namespace liteforge::ui
