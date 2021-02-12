#ifndef CONNECTIONSWINDOW_H
#define CONNECTIONSWINDOW_H

#include "../../util/window/widgetwindow.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)

class ConnListModel;
class ConnectionsController;
class FirewallConf;
class FortManager;
class FortSettings;
class TableView;

class ConnectionsWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit ConnectionsWindow(FortManager *fortManager, QWidget *parent = nullptr);

protected slots:
    void onSaveWindowState();
    void onRestoreWindowState();

    void onRetranslateUi();

private:
    void setupController();

    void setupUi();
    QLayout *setupHeader();
    void setupLogOptions();
    void setupLogBlockedIp();
    void setupTableConnList();
    void setupTableConnListHeader();

    ConnectionsController *ctrl() const { return m_ctrl; }
    FortManager *fortManager() const;
    FortSettings *settings() const;
    FirewallConf *conf() const;
    ConnListModel *connListModel() const;

private:
    ConnectionsController *m_ctrl = nullptr;

    QPushButton *m_btLogOptions = nullptr;
    QCheckBox *m_cbLogBlockedIp = nullptr;
    TableView *m_connListView = nullptr;
};

#endif // CONNECTIONSWINDOW_H
