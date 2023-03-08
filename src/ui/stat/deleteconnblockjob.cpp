#include "deleteconnblockjob.h"

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <util/worker/workerobject.h>

#include "statblockmanager.h"
#include "statsql.h"

DeleteConnBlockJob::DeleteConnBlockJob(qint64 connIdTo) : m_connIdTo(connIdTo) { }

bool DeleteConnBlockJob::processMerge(const StatBlockBaseJob &statJob)
{
    const auto &job = static_cast<const DeleteConnBlockJob &>(statJob);

    if (connIdTo() > 0 && (job.connIdTo() <= 0 || connIdTo() < job.connIdTo())) {
        m_connIdTo = job.connIdTo();
    }

    return true;
}

void DeleteConnBlockJob::processJob()
{
    sqliteDb()->beginTransaction();

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
