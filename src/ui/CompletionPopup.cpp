#include "liteforge/ui/CompletionPopup.hpp"
#include <QKeyEvent>

namespace liteforge::ui {

CompletionPopup::CompletionPopup(QWidget* parent)
    : QListWidget(parent) {
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setUniformItemSizes(true);
    setStyleSheet("QListWidget { background-color: #21252b; color: #abb2bf; border: 1px solid #181a1f; }"
                  "QListWidget::item { padding: 4px 8px; }"
                  "QListWidget::item:selected { background-color: #2c313a; color: #61afef; }");

    connect(this, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        if (item) {
            emit itemSelected(item->data(Qt::UserRole).toString());
            hide();
        }
    });
}

void CompletionPopup::showCompletions(const std::vector<lsp::CompletionItem>& items, const QPoint& globalPos) {
    clear();
    if (items.empty()) {
        hide();
        return;
    }

    int count = 0;
    for (const auto& item : items) {
        if (count++ > 30) break; // Keep snappy
        QString displayText = item.label;
        if (!item.detail.isEmpty()) {
            displayText += " — " + item.detail;
        }
        auto* listItem = new QListWidgetItem(displayText, this);
        listItem->setData(Qt::UserRole, item.insertText);
    }

    setCurrentRow(0);
    adjustSize();
    resize(std::min(400, std::max(200, sizeHintForColumn(0) + 30)), std::min(220, count * 24 + 10));
    move(globalPos);
    show();
}

void CompletionPopup::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Tab) {
        if (currentItem()) {
            emit itemSelected(currentItem()->data(Qt::UserRole).toString());
            hide();
            event->accept();
            return;
        }
    } else if (event->key() == Qt::Key_Escape) {
        hide();
        event->accept();
        return;
    }
    QListWidget::keyPressEvent(event);
}

} // namespace liteforge::ui
