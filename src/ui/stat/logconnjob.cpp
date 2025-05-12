#include "logconnjob.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <util/worker/workerobject.h>

#include "statconnmanager.h"
#include "statsql.h"

namespace {

constexpr qint64 INVALID_APP_ID = Q_INT64_C(-1);
constexpr int MAX_LOG_BLOCKED_IP_MERGE_COUNT = 1000;

}

LogConnJob::LogConnJob(const LogEntryConn &entry)
{
    m_entries.append(entry);
}

bool LogConnJob::processMerge(const StatConnBaseJob &statJob)
{
    const auto &job = static_cast<const LogConnJob &>(statJob);

    if (m_entries.size() >= MAX_LOG_BLOCKED_IP_MERGE_COUNT)
        return false;

    m_entries.append(job.entries());

    return true;
}

void LogConnJob::processJob()
{
    int resultCount = 0;

    beginWriteTransaction();

    for (const LogEntryConn &entry : entries()) {
        if (processEntry(entry)) {
            ++resultCount;
        }
    }

    commitTransaction();

    setResultCount(resultCount);
}

void LogConnJob::emitFinished()
{
    emit manager()->logConnFinished(resultCount(), m_connId);
}

bool LogConnJob::processEntry(const LogEntryConn &entry)
{
    const qint64 appId = getOrCreateAppId(entry.path(), entry.appId(), entry.connTime());
    if (appId == INVALID_APP_ID)
        return false;

    const qint64 connId = insertConn(entry, appId);
    if (connId <= 0)
        return false;

    if (m_connId < connId) {
        m_connId = connId;
    }

    return true;
}

qint64 LogConnJob::getAppId(const QString &appPath)
{
    qint64 appId = INVALID_APP_ID;

    SqliteStmt *stmt = getStmt(StatSql::sqlSelectAppId);

    stmt->bindText(1, appPath);
    if (stmt->step() == SqliteStmt::StepRow) {
        appId = stmt->columnInt64();
    }
    stmt->reset();

    return appId;
}

qint64 LogConnJob::createAppId(const QString &appPath, quint32 confAppId, qint64 unixTime)
{
    SqliteStmt *stmt = getStmt(StatSql::sqlInsertAppId);

    stmt->bindInt64(1, confAppId);
    stmt->bindText(2, appPath);
    stmt->bindInt64(3, unixTime);

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return INVALID_APP_ID;
}

qint64 LogConnJob::getOrCreateAppId(const QString &appPath, quint32 confAppId, qint64 unixTime)
{
    qint64 appId = getAppId(appPath);
    if (appId == INVALID_APP_ID) {
        appId = createAppId(appPath, confAppId, unixTime);

        Q_ASSERT(appId != INVALID_APP_ID);
    }

    return appId;
}

qint64 LogConnJob::insertConn(const LogEntryConn &entry, qint64 appId)
{
    SqliteStmt *stmt = getStmt(StatSql::sqlInsertConn);

    stmt->bindInt64(1, appId);
    stmt->bindInt64(2, entry.connTime());
    stmt->bindInt(3, entry.pid());
    stmt->bindInt(4, entry.reason());
    stmt->bindInt(5, entry.blocked());
    stmt->bindInt(6, entry.inherited());
    stmt->bindInt(7, entry.inbound());
    stmt->bindInt(8, entry.ipProto());
    stmt->bindInt(9, entry.localPort());
    stmt->bindInt(10, entry.remotePort());

    if (!entry.isIPv6()) {
        stmt->bindInt(11, entry.localIp4());
        stmt->bindInt(12, entry.remoteIp4());
        stmt->bindNull(13);
        stmt->bindNull(14);
    } else {
        stmt->bindNull(11);
        stmt->bindNull(12);
        stmt->bindBlobView(13, entry.localIp6View());
        stmt->bindBlobView(14, entry.remoteIp6View());
    }

    stmt->bindInt(15, entry.zoneId());
    stmt->bindInt(16, entry.ruleId());

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return 0;
}
