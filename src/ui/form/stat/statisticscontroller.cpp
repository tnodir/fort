#include "statisticscontroller.h"

#include <stat/statblockmanager.h>
#include <util/ioc/ioccontainer.h>

StatisticsController::StatisticsController(QObject *parent) : BaseController(parent) { }

StatBlockManager *StatisticsController::statBlockManager() const
{
    return IoC<StatBlockManager>();
}

void StatisticsController::deleteBlockedConn(qint64 connIdTo)
{
    statBlockManager()->deleteConn(connIdTo);
}
