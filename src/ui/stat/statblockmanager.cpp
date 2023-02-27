#include "statblockmanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/firewallconf.h>
#include <util/dateutil.h>
#include <util/ioc/ioccontainer.h>

#include "logblockedipjob.h"
#include "statblockworker.h"
#include "statsql.h"

namespace {

const QLoggingCategory LC("statBlock");

constexpr int DATABASE_USER_VERSION = 7;

constexpr qint64 INVALID_APP_ID = Q_INT64_C(-1);

bool migrateFunc(SqliteDb *db, int version, bool isNewDb, void *ctx)
{
    Q_UNUSED(ctx);

    if (isNewDb)
        return true;

    switch (version) {
    }

    return true;
}

}

StatBlockManager::StatBlockManager(const QString &filePath, QObject *parent, quint32 openFlags) :
    WorkerManager(parent),
    m_isConnIdRangeUpdated(false),
    m_sqliteDb(new SqliteDb(filePath, openFlags, this)),
    m_connChangedTimer(500)
{
    setMaxWorkersCount(1);

    connect(&m_connChangedTimer, &QTimer::timeout, this, &StatBlockManager::connChanged);
}

void StatBlockManager::emitConnChanged()
{
    m_connChangedTimer.startTrigger();
}

void StatBlockManager::setUp()
{
    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return;
    }

    SqliteDb::MigrateOptions opt = { .sqlDir = ":/stat/migrations/block",
        .version = DATABASE_USER_VERSION,
        .recreate = true,
        .migrateFunc = &migrateFunc };

    if (!sqliteDb()->migrate(opt)) {
        qCCritical(LC) << "Migration error" << sqliteDb()->filePath();
        return;
    }

    updateConnBlockId();
}

void StatBlockManager::updateConnBlockId()
{
    if (isConnIdRangeUpdated())
        return;

    setIsConnIdRangeUpdated(true);

    const auto vars = sqliteDb()->executeEx(StatSql::sqlSelectMinMaxConnBlockId, {}, 2).toList();
    m_connBlockIdMin = vars.value(0).toLongLong();
    m_connBlockIdMax = vars.value(1).toLongLong();
}

bool StatBlockManager::logBlockedIp(const LogEntryBlockedIp &entry, qint64 unixTime)
{
    auto job = new LogBlockedIpJob(unixTime);
    job->entry() = entry;

    enqueueJob(job);

    return true;

    //
    bool ok = false;
    sqliteDb()->beginTransaction();

    const qint64 appId = getOrCreateAppId(entry.path(), unixTime);
    if (appId != INVALID_APP_ID) {
        ok = createConnBlock(entry, unixTime, appId);
    }

    sqliteDb()->endTransaction();

    if (ok) {
        emitConnChanged();

        constexpr int connBlockIncMax = 100;
        if (++m_connBlockInc >= connBlockIncMax) {
            m_connBlockInc = 0;
            deleteOldConnBlock();
        }
    }

    return ok;
}

bool StatBlockManager::deleteOldConnBlock()
{
    const int keepCount = ini()->blockedIpKeepCount();
    const int totalCount = m_connBlockIdMax - m_connBlockIdMin + 1;
    const int oldCount = totalCount - keepCount;
    if (oldCount <= 0)
        return false;

    deleteConnBlock(m_connBlockIdMin + oldCount - 1);

    emitConnChanged();

    return true;
}

bool StatBlockManager::deleteConn(qint64 rowIdTo)
{
    sqliteDb()->beginTransaction();

    deleteConnBlock(rowIdTo);

    sqliteDb()->commitTransaction();

    emitConnChanged();

    return true;
}

bool StatBlockManager::deleteConnAll()
{
    sqliteDb()->beginTransaction();

    deleteAppStmtList({ sqliteDb()->stmt(StatSql::sqlDeleteAllConn),
                              sqliteDb()->stmt(StatSql::sqlDeleteAllConnBlock) },
            sqliteDb()->stmt(StatSql::sqlSelectDeletedAllConnAppList));

    sqliteDb()->vacuum();

    sqliteDb()->commitTransaction();

    m_connBlockIdMin = m_connBlockIdMax = 0;

    emitConnChanged();

    return true;
}

qint64 StatBlockManager::getAppId(const QString &appPath)
{
    qint64 appId = INVALID_APP_ID;

    SqliteStmt *stmt = sqliteDb()->stmt(StatSql::sqlSelectAppId);

    stmt->bindText(1, appPath);
    if (stmt->step() == SqliteStmt::StepRow) {
        appId = stmt->columnInt64();
    }
    stmt->reset();

    return appId;
}

qint64 StatBlockManager::createAppId(const QString &appPath, qint64 unixTime)
{
    SqliteStmt *stmt = sqliteDb()->stmt(StatSql::sqlInsertAppId);

    stmt->bindText(1, appPath);
    stmt->bindInt64(2, unixTime);

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return INVALID_APP_ID;
}

