#include "servicescontroller.h"

#include <model/servicelistmodel.h>

ServicesController::ServicesController(QObject *parent) :
    BaseController(parent), m_serviceListModel(new ServiceListModel(this))
{
}

void ServicesController::initialize()
{
    serviceListModel()->initialize();
}
