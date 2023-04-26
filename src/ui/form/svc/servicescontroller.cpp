#include "servicescontroller.h"

#include <manager/serviceinfomanager.h>
#include <model/servicelistmodel.h>
#include <util/ioc/ioccontainer.h>

ServicesController::ServicesController(QObject *parent) :
    BaseController(parent), m_serviceListModel(new ServiceListModel(this))
{
}

ServiceInfoManager *ServicesController::serviceInfoManager() const
{
    return IoC<ServiceInfoManager>();
}

void ServicesController::initialize()
{
    serviceListModel()->initialize();
}
