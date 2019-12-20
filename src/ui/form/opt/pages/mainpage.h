#ifndef MAINPAGE_H
#define MAINPAGE_H

#include "basepage.h"

QT_FORWARD_DECLARE_CLASS(QTabWidget)

QT_FORWARD_DECLARE_CLASS(AddressesPage)
QT_FORWARD_DECLARE_CLASS(ApplicationsPage)
QT_FORWARD_DECLARE_CLASS(OptionsPage)
QT_FORWARD_DECLARE_CLASS(SchedulePage)
QT_FORWARD_DECLARE_CLASS(StatisticsPage)

class MainPage : public BasePage
{
    Q_OBJECT

public:
    explicit MainPage(OptionsController *ctrl = nullptr,
                      QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private slots:
    void onLinkClicked();

private:
    void setupUi();
    QLayout *setupDialogButtons();
    void setupNewVersionButton();
    void setupOkApplyButtons();

private:
    QTabWidget *m_tabBar = nullptr;

    QPushButton *m_logsButton = nullptr;
    QPushButton *m_profileButton = nullptr;
    QPushButton *m_statButton = nullptr;
    QPushButton *m_releasesButton = nullptr;
    QPushButton *m_newVersionButton = nullptr;

    QPushButton *m_okButton = nullptr;
    QPushButton *m_applyButton = nullptr;
    QPushButton *m_cancelButton = nullptr;

    OptionsPage *m_optionsPage = nullptr;
    AddressesPage *m_addressesPage = nullptr;
    ApplicationsPage *m_applicationsPage = nullptr;
    StatisticsPage *m_statisticsPage = nullptr;
    SchedulePage *m_schedulePage = nullptr;
};

#endif // MAINPAGE_H
