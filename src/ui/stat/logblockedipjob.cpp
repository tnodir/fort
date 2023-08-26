#include "logblockedipjob.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <util/worker/workerobject.h>

#include "statblockmanager.h"
#include "statsql.h"

namespace {

constexpr qint64 INVALID_APP_ID = Q_INT64_C(-1);
constexpr int MAX_LOG_BLOCKED_IP_MERGE_COUNT = 1000;

}

LogBlockedIpJob::LogBlockedIpJob(const LogEntryBlockedIp &entry)
{
    m_entries.append(entry);
}

bool LogBlockedIpJob::processMerge(const StatBlockBaseJob &statJob)
{
    const auto &job = static_cast<const LogBlockedIpJob &>(statJob);

    if (m_entries.size() >= MAX_LOG_BLOCKED_IP_MERGE_COUNT)
        return false;

    m_entries.append(job.entries());

    return true;
}

void LogBlockedIpJob::processJob()
{
    int resultCount = 0;

    sqliteDb()->beginWriteTransaction();

    for (const LogEntryBlockedIp &entry : entries()) {
        if (processEntry(entry)) {
            ++resultCount;
        }
    }

    sqliteDb()->endTransaction();

    setResultCount(resultCount);
}

void LogBlockedIpJob::emitFinished()
{
    emit manager()->logBlockedIpFinished(resultCount(), m_connId);
}

bool LogBlockedIpJob::processEntry(const LogEntryBlockedIp &entry)
{
    const qint64 appId = getOrCreateAppId(entry.path(), entry.connTime());
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

qint64 LogBlockedIpJob::getAppId(const QString &appPath)
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

qint64 LogBlockedIpJob::createAppId(const QString &appPath, qint64 unixTime)
{
    SqliteStmt *stmt = getStmt(StatSql::sqlInsertAppId);

    stmt->bindText(1, appPath);
    stmt->bindInt64(2, unixTime);

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return INVALID_APP_ID;
}

qint64 LogBlockedIpJob::getOrCreateAppId(const QString &appPath, qint64 unixTime)
{
    qint64 appId = getAppId(appPath);
    if (appId == INVALID_APP_ID) {
        appId = createAppId(appPath, unixTime);
    }

    Q_ASSERT(appId != INVALID_APP_ID);

    return appId;
}

qint64 LogBlockedIpJob::insertConn(const LogEntryBlockedIp &entry, qint64 appId)
{
    SqliteStmt *stmt = getStmt(StatSql::sqlInsertConnBlock);

    stmt->bindInt64(1, appId);
    stmt->bindInt64(2, entry.connTime());
    stmt->bindInt(3, entry.pid());
    stmt->bindInt(4, entry.inbound());
    stmt->bindInt(5, entry.inherited());
    stmt->bindInt(6, entry.ipProto());
    stmt->bindInt(7, entry.localPort());
    stmt->bindInt(8, entry.remotePort());

    if (!entry.isIPv6()) {
        stmt->bindInt(9, entry.localIp4());
        stmt->bindInt(10, entry.remoteIp4());
        stmt->bindNull(11);
        stmt->bindNull(12);
    } else {
        stmt->bindNull(9);
        stmt->bindNull(10);
        stmt->bindBlob(11, entry.localIp6());
        stmt->bindBlob(12, entry.remoteIp6());
    }

    stmt->bindInt(13, entry.blockReason());

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return 0;
}
