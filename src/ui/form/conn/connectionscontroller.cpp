#include "connectionscontroller.h"

#include <fortglobal.h>
#include <stat/statconnmanager.h>

using namespace Fort;

ConnectionsController::ConnectionsController(QObject *parent) : BaseController(parent) { }

void ConnectionsController::deleteConn(qint64 connIdTo)
{
    statConnManager()->deleteConn(connIdTo);
}

void ConnectionsController::deleteConns(const QVector<qint64> &connIds)
{
    statConnManager()->deleteConns(connIds);
}
