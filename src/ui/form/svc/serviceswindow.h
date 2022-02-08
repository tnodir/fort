#ifndef SERVICESWINDOW_H
#define SERVICESWINDOW_H

#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QPushButton)

class ConfManager;
class IniUser;
class ServiceListModel;
class ServicesController;
class TableView;
class WidgetWindowStateWatcher;
class WindowManager;

class ServicesWindow : public WidgetWindow
{
    Q_OBJECT

public:
    explicit ServicesWindow(QWidget *parent = nullptr);

    ServicesController *ctrl() const { return m_ctrl; }
    ConfManager *confManager() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;
    ServiceListModel *serviceListModel() const;

    void saveWindowState();
    void restoreWindowState();

private:
    void setupController();
    void setupStateWatcher();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupOptions();
    void setupTableServiceList();
    void setupTableServiceListHeader();
    void setupTableServicesChanged();

    int serviceListCurrentIndex() const;

private:
    ServicesController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;

    QAction *m_actTrack = nullptr;
    QAction *m_actRevert = nullptr;
    QAction *m_actAddProgram = nullptr;
    QPushButton *m_btTrack = nullptr;
    QPushButton *m_btRevert = nullptr;
    QPushButton *m_btEdit = nullptr;
    QPushButton *m_btRefresh = nullptr;
    TableView *m_serviceListView = nullptr;
};

#endif // SERVICESWINDOW_H
