#pragma once

#include <QDialog>
#include <QFontComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

namespace liteforge::ui {

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

signals:
    void settingsApplied();

private slots:
    void onApply();
    void onResetDefaults();

private:
    void setupUi();
    void loadCurrentSettings();

    QFontComboBox* m_fontCombo;
    QSpinBox* m_fontSizeSpin;
    QSpinBox* m_tabWidthSpin;
    QCheckBox* m_useSpacesCheck;
    QCheckBox* m_wordWrapCheck;
    QCheckBox* m_lineNumbersCheck;
    QCheckBox* m_highlightLineCheck;
    QCheckBox* m_autoCloseBracketsCheck;
    QCheckBox* m_showWhitespaceCheck;
    QCheckBox* m_trimTrailingCheck;

    QComboBox* m_themeCombo;
    QComboBox* m_densityCombo;
    QSpinBox* m_uiFontSizeSpin;
    QSpinBox* m_radiusSpin;
    QLineEdit* m_accentEdit;
    QCheckBox* m_toolbarCheck;
    QCheckBox* m_minimapCheck;
    QCheckBox* m_stickyCheck;
    QSpinBox* m_autoSaveSpin;
    QCheckBox* m_autoSaveFocusCheck;

    QLineEdit* m_customBuildEdit;
    QLineEdit* m_customRunEdit;
    QLineEdit* m_cmakeGeneratorEdit;

    QPushButton* m_applyBtn;
    QPushButton* m_closeBtn;
    QPushButton* m_resetBtn;
};

} // namespace liteforge::ui