qint64 StatBlockManager::getOrCreateAppId(const QString &appPath, qint64 unixTime)
{
    qint64 appId = getCachedAppId(appPath);
    if (appId == INVALID_APP_ID) {
        appId = getAppId(appPath);
        if (appId == INVALID_APP_ID) {
            if (unixTime == 0) {
                unixTime = DateUtil::getUnixTime();
            }
            appId = createAppId(appPath, unixTime);
        }

        Q_ASSERT(appId != INVALID_APP_ID);

        addCachedAppId(appPath, appId);
    }
    return appId;
}

bool StatBlockManager::deleteAppId(qint64 appId)
{
    SqliteStmt *stmt = getIdStmt(StatSql::sqlDeleteAppId, appId);

    return sqliteDb()->done(stmt);
}

qint64 StatBlockManager::insertConn(const LogEntryBlockedIp &entry, qint64 unixTime, qint64 appId)
{
    SqliteStmt *stmt = sqliteDb()->stmt(StatSql::sqlInsertConn);

    stmt->bindInt64(1, appId);
    stmt->bindInt64(2, unixTime);
    stmt->bindInt(3, entry.pid());
    stmt->bindInt(4, entry.inbound());
    stmt->bindInt(5, entry.inherited());
    stmt->bindInt(6, entry.ipProto());
    stmt->bindInt(7, entry.localPort());
    stmt->bindInt(8, entry.remotePort());

    if (entry.isIPv6()) {
        stmt->bindNull(9);
        stmt->bindBlob(10, entry.localIp6());
        stmt->bindNull(11);
        stmt->bindBlob(12, entry.remoteIp6());
    } else {
        stmt->bindInt(9, entry.localIp4());
        stmt->bindNull(10);
        stmt->bindInt(11, entry.remoteIp4());
        stmt->bindNull(12);
    }

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return 0;
}

qint64 StatBlockManager::insertConnBlock(qint64 connId, quint8 blockReason)
{
    SqliteStmt *stmt = sqliteDb()->stmt(StatSql::sqlInsertConnBlock);

    stmt->bindInt64(1, connId);
    stmt->bindInt(2, blockReason);

    if (sqliteDb()->done(stmt)) {
        return sqliteDb()->lastInsertRowid();
    }

    return 0;
}

bool StatBlockManager::createConnBlock(
        const LogEntryBlockedIp &entry, qint64 unixTime, qint64 appId)
{
    const qint64 connId = insertConn(entry, unixTime, appId);
    if (connId <= 0)
        return false;

    const qint64 connBlockId = insertConnBlock(connId, entry.blockReason());
    if (connBlockId <= 0)
        return false;

    m_connBlockIdMax = connBlockId;
    if (m_connBlockIdMin <= 0) {
        m_connBlockIdMin = m_connBlockIdMax;
    }

    return true;
}

void StatBlockManager::deleteConnBlock(qint64 rowIdTo)
{
    deleteAppStmtList({ getIdStmt(StatSql::sqlDeleteConnForBlock, rowIdTo),
                              getIdStmt(StatSql::sqlDeleteConnBlock, rowIdTo) },
            getStmt(StatSql::sqlSelectDeletedConnBlockAppList));

    m_connBlockIdMin = rowIdTo + 1;
    if (m_connBlockIdMin >= m_connBlockIdMax) {
        m_connBlockIdMin = m_connBlockIdMax = 0;
    }
}

void StatBlockManager::deleteAppStmtList(const SqliteStmtList &stmtList, SqliteStmt *stmtAppList)
{
    // Delete Statements
    SqliteStmt::doList(stmtList);

    // Delete Cached AppIds
    {
        while (stmtAppList->step() == SqliteStmt::StepRow) {
            const qint64 appId = stmtAppList->columnInt64(0);
            const QString appPath = stmtAppList->columnText(1);

            deleteAppId(appId);
            clearCachedAppId(appPath);
        }
        stmtAppList->reset();
    }
}

SqliteStmt *StatBlockManager::getStmt(const char *sql)
{
    return sqliteDb()->stmt(sql);
}

SqliteStmt *StatBlockManager::getTrafficStmt(const char *sql, qint32 trafTime)
{
    SqliteStmt *stmt = getStmt(sql);

    stmt->bindInt(1, trafTime);

    return stmt;
}

SqliteStmt *StatBlockManager::getIdStmt(const char *sql, qint64 id)
{
    SqliteStmt *stmt = getStmt(sql);

    stmt->bindInt64(1, id);

    return stmt;
}

WorkerObject *StatBlockManager::createWorker()
{
    return new StatBlockWorker(this);
}
