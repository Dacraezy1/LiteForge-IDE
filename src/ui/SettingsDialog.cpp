#include "liteforge/ui/SettingsDialog.hpp"
#include "liteforge/core/Config.hpp"
#include "liteforge/core/Theme.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QColor>

namespace liteforge::ui {

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("LiteForge Preferences");
    resize(540, 620);
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

    m_densityCombo = new QComboBox(appGroup);
    m_densityCombo->addItems({"compact", "normal", "comfortable"});
    appLayout->addRow("UI Density:", m_densityCombo);

    m_uiFontSizeSpin = new QSpinBox(appGroup);
    m_uiFontSizeSpin->setRange(0, 24);
    m_uiFontSizeSpin->setSpecialValueText("System default");
    appLayout->addRow("UI Font Size (pt):", m_uiFontSizeSpin);

    m_radiusSpin = new QSpinBox(appGroup);
    m_radiusSpin->setRange(0, 12);
    appLayout->addRow("Corner Radius (px):", m_radiusSpin);

    m_accentEdit = new QLineEdit(appGroup);
    m_accentEdit->setPlaceholderText("#rrggbb  (empty = theme accent)");
    appLayout->addRow("Accent Color:", m_accentEdit);

    m_toolbarCheck = new QCheckBox("Show toolbar", appGroup);
    appLayout->addRow("", m_toolbarCheck);

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

    m_minimapCheck = new QCheckBox("Show minimap", edGroup);
    edLayout->addRow("", m_minimapCheck);

    m_stickyCheck = new QCheckBox("Sticky scroll (pin enclosing scope)", edGroup);
    edLayout->addRow("", m_stickyCheck);

    m_autoSaveSpin = new QSpinBox(edGroup);
    m_autoSaveSpin->setRange(0, 600);
    m_autoSaveSpin->setSpecialValueText("Off");
    m_autoSaveSpin->setSuffix(" s");
    edLayout->addRow("Auto-save after:", m_autoSaveSpin);

    m_autoSaveFocusCheck = new QCheckBox("Auto-save when window loses focus", edGroup);
    edLayout->addRow("", m_autoSaveFocusCheck);

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
    m_minimapCheck->setChecked(ed.minimap);
    m_stickyCheck->setChecked(ed.stickyScroll);
    m_autoSaveSpin->setValue(ed.autoSaveDelayMs / 1000);
    m_autoSaveFocusCheck->setChecked(ed.autoSaveOnFocusLoss);

    const auto& ui = cfg.ui();
    m_densityCombo->setCurrentText(ui.density);
    m_uiFontSizeSpin->setValue(ui.fontSize);
    m_radiusSpin->setValue(ui.cornerRadius);
    m_accentEdit->setText(ui.accentOverride);
    m_toolbarCheck->setChecked(ui.showToolbar);

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
    ed.minimap = m_minimapCheck->isChecked();
    ed.stickyScroll = m_stickyCheck->isChecked();
    ed.autoSaveDelayMs = m_autoSaveSpin->value() * 1000;
    ed.autoSaveOnFocusLoss = m_autoSaveFocusCheck->isChecked();

    auto& ui = cfg.ui();
    ui.density = m_densityCombo->currentText();
    ui.fontSize = m_uiFontSizeSpin->value();
    ui.cornerRadius = m_radiusSpin->value();
    const QString accent = m_accentEdit->text().trimmed();
    ui.accentOverride = QColor(accent).isValid() ? accent : QString();
    ui.showToolbar = m_toolbarCheck->isChecked();

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
    m_minimapCheck->setChecked(defEd.minimap);
    m_stickyCheck->setChecked(defEd.stickyScroll);
    m_autoSaveSpin->setValue(0);
    m_autoSaveFocusCheck->setChecked(false);
    core::UiConfig defUi;
    m_densityCombo->setCurrentText(defUi.density);
    m_uiFontSizeSpin->setValue(defUi.fontSize);
    m_radiusSpin->setValue(defUi.cornerRadius);
    m_accentEdit->clear();
    m_toolbarCheck->setChecked(defUi.showToolbar);
}

} // namespace liteforge::ui
