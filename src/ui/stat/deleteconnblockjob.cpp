#include "deleteconnblockjob.h"

#include <sqlite/dbutil.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <util/worker/workerobject.h>

#include "statblockmanager.h"
#include "statsql.h"

DeleteConnBlockJob::DeleteConnBlockJob(qint64 connIdTo) : m_connIdTo(connIdTo) { }

bool DeleteConnBlockJob::processMerge(const StatBlockBaseJob &statJob)
{
    if (connIdTo() <= 0)
        return true; // already delete all

    const auto &job = static_cast<const DeleteConnBlockJob &>(statJob);

    if (job.connIdTo() <= 0 || connIdTo() < job.connIdTo()) {
        m_connIdTo = job.connIdTo();
    }

    return true;
}

void DeleteConnBlockJob::processJob()
{
    const bool isDeleteAll = (connIdTo() <= 0);

    sqliteDb()->beginWriteTransaction();

    SqliteStmtList stmtList;
    if (isDeleteAll) {
        stmtList = { getStmt(StatSql::sqlDeleteAllConnBlock), getStmt(StatSql::sqlDeleteAllApps) };
    } else {
        stmtList = { getIdStmt(StatSql::sqlDeleteConnBlock, connIdTo()),
            getStmt(StatSql::sqlDeleteConnBlockApps) };
    }

    DbUtil::doList(stmtList);

    sqliteDb()->commitTransaction();

    if (isDeleteAll) {
        sqliteDb()->vacuum(); // Vacuum outside of transaction
    }

    setResultCount(1);
}

void DeleteConnBlockJob::emitFinished()
{
    emit manager()->deleteConnBlockFinished(connIdTo());
}
