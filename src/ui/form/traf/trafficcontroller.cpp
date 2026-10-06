#include "trafficcontroller.h"

#include <fortglobal.h>
#include <stat/statmanager.h>

using namespace Fort;

TrafficController::TrafficController(QObject *parent) : BaseController(parent) { }

void TrafficController::clearTraffic()
{
    statManager()->clearTraffic();
}

void TrafficController::deleteStatApp(qint64 appId)
{
    statManager()->deleteStatApp(appId);
}

void TrafficController::resetAppTotals()
{
    statManager()->resetAppTrafTotals();
}
