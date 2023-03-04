#include "deleteconnblockjob.h"

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <util/worker/workerobject.h>

#include "statblockmanager.h"
#include "statsql.h"

DeleteConnBlockJob::DeleteConnBlockJob(qint64 connIdTo) : m_connIdTo(connIdTo) { }

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
