#ifndef SERVICESWINDOW_H
#define SERVICESWINDOW_H

#include <form/controls/formwindow.h>

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QToolButton)

class ServiceInfo;
class ServiceListModel;
class ServicesController;
class TableView;

class ServicesWindow : public FormWindow
{
    Q_OBJECT

public:
    explicit ServicesWindow(QWidget *parent = nullptr);

    WindowCode windowCode() const override { return WindowServices; }
    QString windowOverlayIconPath() const override { return ":/icons/windows-48.png"; }

    ServicesController *ctrl() const { return m_ctrl; }
    ServiceListModel *serviceListModel() const;

    void saveWindowState(bool wasVisible) override;
    void restoreWindowState() override;

private:
    void setupController();

    void retranslateUi();

    void setupUi();
    QLayout *setupHeader();
    void setupEditSearch();
    void setupOptions();
    void setupTableServiceList();
    void setupTableServiceListHeader();
    void setupTableServicesChanged();

    void updateServiceListModel();
    const ServiceInfo *currentServiceInfo() const;
    int serviceListCurrentIndex() const;

private:
    ServicesController *m_ctrl = nullptr;

    QAction *m_actTrack = nullptr;
    QAction *m_actRevert = nullptr;
    QAction *m_actAddProgram = nullptr;
    QAction *m_actFind = nullptr;
    QPushButton *m_btEdit = nullptr;
    QToolButton *m_btTrack = nullptr;
    QToolButton *m_btRevert = nullptr;
    QToolButton *m_btRefresh = nullptr;
    QLineEdit *m_editSearch = nullptr;
    QToolButton *m_btOptions = nullptr;
    QPushButton *m_btMenu = nullptr;
    TableView *m_serviceListView = nullptr;
};

#endif // SERVICESWINDOW_H
