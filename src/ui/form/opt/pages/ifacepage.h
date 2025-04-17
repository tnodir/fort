#ifndef IFACEPAGE_H
#define IFACEPAGE_H

#include "optbasepage.h"

QT_FORWARD_DECLARE_CLASS(QKeySequenceEdit)

class IfacePage : public OptBasePage
{
    Q_OBJECT

public:
    explicit IfacePage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

    bool languageEdited() const { return m_languageEdited; }
    void setLanguageEdited(bool v) { m_languageEdited = v; }

    bool themeEdited() const { return m_themeEdited; }
    void setThemeEdited(bool v) { m_themeEdited = v; }

    bool styleEdited() const { return m_styleEdited; }
    void setStyleEdited(bool v) { m_styleEdited = v; }

public slots:
    void onResetToDefault() override;

protected slots:
    void onEditResetted() override;

    void onRetranslateUi() override;

private:
    void retranslateComboLanguage();
    void retranslateComboTheme();
    void retranslateComboStyle();
    void retranslateComboHotKey();
    void retranslateComboTrayEvent();
    void retranslateComboTrayAction();

    void setupUi();
    QLayout *setupColumn1();
    QLayout *setupColumn2();

    void setupGlobalBox();
    QLayout *setupLangLayout();
    QLayout *setupThemeLayout();
    QLayout *setupStyleLayout();
    void setupHotKeysBox();
    void refreshEditShortcut();
    QLayout *setupComboHotKeyLayout();
    QLayout *setupEditShortcutLayout();
    void setupHomeBox();
    void setupProgBox();
    QLayout *setupAlertLayout();
    void setupAlertModes();
    void setupAlertModesButton();
    void setupTrayBox();
    QLayout *setupTrayMaxGroupsLayout();
    void refreshComboTrayAction();
    QLayout *setupTrayEventLayout();
    QLayout *setupTrayActionLayout();
    void setupConfirmationsBox();

    void updateTheme();
    void updateStyle();

private:
    bool m_languageEdited : 1 = false;
    bool m_themeEdited : 1 = false;
    bool m_styleEdited : 1 = false;

    QGroupBox *m_gbGlobal = nullptr;
    QGroupBox *m_gbHotKeys = nullptr;
    QGroupBox *m_gbHome = nullptr;
    QGroupBox *m_gbProg = nullptr;
    QGroupBox *m_gbTray = nullptr;
    QGroupBox *m_gbConfirmations = nullptr;

    QLabel *m_labelLanguage = nullptr;
    QComboBox *m_comboLanguage = nullptr;
    QLabel *m_labelTheme = nullptr;
    QComboBox *m_comboTheme = nullptr;
    QLabel *m_labelStyle = nullptr;
    QComboBox *m_comboStyle = nullptr;
    QCheckBox *m_cbUseSystemLocale = nullptr;
    QCheckBox *m_cbExcludeCapture = nullptr;

    QCheckBox *m_cbHotKeysEnabled = nullptr;
    QCheckBox *m_cbHotKeysGlobal = nullptr;
    QLabel *m_labelHotKey = nullptr;
    QComboBox *m_comboHotKey = nullptr;
    QLabel *m_labelShortcut = nullptr;
    QKeySequenceEdit *m_editShortcut = nullptr;

    QCheckBox *m_cbHomeAutoShowMenu = nullptr;
    QCheckBox *m_cbSplashVisible = nullptr;
    QCheckBox *m_cbUpdateWindowIcons = nullptr;

    QCheckBox *m_cbAppNotifyMessage = nullptr;
    QCheckBox *m_cbAppAlertAutoShow = nullptr;
    QCheckBox *m_cbAppAlertAutoLearn = nullptr;
    QCheckBox *m_cbAppAlertBlockAll = nullptr;
    QCheckBox *m_cbAppAlertAllowAll = nullptr;
    QPushButton *m_btAlertModes = nullptr;
    QCheckBox *m_cbAppAlertAlwaysOnTop = nullptr;
    QCheckBox *m_cbAppAlertAutoActive = nullptr;
    QCheckBox *m_cbAppAlertAutoClear = nullptr;
    QCheckBox *m_cbAppAlertSound = nullptr;
    QCheckBox *m_cbSnoozeAlerts = nullptr;

    QCheckBox *m_cbTrayShowIcon = nullptr;
    QCheckBox *m_cbTrayShowAlert = nullptr;
    QCheckBox *m_cbTrayAnimateAlert = nullptr;
    QLabel *m_labelTrayMaxGroups = nullptr;
    QSpinBox *m_spinTrayMaxGroups = nullptr;

    QLabel *m_labelTrayEvent = nullptr;
    QComboBox *m_comboTrayEvent = nullptr;
    QLabel *m_labelTrayAction = nullptr;
    QComboBox *m_comboTrayAction = nullptr;
    QCheckBox *m_cbConfirmTrayFlags = nullptr;
    QCheckBox *m_cbConfirmQuit = nullptr;
};

#endif // IFACEPAGE_H
