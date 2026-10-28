#include "pendingconnsearchmodel.h"

#include <fortglobal.h>
#include <stat/pendingconnmanager.h>

PendingConnSearchModel::PendingConnSearchModel(QObject *parent) : ConnSearchModel(parent) { }

StatConnManager *PendingConnSearchModel::connManager() const
{
    return Fort::pendingConnManager();
}

QString PendingConnSearchModel::sqlSearchWhere() const
{
    return ConnSearchModel::sqlSearchWhere() + " AND t.conn_time > ?3";
}

void PendingConnSearchModel::fillSearchVars(QVariantList &vars) const
{
    vars << PendingConnManager::expireTime();
}
