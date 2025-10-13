#include "statisticscontroller.h"

#include <fortglobal.h>
#include <stat/statconnmanager.h>
#include <stat/statmanager.h>

using namespace Fort;

StatisticsController::StatisticsController(QObject *parent) : BaseController(parent) { }

void StatisticsController::clearTraffic()
{
    statManager()->clearTraffic();
}

void StatisticsController::deleteStatApp(qint64 appId)
{
    statManager()->deleteStatApp(appId);
}

void StatisticsController::resetAppTotals()
{
    statManager()->resetAppTrafTotals();
}

void StatisticsController::deleteConn(qint64 connIdTo)
{
    statConnManager()->deleteConn(connIdTo);
}
