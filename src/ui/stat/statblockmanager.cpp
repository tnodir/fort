#include "statblockmanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <util/ioc/ioccontainer.h>

#include "deleteconnblockjob.h"
#include "logblockedipjob.h"
#include "statblockworker.h"
#include "statsql.h"

namespace {

const QLoggingCategory LC("statBlock");

constexpr int DATABASE_USER_VERSION = 7;

}

StatBlockManager::StatBlockManager(const QString &filePath, QObject *parent, quint32 openFlags) :
    WorkerManager(parent),
    m_isConnIdRangeUpdated(false),
    m_sqliteDb(new SqliteDb(filePath, openFlags, this)),
    m_roSqliteDb((openFlags & SqliteDb::OpenReadWrite) != 0
                    ? new SqliteDb(filePath, SqliteDb::OpenDefaultReadOnly, this)
                    : m_sqliteDb),
    m_connChangedTimer(500)
{
    connect(&m_connChangedTimer, &QTimer::timeout, this, &StatBlockManager::connChanged);
}

void StatBlockManager::emitConnChanged()
{
    m_connChangedTimer.startTrigger();
}

void StatBlockManager::setUp()
{
    setupWorker();
    setupConfManager();

    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return;
    }

    SqliteDb::MigrateOptions opt = {
        .sqlDir = ":/stat/migrations/block", .version = DATABASE_USER_VERSION, .recreate = true
    };

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
}

void StatBlockManager::deleteConn(qint64 connIdTo)
{
    enqueueJob(new DeleteConnBlockJob(connIdTo));
}

void StatBlockManager::onLogBlockedIpFinished(int count, qint64 newConnId)
{
    emitConnChanged();

    m_connBlockIdMax = newConnId;
    if (m_connBlockIdMin <= 0) {
        m_connBlockIdMin = m_connBlockIdMax;
    }

    constexpr int connBlockIncMax = 99;

    m_connBlockInc += count;
    if (m_connBlockInc < connBlockIncMax)
        return;

    m_connBlockInc = 0;

    const int totalCount = m_connBlockIdMax - m_connBlockIdMin;
    const int oldCount = totalCount - m_blockedIpKeepCount;
    if (oldCount <= 0)
        return;

    deleteConn(m_connBlockIdMin + oldCount);
}

void StatBlockManager::onDeleteConnBlockFinished(qint64 connIdTo)
{
    emitConnChanged();

    if (connIdTo > 0) {
        m_connBlockIdMin = connIdTo + 1;
    } else {
        m_connBlockIdMin = m_connBlockIdMax;
    }

    if (m_connBlockIdMin >= m_connBlockIdMax) {
        m_connBlockIdMin = m_connBlockIdMax = 0;
    }
}

WorkerObject *StatBlockManager::createWorker()
{
    return new StatBlockWorker(this);
}

void StatBlockManager::setupWorker()
{
    setMaxWorkersCount(1);

    connect(this, &StatBlockManager::logBlockedIpFinished, this,
            &StatBlockManager::onLogBlockedIpFinished);
    connect(this, &StatBlockManager::deleteConnBlockFinished, this,
            &StatBlockManager::onDeleteConnBlockFinished);
}

void StatBlockManager::setupConfManager()
{
    auto confManager = IoC()->setUpDependency<ConfManager>();

    connect(confManager, &ConfManager::iniChanged, this, &StatBlockManager::setupByConf);
}

void StatBlockManager::setupByConf(const IniOptions &ini)
{
    m_blockedIpKeepCount = ini.blockedIpKeepCount();
}
