#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include "homebasepage.h"

class HomePage : public HomeBasePage
{
    Q_OBJECT

public:
    explicit HomePage(HomeController *ctrl = nullptr, QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private:
    void retranslateDriverMessage();
    void retranslateServiceMessage();

    void setupUi();
    void setupDriverBox();
    QLayout *setupDriverLabelLayout();
    void setupDriverIcon();
    QLayout *setupDriverButtonsLayout();
    void setupServiceBox();
    QLayout *setupServiceLabelLayout();
    void setupServiceIcon();
    QLayout *setupServiceButtonsLayout();
    void setupPortableBox();

    void setServiceInstalled(bool install);

private:
    QGroupBox *m_gbDriver = nullptr;
    QGroupBox *m_gbService = nullptr;
    QGroupBox *m_gbPortable = nullptr;

    QLabel *m_iconDriver = nullptr;
    QLabel *m_labelDriverMessage = nullptr;
    QPushButton *m_btInstallDriver = nullptr;
    QPushButton *m_btRemoveDriver = nullptr;

    QLabel *m_iconService = nullptr;
    QLabel *m_labelServiceMessage = nullptr;
    QPushButton *m_btInstallService = nullptr;
    QPushButton *m_btRemoveService = nullptr;

    QPushButton *m_btUninstallPortable = nullptr;
};

#endif // HOMEPAGE_H
