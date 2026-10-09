#ifndef PROGMAINPAGE_H
#define PROGMAINPAGE_H

#include "progbasepage.h"

QT_FORWARD_DECLARE_CLASS(QTabWidget)

class App;
class Conn;

class ProgMainPage : public ProgBasePage
{
    Q_OBJECT

public:
    explicit ProgMainPage(ProgramEditController *ctrl, QWidget *parent = nullptr);

    void selectTab(int index);

    void setConnsWindowChecked(bool checked);

signals:
    void connsWindowToggled(bool checked);
    void addConnFilterRequested(const Conn &conn);

protected slots:
    void onValidateFields(bool &ok);
    void onFillApp(App &app);

    void onPageInitialize(const App &app) override;

    void onRetranslateUi() override;

private:
    void setupController();

    void setupUi();
    void setupTabBar();
    QLayout *setupButtonsLayout();
    void setupSwitchWildcard();

    void setNetworkTabEnabled(bool enabled);

    ProgBasePage *currentPage() const;
    ProgBasePage *pageAt(int index) const;

private:
    QTabWidget *m_tabWidget = nullptr;

    QToolButton *m_btConnections = nullptr;
    QPushButton *m_btMenu = nullptr;

    QToolButton *m_btSwitchWildcard = nullptr;

    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    QList<ProgBasePage *> m_pages;
};

#endif // PROGMAINPAGE_H
