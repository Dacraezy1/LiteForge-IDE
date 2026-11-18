#include "liteforge/ui/SettingsDialog.hpp"
#include "liteforge/core/Config.hpp"
#include "liteforge/core/Theme.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

namespace liteforge::ui {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("LiteForge Preferences");
    resize(520, 500);
    setupUi();
    loadCurrentSettings();
}

void SettingsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // Group 1: Appearance & Theme
    auto* appGroup = new QGroupBox("Appearance", this);
    auto* appLayout = new QFormLayout(appGroup);

    m_themeCombo = new QComboBox(appGroup);
    for (const auto& th : core::ThemeManager::instance().availableThemes()) {
        m_themeCombo->addItem(th);
    }
    appLayout->addRow("Theme:", m_themeCombo);

    m_fontCombo = new QFontComboBox(appGroup);
    m_fontCombo->setFontFilters(QFontComboBox::MonospacedFonts);
    appLayout->addRow("Editor Font:", m_fontCombo);

    m_fontSizeSpin = new QSpinBox(appGroup);
    m_fontSizeSpin->setRange(8, 36);
    appLayout->addRow("Font Size (pt):", m_fontSizeSpin);

    mainLayout->addWidget(appGroup);

    // Group 2: Editor Behavior
    auto* edGroup = new QGroupBox("Editor Settings", this);
    auto* edLayout = new QFormLayout(edGroup);

    m_tabWidthSpin = new QSpinBox(edGroup);
    m_tabWidthSpin->setRange(1, 16);
    edLayout->addRow("Tab Width:", m_tabWidthSpin);

    m_useSpacesCheck = new QCheckBox("Insert spaces instead of tabs", edGroup);
    edLayout->addRow("", m_useSpacesCheck);

    m_lineNumbersCheck = new QCheckBox("Show line numbers gutter", edGroup);
    edLayout->addRow("", m_lineNumbersCheck);

    m_highlightLineCheck = new QCheckBox("Highlight current line", edGroup);
    edLayout->addRow("", m_highlightLineCheck);

    m_autoCloseBracketsCheck = new QCheckBox("Auto-close brackets and quotes", edGroup);
    edLayout->addRow("", m_autoCloseBracketsCheck);

    m_wordWrapCheck = new QCheckBox("Enable soft word wrapping", edGroup);
    edLayout->addRow("", m_wordWrapCheck);

    m_trimTrailingCheck = new QCheckBox("Trim trailing whitespace on save", edGroup);
    edLayout->addRow("", m_trimTrailingCheck);

    mainLayout->addWidget(edGroup);

    // Group 3: Build & Run
    auto* buildGroup = new QGroupBox("Build & Run", this);
    auto* bLayout = new QFormLayout(buildGroup);

    m_customBuildEdit = new QLineEdit(buildGroup);
    m_customBuildEdit->setPlaceholderText("e.g. cmake --build build");
    bLayout->addRow("Custom Build:", m_customBuildEdit);

    m_customRunEdit = new QLineEdit(buildGroup);
    m_customRunEdit->setPlaceholderText("e.g. ./build/my_app");
    bLayout->addRow("Custom Run:", m_customRunEdit);

    mainLayout->addWidget(buildGroup);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    m_resetBtn = new QPushButton("Restore Defaults", this);
    m_applyBtn = new QPushButton("Apply", this);
    m_applyBtn->setDefault(true);
    m_closeBtn = new QPushButton("Close", this);

    btnLayout->addWidget(m_resetBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_applyBtn);
    btnLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(btnLayout);

    connect(m_applyBtn, &QPushButton::clicked, this, &SettingsDialog::onApply);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_resetBtn, &QPushButton::clicked, this, &SettingsDialog::onResetDefaults);
}

void SettingsDialog::loadCurrentSettings() {
    auto& cfg = core::Config::instance();
    const auto& ed = cfg.editor();

    m_themeCombo->setCurrentText(cfg.activeTheme());
    m_fontCombo->setCurrentFont(QFont(ed.fontFamily));
    m_fontSizeSpin->setValue(ed.fontSize);
    m_tabWidthSpin->setValue(ed.tabWidth);
    m_useSpacesCheck->setChecked(ed.useSpacesForTabs);
    m_lineNumbersCheck->setChecked(ed.showLineNumbers);
    m_highlightLineCheck->setChecked(ed.highlightCurrentLine);
    m_autoCloseBracketsCheck->setChecked(ed.autoCloseBrackets);
    m_wordWrapCheck->setChecked(ed.wordWrap);
    m_trimTrailingCheck->setChecked(ed.trimTrailingWhitespaceOnSave);

    const auto& b = cfg.build();
    m_customBuildEdit->setText(b.customBuildCommand);
    m_customRunEdit->setText(b.customRunCommand);
}

void SettingsDialog::onApply() {
    auto& cfg = core::Config::instance();
    auto& ed = cfg.editor();

    ed.fontFamily = m_fontCombo->currentFont().family();
    ed.fontSize = m_fontSizeSpin->value();
    ed.tabWidth = m_tabWidthSpin->value();
    ed.useSpacesForTabs = m_useSpacesCheck->isChecked();
    ed.showLineNumbers = m_lineNumbersCheck->isChecked();
    ed.highlightCurrentLine = m_highlightLineCheck->isChecked();
    ed.autoCloseBrackets = m_autoCloseBracketsCheck->isChecked();
    ed.wordWrap = m_wordWrapCheck->isChecked();
    ed.trimTrailingWhitespaceOnSave = m_trimTrailingCheck->isChecked();

    cfg.setActiveTheme(m_themeCombo->currentText());

    auto& b = cfg.build();
    b.customBuildCommand = m_customBuildEdit->text();
    b.customRunCommand = m_customRunEdit->text();

    cfg.save();
    emit settingsApplied();
}

void SettingsDialog::onResetDefaults() {
    core::EditorConfig defEd;
    m_themeCombo->setCurrentText("LiteForge Obsidian");
    m_fontCombo->setCurrentFont(QFont(defEd.fontFamily));
    m_fontSizeSpin->setValue(defEd.fontSize);
    m_tabWidthSpin->setValue(defEd.tabWidth);
    m_useSpacesCheck->setChecked(defEd.useSpacesForTabs);
    m_lineNumbersCheck->setChecked(defEd.showLineNumbers);
    m_highlightLineCheck->setChecked(defEd.highlightCurrentLine);
    m_autoCloseBracketsCheck->setChecked(defEd.autoCloseBrackets);
    m_wordWrapCheck->setChecked(defEd.wordWrap);
    m_trimTrailingCheck->setChecked(defEd.trimTrailingWhitespaceOnSave);
}

} // namespace liteforge::ui
