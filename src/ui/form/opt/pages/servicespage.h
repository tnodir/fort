#ifndef SERVICESPAGE_H
#define SERVICESPAGE_H

#include "optbasepage.h"

class ServiceInfoManager;
class ServiceListModel;
class TableView;

class ServicesPage : public OptBasePage
{
    Q_OBJECT

public:
    explicit ServicesPage(OptionsController *ctrl = nullptr, QWidget *parent = nullptr);

    ServiceListModel *serviceListModel() const { return m_serviceListModel; }
    ServiceInfoManager *serviceInfoManager() const;

public slots:
    void onPageActivated() override;

protected slots:
    void onRetranslateUi() override;

private:
    void setupUi();
    QLayout *setupHeader();
    void setupOptions();
    void setupTableServiceList();
    void setupTableServiceListHeader();

private:
    ServiceListModel *m_serviceListModel;

    QPushButton *m_btRefresh = nullptr;
    QPushButton *m_btEdit = nullptr;
    QAction *m_actEditService = nullptr;
    QPushButton *m_btOptions = nullptr;
    TableView *m_serviceListView = nullptr;
};

#endif // SERVICESPAGE_H
