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

    auto* titleLabel = new QLabel("⚡ LiteForge IDE", this);
    titleLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #38bdf8;");
    layout->addWidget(titleLabel);

    auto* subTitleLabel = new QLabel("The Featherweight, Zero-Bloat Native C++ IDE", this);
    subTitleLabel->setStyleSheet("font-size: 13px; color: #a1a1aa; font-style: italic;");
    layout->addWidget(subTitleLabel);

    auto* descLabel = new QLabel(
        "LiteForge is an ultra-fast, minimalist, and feature-rich code editor and IDE "
        "crafted entirely in modern C++20 and Qt6.\n\n"
        "• Startup Time: < 40ms (Instantaneous)\n"
        "• Minimal Memory Footprint: Lean and unbloated native Qt6 architecture\n"
        "• Zero JavaScript/Node runtime overhead\n"
        "• High-performance multi-language syntax highlighting\n"
        "• Integrated terminal, build runner & diagnostics\n"
        "• Subsequence fuzzy finder & command palette\n"
        "• Native Wayland & X11 acceleration\n\n"
        "Open Source under the GNU General Public License v3 (GPLv3).", this
    );
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("font-size: 12px; line-height: 1.5; color: #e4e4e7;");
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
