#ifndef PROGMAINPAGE_H
#define PROGMAINPAGE_H

#include "progbasepage.h"

QT_FORWARD_DECLARE_CLASS(QTabWidget)

class App;
class AppConnListModel;
class AppInfoRow;
class TableView;

struct ConnRow;

class ProgMainPage : public ProgBasePage
{
    Q_OBJECT

public:
    explicit ProgMainPage(ProgramEditController *ctrl, QWidget *parent = nullptr);

    AppConnListModel *appConnListModel() const { return m_appConnListModel; }

    void selectTab(int index);

protected slots:
    void onValidateFields(bool &ok);
    void onFillApp(App &app);

    void onPageInitialize(const App &app) override;

    void onRetranslateUi() override;

private:
    void retranslateTableConnListMenu();

    void setupController();

    void setupUi();
    void setupTabBar();
    QLayout *setupButtonsLayout();
    void setupSwitchWildcard();
    void setupConnections();
    void setupConnectionsMenuLayout();
    void closeConnectionsMenuLayout();
    void setupConnectionsModel();
    void setupTableConnList();
    void setupTableConnListMenu();
    void setupTableConnListHeader();
    void setupConnectionsAppInfoRow();
    void setupTableConnsChanged();

    int connListCurrentIndex() const;
    const ConnRow &connListCurrentRow() const;
    QString connListCurrentPath() const;

    void setNetworkTabEnabled(bool enabled);

    ProgBasePage *currentPage() const;
    ProgBasePage *pageAt(int index) const;

private:
    AppConnListModel *m_appConnListModel = nullptr;

    QTabWidget *m_tabWidget = nullptr;

    QPushButton *m_btMenu = nullptr;

    QToolButton *m_btSwitchWildcard = nullptr;

    QPushButton *m_btConnections = nullptr;
    QBoxLayout *m_connectionsLayout = nullptr;
    TableView *m_connListView = nullptr;
    QAction *m_actCopyAsFilter = nullptr;
    QAction *m_actCopy = nullptr;
    QAction *m_actLookupIp = nullptr;
    AppInfoRow *m_appInfoRow = nullptr;

    QPushButton *m_btOk = nullptr;
    QPushButton *m_btCancel = nullptr;

    QList<ProgBasePage *> m_pages;
};

#endif // PROGMAINPAGE_H
