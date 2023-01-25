#ifndef SERVICESCONTROLLER_H
#define SERVICESCONTROLLER_H

#include <form/basecontroller.h>

class ServiceInfoManager;
class ServiceListModel;

class ServicesController : public BaseController
{
    Q_OBJECT

public:
    explicit ServicesController(QObject *parent = nullptr);

    ServiceInfoManager *serviceInfoManager() const;
    ServiceListModel *serviceListModel() const { return m_serviceListModel; }

    void initialize();

private:
    ServiceListModel *m_serviceListModel = nullptr;
};

#endif // SERVICESCONTROLLER_H
