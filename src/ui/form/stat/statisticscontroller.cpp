#include "statisticscontroller.h"

#include <stat/statconnmanager.h>
#include <stat/statmanager.h>
#include <util/ioc/ioccontainer.h>

StatisticsController::StatisticsController(QObject *parent) : BaseController(parent) { }

StatManager *StatisticsController::statManager() const
{
    return IoC<StatManager>();
}

StatConnManager *StatisticsController::statConnManager() const
{
    return IoC<StatConnManager>();
}

void StatisticsController::clearTraffic()
{
    statManager()->clearTraffic();
}

void StatisticsController::resetAppTotals()
{
    statManager()->resetAppTrafTotals();
}

void StatisticsController::deleteConn(qint64 connIdTo)
{
    statConnManager()->deleteConn(connIdTo);
}
