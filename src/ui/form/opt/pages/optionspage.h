#ifndef OPTIONSPAGE_H
#define OPTIONSPAGE_H

#include "optbasepage.h"

QT_FORWARD_DECLARE_CLASS(QKeySequenceEdit)

class OptionsPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit OptionsPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

    bool passwordEdited() const { return m_passwordEdited; }
    void setPasswordEdited(bool v) { m_passwordEdited = v; }

    bool languageEdited() const { return m_languageEdited; }
    void setLanguageEdited(bool v) { m_languageEdited = v; }

    bool explorerEdited() const { return m_explorerEdited; }
    void setExplorerEdited(bool v) { m_explorerEdited = v; }

public slots:
    void onResetToDefault() override;

protected slots:
    void onAboutToSave() override;
    void onEditResetted() override;

    void onRetranslateUi() override;

private:
    void saveAutoRunMode(int mode);
    void saveService(bool isService);

    void retranslateComboStartMode();
    void retranslateComboFilterMode();
    void retranslateEditPassword();
    void retranslateComboHotKey();
    void retranslateComboTrayEvent();
    void retranslateComboTrayAction();

    void setupStartup();

    void setupUi();
    QLayout *setupColumn1();
    void setupStartupBox();
    QLayout *setupStartModeLayout();
    void setupTrafficBox();
    QLayout *setupFilterModeLayout();
    void setupProtectionBox();
    QLayout *setupPasswordLayout();
    void setupEditPassword();
    void setupPasswordLock();
    void setupProgBox();
    void setupLogBlocked();

    QLayout *setupColumn2();
    void setupGlobalBox();
    QLayout *setupLangLayout();
    void setupComboLanguage();
    void setupHotKeysBox();
    void refreshEditShortcut();
    QLayout *setupComboHotKeyLayout();
    QLayout *setupEditShortcutLayout();
    void setupHomeBox();
    void setupTrayBox();
    QLayout *setupTrayMaxGroupsLayout();
    void refreshComboTrayAction();
    QLayout *setupTrayEventLayout();
    QLayout *setupTrayActionLayout();
    void setupConfirmationsBox();
    void setupLogsBox();

private:
    bool m_passwordEdited : 1 = false;
    bool m_languageEdited : 1 = false;
    bool m_explorerEdited : 1 = false;

    qint8 m_currentAutoRunMode = 0;

    QGroupBox *m_gbStartup = nullptr;
    QGroupBox *m_gbTraffic = nullptr;
    QGroupBox *m_gbProtection = nullptr;
    QGroupBox *m_gbProg = nullptr;
    QGroupBox *m_gbGlobal = nullptr;
    QGroupBox *m_gbHotKeys = nullptr;
    QGroupBox *m_gbHome = nullptr;
    QGroupBox *m_gbTray = nullptr;
    QGroupBox *m_gbConfirmations = nullptr;
    QGroupBox *m_gbLogs = nullptr;

    QLabel *m_labelStartMode = nullptr;
    QComboBox *m_comboAutoRun = nullptr;
    QCheckBox *m_cbService = nullptr;
    QCheckBox *m_cbFilterEnabled = nullptr;
    QCheckBox *m_cbBlockTraffic = nullptr;
    QCheckBox *m_cbBlockInetTraffic = nullptr;
    QLabel *m_labelFilterMode = nullptr;
    QComboBox *m_comboFilterMode = nullptr;
    QCheckBox *m_cbBootFilter = nullptr;
    QCheckBox *m_cbFilterLocals = nullptr;
    QCheckBox *m_cbNoServiceControl = nullptr;
    QCheckBox *m_cbCheckPasswordOnUninstall = nullptr;
    QCheckBox *m_cbPassword = nullptr;
    QLineEdit *m_editPassword = nullptr;
    QToolButton *m_btPasswordLock = nullptr;
    QCheckBox *m_cbLogBlocked = nullptr;
    QCheckBox *m_cbAppNotifyMessage = nullptr;
    QCheckBox *m_cbAppAlertAutoShow = nullptr;
    QCheckBox *m_cbAppAlertAlwaysOnTop = nullptr;
    QCheckBox *m_cbPurgeOnMounted = nullptr;

    QCheckBox *m_cbExplorerMenu = nullptr;
    QCheckBox *m_cbUseSystemLocale = nullptr;
    QLabel *m_labelLanguage = nullptr;
    QComboBox *m_comboLanguage = nullptr;
    QCheckBox *m_cbHotKeysEnabled = nullptr;
    QCheckBox *m_cbHotKeysGlobal = nullptr;
    QLabel *m_labelHotKey = nullptr;
    QComboBox *m_comboHotKey = nullptr;
    QLabel *m_labelShortcut = nullptr;
    QKeySequenceEdit *m_editShortcut = nullptr;

    QCheckBox *m_cbHomeAutoShowMenu = nullptr;
    QCheckBox *m_cbSplashVisible = nullptr;
    QCheckBox *m_cbTrayShowIcon = nullptr;
    QCheckBox *m_cbTrayAnimateAlert = nullptr;
    QLabel *m_labelTrayMaxGroups = nullptr;
    QSpinBox *m_spinTrayMaxGroups = nullptr;

    QLabel *m_labelTrayEvent = nullptr;
    QComboBox *m_comboTrayEvent = nullptr;
    QLabel *m_labelTrayAction = nullptr;
    QComboBox *m_comboTrayAction = nullptr;
    QCheckBox *m_cbConfirmTrayFlags = nullptr;
    QCheckBox *m_cbConfirmQuit = nullptr;
    QCheckBox *m_cbLogDebug = nullptr;
    QCheckBox *m_cbLogConsole = nullptr;
};

#endif // OPTIONSPAGE_H
