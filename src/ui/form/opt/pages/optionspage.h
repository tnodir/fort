#ifndef OPTIONSPAGE_H
#define OPTIONSPAGE_H

#include "basepage.h"

class OptionsPage : public BasePage
{
    Q_OBJECT

public:
    explicit OptionsPage(OptionsController *ctrl = nullptr,
                         QWidget *parent = nullptr);

    bool iniEdited() const { return m_iniEdited; }
    void setIniEdited(bool v);

protected slots:
    void onEditResetted() override;
    void onSaved() override;

    void onRetranslateUi() override;

private:
    void retranslateEditPassword();
    void retranslateDriverMessage();

    void setupUi();
    void setupEditPassword();
    void setupComboLanguage();
    void setupDriverBox();
    void setupDriverIcon();
    void setupNewVersionBox();
    void setupNewVersionUpdate();

private:
    bool m_iniEdited = false;

    QGroupBox *m_gbStartup = nullptr;
    QGroupBox *m_gbTraffic = nullptr;
    QGroupBox *m_gbGlobal = nullptr;
    QGroupBox *m_gbDriver = nullptr;
    QGroupBox *m_gbNewVersion = nullptr;
    QCheckBox *m_cbStart = nullptr;
    QCheckBox *m_cbProvBoot = nullptr;
    QCheckBox *m_cbFilterEnabled = nullptr;
    QCheckBox *m_cbFilterLocals = nullptr;
    QCheckBox *m_cbStopTraffic = nullptr;
    QCheckBox *m_cbStopInetTraffic = nullptr;
    QCheckBox *m_cbHotKeys = nullptr;
    QCheckBox *m_cbPassword = nullptr;
    QLineEdit *m_editPassword = nullptr;
    QLabel *m_labelLanguage = nullptr;
    QComboBox *m_comboLanguage = nullptr;
    QLabel *m_iconDriver = nullptr;
    QLabel *m_labelDriverMessage = nullptr;
    QPushButton *m_btInstallDriver = nullptr;
    QPushButton *m_btRemoveDriver = nullptr;
    QLabel *m_labelNewVersion = nullptr;
    QPushButton *m_btNewVersion = nullptr;
};

#endif // OPTIONSPAGE_H
