#include "liteforge/ui/CommandPalette.hpp"
#include "liteforge/core/FileUtils.hpp"
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QFileInfo>
#include <algorithm>

namespace liteforge::ui {

CommandPalette::CommandPalette(QWidget* parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Popup) {
    setAttribute(Qt::WA_TranslucentBackground);

    auto* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto* container = new QWidget(this);
    container->setObjectName("paletteContainer");
    container->setStyleSheet(R"(
        QWidget#paletteContainer {
            background-color: #1e1e24;
            border: 1px solid #38bdf8;
            border-radius: 8px;
        }
        QLineEdit {
            background-color: #141416;
            color: #ffffff;
            border: 1px solid #27272a;
            border-radius: 6px;
            padding: 8px 12px;
            font-size: 14px;
        }
        QLineEdit:focus {
            border: 1px solid #38bdf8;
        }
        QListWidget {
            background-color: transparent;
            color: #d4d4d8;
            border: none;
            outline: none;
        }
        QListWidget::item {
            padding: 6px 10px;
            border-radius: 4px;
            font-size: 13px;
        }
        QListWidget::item:selected {
            background-color: #2b3245;
            color: #38bdf8;
        }
    )");

    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    m_searchEdit = new QLineEdit(container);
    m_searchEdit->setPlaceholderText("Type a command or file name...");
    m_searchEdit->installEventFilter(this);
    layout->addWidget(m_searchEdit);

    m_listWidget = new QListWidget(container);
    m_listWidget->setMaximumHeight(320);
    layout->addWidget(m_listWidget);

    outerLayout->addWidget(container);

    setFixedWidth(560);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &CommandPalette::onTextChanged);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &CommandPalette::onItemSelected);
}

void CommandPalette::registerCommand(const QString& category, const QString& title, const QString& shortcut, std::function<void()> action) {
    m_commands.push_back(PaletteCommand{title, category, shortcut, action});
}

void CommandPalette::setProjectFiles(const QStringList& files) {
    m_projectFiles = files;
}

void CommandPalette::showPalette(Mode mode) {
    m_mode = mode;
    m_searchEdit->clear();

    if (m_mode == Mode::Commands) {
        m_searchEdit->setPlaceholderText("Type a command to run...");
    } else {
        m_searchEdit->setPlaceholderText("Search file by name (Fuzzy)...");
    }

    updateList();

    // Center in parent window
    if (parentWidget()) {
        int x = parentWidget()->x() + (parentWidget()->width() - width()) / 2;
        int y = parentWidget()->y() + 60;
        move(x, y);
    }

    show();
    m_searchEdit->setFocus();
}

void CommandPalette::onTextChanged(const QString& text) {
    if (text.startsWith(">")) {
        m_mode = Mode::Commands;
        updateCommandList(text.mid(1).trimmed());
    } else if (m_mode == Mode::Commands) {
        updateCommandList(text.trimmed());
    } else {
        updateFileList(text.trimmed());
    }
}

void CommandPalette::updateList() {
    onTextChanged(m_searchEdit->text());
}

void CommandPalette::updateFileList(const QString& query) {
    m_listWidget->clear();

    struct ScoredFile {
        QString fullPath;
        QString display;
        int score;
    };

    std::vector<ScoredFile> matched;

    for (const QString& file : m_projectFiles) {
        QFileInfo fi(file);
        QString name = fi.fileName();

        if (query.isEmpty()) {
            matched.push_back({file, name + "  (" + file + ")", 0});
            if (matched.size() >= 30) break;
        } else {
            auto res = core::FileUtils::fuzzyMatch(query, name);
            if (res.matched) {
                matched.push_back({file, name + "  (" + file + ")", res.score});
            } else {
                auto resPath = core::FileUtils::fuzzyMatch(query, file);
                if (resPath.matched) {
                    matched.push_back({file, name + "  (" + file + ")", resPath.score - 10});
                }
            }
        }
    }

    if (!query.isEmpty()) {
        std::sort(matched.begin(), matched.end(), [](const ScoredFile& a, const ScoredFile& b) {
            return a.score > b.score;
        });
    }

    int count = std::min((int)matched.size(), 30);
    for (int i = 0; i < count; ++i) {
        auto* item = new QListWidgetItem(matched[i].display, m_listWidget);
        item->setData(Qt::UserRole, matched[i].fullPath);
    }

    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void CommandPalette::updateCommandList(const QString& query) {
    m_listWidget->clear();

    struct ScoredCmd {
        size_t index;
        int score;
    };

    std::vector<ScoredCmd> matched;

    for (size_t i = 0; i < m_commands.size(); ++i) {
        const auto& cmd = m_commands[i];
        QString fullText = cmd.category + ": " + cmd.title;

        if (query.isEmpty()) {
            matched.push_back({i, 0});
        } else {
            auto res = core::FileUtils::fuzzyMatch(query, fullText);
            if (res.matched) {
                matched.push_back({i, res.score});
            }
        }
    }

    if (!query.isEmpty()) {
        std::sort(matched.begin(), matched.end(), [](const ScoredCmd& a, const ScoredCmd& b) {
            return a.score > b.score;
        });
    }

    for (const auto& m : matched) {
        const auto& cmd = m_commands[m.index];
        QString label = QString("%1: %2").arg(cmd.category, cmd.title);
        if (!cmd.shortcut.isEmpty()) {
            label += QString("  [%1]").arg(cmd.shortcut);
        }

        auto* item = new QListWidgetItem(label, m_listWidget);
        item->setData(Qt::UserRole, static_cast<int>(m.index));
    }

    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void CommandPalette::onItemSelected(QListWidgetItem* item) {
    if (!item) return;

    if (m_mode == Mode::Commands || m_searchEdit->text().startsWith(">")) {
        int idx = item->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < static_cast<int>(m_commands.size())) {
            hide();
            m_commands[idx].action();
        }
    } else {
        QString filePath = item->data(Qt::UserRole).toString();
        hide();
        emit fileSelected(filePath);
    }
}

bool CommandPalette::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_searchEdit && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Down) {
            int row = m_listWidget->currentRow();
            if (row < m_listWidget->count() - 1) {
                m_listWidget->setCurrentRow(row + 1);
            }
            return true;
        } else if (keyEvent->key() == Qt::Key_Up) {
            int row = m_listWidget->currentRow();
            if (row > 0) {
                m_listWidget->setCurrentRow(row - 1);
            }
            return true;
        } else if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            onItemSelected(m_listWidget->currentItem());
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void CommandPalette::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        hide();
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

} // namespace liteforge::ui
