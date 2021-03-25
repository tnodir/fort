#ifndef MAINPAGE_H
#define MAINPAGE_H

#include "basepage.h"

QT_FORWARD_DECLARE_CLASS(QTabWidget)

class AddressesPage;
class ApplicationsPage;
class OptionsPage;
class RulesPage;
class SchedulePage;
class StatisticsPage;

class MainPage : public BasePage
{
    Q_OBJECT

public:
    explicit MainPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
    void setupTabBar();
    QLayout *setupDialogButtons();
    void setupOkApplyButtons();

private:
    QTabWidget *m_tabBar = nullptr;

    QPushButton *m_btLogs = nullptr;
    QPushButton *m_btProfile = nullptr;
    QPushButton *m_btStat = nullptr;
    QPushButton *m_btReleases = nullptr;

    QPushButton *m_btOk = nullptr;
    QPushButton *m_btApply = nullptr;
    QPushButton *m_btCancel = nullptr;

    OptionsPage *m_optionsPage = nullptr;
    AddressesPage *m_addressesPage = nullptr;
    RulesPage *m_rulesPage = nullptr;
    ApplicationsPage *m_applicationsPage = nullptr;
    StatisticsPage *m_statisticsPage = nullptr;
    SchedulePage *m_schedulePage = nullptr;
};

#endif // MAINPAGE_H
