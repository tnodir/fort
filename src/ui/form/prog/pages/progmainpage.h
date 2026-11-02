#ifndef PROGMAINPAGE_H
#define PROGMAINPAGE_H

#include "progbasepage.h"

QT_FORWARD_DECLARE_CLASS(QTabWidget)

class App;
class Conn;
class ProgNetworkPage;

class ProgMainPage : public ProgBasePage
{
    Q_OBJECT

public:
    explicit ProgMainPage(ProgramEditController *ctrl, QWidget *parent = nullptr);

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
    void setupController();

    void setupUi();
    void setupTabBar();
    QLayout *setupButtonsLayout();

    void setNetworkTabEnabled(bool enabled);

    void updatePendingConnsVisible();

    ProgBasePage *currentPage() const;
    ProgBasePage *pageAt(int index) const;

private:
    int m_pendingConnsCount = 0;

    QTabWidget *m_tabWidget = nullptr;
    ProgNetworkPage *m_networkPage = nullptr;

    QToolButton *m_btPendingConns = nullptr;
    QToolButton *m_btConnections = nullptr;
    QPushButton *m_btMenu = nullptr;

    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    QList<ProgBasePage *> m_pages;
};

#endif // PROGMAINPAGE_H
