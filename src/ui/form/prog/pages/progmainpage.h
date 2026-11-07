#ifndef PROGMAINPAGE_H
#define PROGMAINPAGE_H

#include "progbasepage.h"

QT_FORWARD_DECLARE_CLASS(QTabWidget)

class App;
class Conn;
class ProgGeneralPage;
class ProgNetworkPage;
class ProgRulesPage;

class ProgMainPage : public ProgBasePage
{
    Q_OBJECT

public:
    explicit ProgMainPage(ProgramEditController *ctrl, QWidget *parent = nullptr);

    QString helpSection() const override;

    void selectTab(int index);

    void setPendingConnsWindowChecked(bool checked);
    void setPendingConnsCount(int count);

    void setConnsWindowChecked(bool checked);

    void addConnFilter(const Conn &conn);

signals:
    void pendingConnsWindowToggled(bool checked);
    void connsWindowToggled(bool checked);

protected slots:
    void onValidateFields(bool &ok);
    void onFillApp(App &app);

    void onPageInitialize(const App &app) override;

    void onRetranslateUi() override;

private:
    void retranslateRulesTabText();

    void setupController();

    void setupUi();
    void setupTabBar();
    QLayout *setupButtonsLayout();

    bool isNetworkTabEnabled() const;
    void setFilterTabsEnabled(bool enabled);

    void onFilterSaved();

    void updatePendingConnsVisible();
    void updateServiceVisible();

    QString svcHostServiceName() const;

    ProgBasePage *currentPage() const;
    ProgBasePage *pageAt(int index) const;

private:
    int m_pendingConnsCount = 0;

    QTabWidget *m_tabWidget = nullptr;
    ProgGeneralPage *m_generalPage = nullptr;
    ProgNetworkPage *m_networkPage = nullptr;
    ProgRulesPage *m_rulesPage = nullptr;

    QToolButton *m_btPendingConns = nullptr;
    QToolButton *m_btConnections = nullptr;
    QPushButton *m_btMenu = nullptr;

    QToolButton *m_btService = nullptr;
    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    QList<ProgBasePage *> m_pages;
};

#endif // PROGMAINPAGE_H
