#include "deleteconnjob.h"

#include <sqlite/dbutil.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <util/worker/workerobject.h>

#include "statconnmanager.h"
#include "statsql.h"

DeleteConnJob::DeleteConnJob(qint64 connIdTo) : m_connIdTo(connIdTo) { }

bool DeleteConnJob::processMerge(const StatConnBaseJob &statJob)
{
    if (connIdTo() <= 0)
        return true; // already delete all

    const auto &job = static_cast<const DeleteConnJob &>(statJob);

    if (job.connIdTo() <= 0 || connIdTo() < job.connIdTo()) {
        m_connIdTo = job.connIdTo();
    }

    return true;
}

void DeleteConnJob::processJob()
{
    const bool isDeleteAll = (connIdTo() <= 0);

    sqliteDb()->beginWriteTransaction();

    SqliteStmtList stmtList;
    if (isDeleteAll) {
        stmtList = { getStmt(StatSql::sqlDeleteAllConn), getStmt(StatSql::sqlDeleteAllApps) };
    } else {
        stmtList = { getIdStmt(StatSql::sqlDeleteConn, connIdTo()),
            getStmt(StatSql::sqlDeleteConnApps) };
    }

    DbUtil::doList(stmtList);

    sqliteDb()->commitTransaction();

    if (isDeleteAll) {
        sqliteDb()->vacuum(); // Vacuum outside of transaction
    }

    setResultCount(1);
}

void DeleteConnJob::emitFinished()
{
    emit manager()->deleteConnFinished(connIdTo());
}
