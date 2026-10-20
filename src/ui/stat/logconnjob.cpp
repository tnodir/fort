#include "logconnjob.h"

#include <QLoggingCategory>

#include <sqlite/dbvar.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <util/worker/workerobject.h>

#include "statconnmanager.h"
#include "statsql.h"

namespace {

inline constexpr qint64 INVALID_APP_ID = Q_INT64_C(-1);
inline constexpr int MAX_LOG_BLOCKED_IP_MERGE_COUNT = 1000;

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
    // A child process of the same program inherits its own path: create it with the conf app id
    const qint64 inheritAppId = getOrCreateInheritAppId(entry);
    if (inheritAppId == INVALID_APP_ID)
        return false;

    // The inherited connection's app is found by the inherited process's name
    const quint32 confAppId = entry.inherited() ? 0 : entry.appId();

    const qint64 appId = getOrCreateAppId(entry.path(), confAppId, entry.connTime());
    if (appId == INVALID_APP_ID)
        return false;

    const qint64 connId = insertConn(entry, appId, inheritAppId);
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

    stmt->bindVar(1, DbVar::nullable(confAppId));
    stmt->bindText(2, appPath);
    stmt->bindNull(3); // appName
    stmt->bindInt64(4, unixTime);

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

qint64 LogConnJob::getOrCreateInheritAppId(const LogEntryConn &entry)
{
    const QString inheritPath = entry.inheritPath();
    if (inheritPath.isEmpty())
        return 0;

    return getOrCreateAppId(inheritPath, entry.appId(), entry.connTime());
}

qint64 LogConnJob::insertConn(const LogEntryConn &entry, qint64 appId, qint64 inheritAppId)
{
    SqliteStmt *stmt = getStmt(sqlInsertConn());

    bindConn(stmt, entry, appId, inheritAppId);

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return 0;
}

const char *LogConnJob::sqlInsertConn() const
{
    return StatSql::sqlInsertConn;
}

void LogConnJob::bindConn(
        SqliteStmt *stmt, const LogEntryConn &entry, qint64 appId, qint64 inheritAppId) const
{
    stmt->bindInt64(ConnAppId, appId);
    stmt->bindInt64(ConnTime, entry.connTime());
    stmt->bindInt(ConnProcessId, entry.pid());
    stmt->bindInt(ConnReason, entry.reason());
    stmt->bindBool(ConnBlocked, entry.blocked());
    stmt->bindBool(ConnDropped, entry.dropped());
    stmt->bindBool(ConnAlerted, entry.alerted());
    stmt->bindBool(ConnTerminated, entry.terminated());
    stmt->bindBool(ConnInherited, entry.inherited());
    stmt->bindBool(ConnInbound, entry.inbound());
    stmt->bindInt(ConnIpProto, entry.ipProto());
    stmt->bindInt(ConnLocalPort, entry.localPort());
    stmt->bindInt(ConnRemotePort, entry.remotePort());

    if (!entry.isIPv6()) {
        stmt->bindInt(ConnLocalIp, entry.localIp4());
        stmt->bindInt(ConnRemoteIp, entry.remoteIp4());
        stmt->bindNull(ConnLocalIp6);
        stmt->bindNull(ConnRemoteIp6);
    } else {
        stmt->bindNull(ConnLocalIp);
        stmt->bindNull(ConnRemoteIp);
        stmt->bindBlobView(ConnLocalIp6, entry.localIp6View());
        stmt->bindBlobView(ConnRemoteIp6, entry.remoteIp6View());
    }

    stmt->bindInt(ConnZoneId, entry.zoneId());
    stmt->bindInt(ConnRuleId, entry.ruleId());
    stmt->bindBool(ConnLoopback, entry.loopback());
    stmt->bindVar(ConnInheritAppId, DbVar::nullable(inheritAppId));
}
