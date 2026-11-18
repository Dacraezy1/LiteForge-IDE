#pragma once

#include "liteforge/lsp/LSPProtocol.hpp"
#include <QListWidget>
#include <vector>

namespace liteforge::ui {

class CompletionPopup : public QListWidget {
    Q_OBJECT

public:
    explicit CompletionPopup(QWidget* parent = nullptr);
    ~CompletionPopup() override = default;

    void showCompletions(const std::vector<lsp::CompletionItem>& items, const QPoint& globalPos);

signals:
    void itemSelected(const QString& insertText);

protected:
    void keyPressEvent(QKeyEvent* event) override;
};

} // namespace liteforge::ui
