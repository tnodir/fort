#ifndef SERVICESWINDOW_H
#define SERVICESWINDOW_H

#include <form/windowtypes.h>
#include <util/window/widgetwindow.h>

QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class ConfManager;
class IniUser;
class ServiceInfoManager;
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

    quint32 windowCode() const override { return WindowServices; }

    ServicesController *ctrl() const { return m_ctrl; }
    ConfManager *confManager() const;
    IniUser *iniUser() const;
    WindowManager *windowManager() const;
    ServiceInfoManager *serviceInfoManager() const;
    ServiceListModel *serviceListModel() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

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

    void updateServiceListModel();
    int serviceListCurrentIndex() const;

private:
    ServicesController *m_ctrl = nullptr;
    WidgetWindowStateWatcher *m_stateWatcher = nullptr;

    QAction *m_actTrack = nullptr;
    QAction *m_actRevert = nullptr;
    QAction *m_actAddProgram = nullptr;
    QPushButton *m_btEdit = nullptr;
    QToolButton *m_btTrack = nullptr;
    QToolButton *m_btRevert = nullptr;
    QToolButton *m_btRefresh = nullptr;
    TableView *m_serviceListView = nullptr;
};

#endif // SERVICESWINDOW_H
