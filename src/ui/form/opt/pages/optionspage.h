#ifndef OPTIONSPAGE_H
#define OPTIONSPAGE_H

#include "optbasepage.h"

class OptionsPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit OptionsPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

    bool passwordEdited() const { return m_passwordEdited; }
    void setPasswordEdited(bool v) { m_passwordEdited = v; }

public slots:
    void onResetToDefault() override;

protected slots:
    void onAboutToSave() override;
    void onEditResetted() override;

    void onRetranslateUi() override;

private:
    void retranslateComboBlockTraffic();
    void retranslateComboFilterMode();
    void retranslateEditPassword();
    void retranslateEditLanPlaceholderText();

    void setupUi();
    QLayout *setupColumn1();
    QLayout *setupColumn2();

    void setupTrafficBox();
    QLayout *setupBlockTrafficLayout();
    QLayout *setupFilterModeLayout();
    void setupProtectionBox();
    QLayout *setupPasswordLayout();
    void setupEditPassword();
    void setupPasswordLock();
    void setupUpdateBox();
    void setupLogsBox();
    void setupProgBox();
    void setupLogBlocked();
    void setupLanBox();
    QLayout *setupLanHeaderLayout();
    void setupEditLanText();

private:
    bool m_passwordEdited : 1 = false;

    QGroupBox *m_gbTraffic = nullptr;
    QGroupBox *m_gbProtection = nullptr;
    QGroupBox *m_gbUpdate = nullptr;
    QGroupBox *m_gbLogs = nullptr;
    QGroupBox *m_gbProg = nullptr;
    QGroupBox *m_gbLan = nullptr;

    QCheckBox *m_cbFilterEnabled = nullptr;
    QLabel *m_labelBlockTraffic = nullptr;
    QComboBox *m_comboBlockTraffic = nullptr;
    QLabel *m_labelFilterMode = nullptr;
    QComboBox *m_comboFilterMode = nullptr;
    QCheckBox *m_cbGroupBlocked = nullptr;

    QCheckBox *m_cbBootFilter = nullptr;
    QCheckBox *m_cbNoServiceControl = nullptr;
    QCheckBox *m_cbCheckPasswordOnUninstall = nullptr;
    QCheckBox *m_cbPassword = nullptr;
    QLineEdit *m_editPassword = nullptr;
    QToolButton *m_btPasswordLock = nullptr;

    QCheckBox *m_cbUpdateKeepCurrentVersion = nullptr;
    QCheckBox *m_cbUpdateAutoDownload = nullptr;
    QCheckBox *m_cbUpdateAutoInstall = nullptr;

    QCheckBox *m_cbLogDebug = nullptr;
    QCheckBox *m_cbLogConsole = nullptr;

    QCheckBox *m_cbLogBlocked = nullptr;
    QCheckBox *m_cbPurgeOnMounted = nullptr;

    QCheckBox *m_cbFilterLocals = nullptr;
    QCheckBox *m_cbFilterLocalNet = nullptr;

    QLabel *m_labelLanText = nullptr;
    QPlainTextEdit *m_editLanText = nullptr;
};

#endif // OPTIONSPAGE_H
