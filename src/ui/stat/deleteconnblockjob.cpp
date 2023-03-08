#include "deleteconnblockjob.h"

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <util/worker/workerobject.h>

#include "statblockmanager.h"
#include "statsql.h"

DeleteConnBlockJob::DeleteConnBlockJob(qint64 connIdTo, int keepCount) :
    m_keepCount(keepCount), m_connIdTo(connIdTo)
{
}

bool DeleteConnBlockJob::processMerge(const StatBlockBaseJob &statJob)
{
    const auto &job = static_cast<const DeleteConnBlockJob &>(statJob);

    if (connIdTo() <= 0) {
        m_keepCount = job.keepCount();
    } else if (job.connIdTo() <= 0 || connIdTo() < job.connIdTo()) {
        m_connIdTo = job.connIdTo();
    }

    return true;
}

void DeleteConnBlockJob::processJob()
{
    sqliteDb()->beginTransaction();

    checkKeepCount();

    if (connIdTo() > 0) {
        deleteConn(connIdTo());
    } else {
        deleteConnAll();
    }

    sqliteDb()->commitTransaction();

    setResultCount(1);
}

void DeleteConnBlockJob::emitFinished()
{
    emit manager()->deleteConnBlockFinished(connIdTo());
}

void DeleteConnBlockJob::checkKeepCount()
{
    if (keepCount() <= 0)
        return;

    qint64 idMin, idMax;
    manager()->getConnIdRange(sqliteDb(), idMin, idMax);

    const qint64 idMinKeep = idMax - keepCount();
    if (idMinKeep > idMin) {
        m_connIdTo = idMinKeep;
    }
}

void DeleteConnBlockJob::deleteConn(qint64 connIdTo)
{
    SqliteStmt::doList({ getIdStmt(StatSql::sqlDeleteConnBlock, connIdTo),
            getStmt(StatSql::sqlDeleteConnBlockApps) });
}

void DeleteConnBlockJob::deleteConnAll()
{
    SqliteStmt::doList(
            { getStmt(StatSql::sqlDeleteAllConnBlock), getStmt(StatSql::sqlDeleteAllApps) });

    sqliteDb()->vacuum();
}
