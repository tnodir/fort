#ifndef OPTIONSPAGE_H
#define OPTIONSPAGE_H

#include "basepage.h"

class OptionsPage : public BasePage
{
    Q_OBJECT

public:
    explicit OptionsPage(OptionsController *ctrl = nullptr,
                         QWidget *parent = nullptr);

protected slots:
    void onEditResetted() override;
    void onSaved() override;

    void onRetranslateUi() override;

private:
    bool iniEdited() const { return m_iniEdited; }
    void setIniEdited(bool v);

    void setupUi();
    void setupEditPassword();
    void retranslateEditPassword();
    void setupComboLanguage();

private:
    bool m_iniEdited = false;

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
};

#endif // OPTIONSPAGE_H
