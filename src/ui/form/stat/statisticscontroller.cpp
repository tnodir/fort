#include "statisticscontroller.h"

#include <stat/statconnmanager.h>
#include <util/ioc/ioccontainer.h>

StatisticsController::StatisticsController(QObject *parent) : BaseController(parent) { }

StatConnManager *StatisticsController::statConnManager() const
{
    return IoC<StatConnManager>();
}

void StatisticsController::deleteConn(qint64 connIdTo)
{
    statConnManager()->deleteConn(connIdTo);
}
