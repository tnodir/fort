#include "logpendingconnjob.h"

#include <sqlite/sqlitestmt.h>

#include "statsql.h"

LogPendingConnJob::LogPendingConnJob(const LogEntryConn &entry) : LogConnJob(entry) { }

const char *LogPendingConnJob::sqlInsertConn() const
{
    return StatSql::sqlInsertPendingConn;
}

void LogPendingConnJob::bindConn(
        SqliteStmt *stmt, const LogEntryConn &entry, qint64 appId, qint64 inheritAppId) const
{
    LogConnJob::bindConn(stmt, entry, appId, inheritAppId);

    stmt->bindInt64(ConnFlowId, qint64(entry.flowId()));
}
