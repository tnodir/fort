#include "statconnmanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/confmanager.h>
#include <conf/firewallconf.h>
#include <util/fileutil.h>
#include <util/ioc/ioccontainer.h>

#include "deleteconnjob.h"
#include "logconnjob.h"
#include "statconnworker.h"
#include "statsql.h"

namespace {

const QLoggingCategory LC("statConn");

constexpr int DATABASE_USER_VERSION = 3;

bool migrateFunc(SqliteDb *db, int version, bool isNewDb, void *ctx)
{
    Q_UNUSED(db);
    Q_UNUSED(version);
    Q_UNUSED(isNewDb);
    Q_UNUSED(ctx);

#if 0
    if (isNewDb) {
        // COMPAT: DB schema
        return true;
    }
#endif

    return true;
}

}

StatConnManager::StatConnManager(const QString &filePath, QObject *parent, quint32 openFlags) :
    WorkerManager(parent),
    m_sqliteDb(new SqliteDb(filePath, openFlags)),
    m_roSqliteDb((openFlags == 0 || (openFlags & SqliteDb::OpenReadWrite) != 0)
                    ? SqliteDbPtr::create(filePath, SqliteDb::OpenDefaultReadOnly)
                    : m_sqliteDb),
    m_connChangedTimer(500)
{
    connect(&m_connChangedTimer, &QTimer::timeout, this, &StatConnManager::connChanged);
}

void StatConnManager::emitConnChanged()
{
    m_connChangedTimer.startTrigger();
}

void StatConnManager::setUp()
{
    setupWorker();
    setupConfManager();

    checkCearConnOnStartup();

    setupDb();
}

void StatConnManager::tearDown()
{
    abortWorkers();

    checkCearConnOnExit();
}

void StatConnManager::logConn(const LogEntryConn &entry)
{
    if (!(entry.blocked() ? m_logBlockedConn : m_logAllowedConn))
        return;

    constexpr int maxJobCount = 16;
    if (jobCount() >= maxJobCount)
        return; // drop excessive data

    enqueueJob(WorkerJobPtr(new LogConnJob(entry)));
}

void StatConnManager::deleteConn(qint64 connIdTo)
{
    if (connIdTo <= 0) {
        clear(); // delete all
    }

    enqueueJob(WorkerJobPtr(new DeleteConnJob(connIdTo)));
}

void StatConnManager::getConnIdRange(SqliteDb *db, qint64 &connIdMin, qint64 &connIdMax)
{
    const auto vars = DbQuery(db).sql(StatSql::sqlSelectMinMaxConnId).execute(2).toList();

    connIdMin = vars.value(0).toLongLong();
    connIdMax = vars.value(1).toLongLong();
}

void StatConnManager::onLogConnFinished(int count, qint64 /*newConnId*/)
{
    emitConnChanged();

    if (m_keepCount <= 0)
        return;

    constexpr int connIncMax = 99;

    m_connInc += count;
    if (m_connInc < connIncMax)
        return;

    m_connInc = 0;

    qint64 idMin, idMax;
    getConnIdRange(roSqliteDb(), idMin, idMax);

    const qint64 idMinKeep = idMax - m_keepCount;
    if (idMinKeep > 0 && idMinKeep > idMin) {
        deleteConn(idMinKeep);
    }
}

void StatConnManager::onDeleteConnFinished(qint64 /*connIdTo*/)
{
    emitConnChanged();
}

WorkerObject *StatConnManager::createWorker()
{
    return new StatConnWorker(this);
}

void StatConnManager::setupWorker()
{
    setMaxWorkersCount(1);

    connect(this, &StatConnManager::logConnFinished, this, &StatConnManager::onLogConnFinished);
    connect(this, &StatConnManager::deleteConnFinished, this,
            &StatConnManager::onDeleteConnFinished);
}

void StatConnManager::setupConfManager()
{
    auto confManager = IoCDependency<ConfManager>();

    connect(confManager, &ConfManager::confChanged, this, &StatConnManager::setupByConf);
    connect(confManager, &ConfManager::iniChanged, this, &StatConnManager::setupByConfIni);
}

bool StatConnManager::setupDb()
{
    if (!sqliteDb()->open()) {
        qCCritical(LC) << "File open error:" << sqliteDb()->filePath()
                       << sqliteDb()->errorMessage();
        return false;
    }

    SqliteDb::MigrateOptions opt = {
        .sqlDir = ":/stat/migrations/conn",
        .version = DATABASE_USER_VERSION,
        .recreate = true,
        .migrateFunc = &migrateFunc,
    };

    if (!sqliteDb()->migrate(opt)) {
        qCCritical(LC) << "Migration error" << sqliteDb()->filePath();
        return false;
    }

    if (roSqliteDb() != sqliteDb()) {
        if (!roSqliteDb()->open()) {
            qCCritical(LC) << "File open error:" << roSqliteDb()->filePath()
                           << roSqliteDb()->errorMessage();
            return false;
        }
    }

    return true;
}

void StatConnManager::setupByConf()
{
    FirewallConf *conf = this->conf();

    m_logAllowedConn = conf->logAllowedConn();
    m_logBlockedConn = conf->logBlockedConn();

    sqliteDb()->setSynchronous(conf->clearConnOnExit() ? SqliteDb::SyncOff : SqliteDb::SyncNormal);
}

void StatConnManager::setupByConfIni(const IniOptions &ini)
{
    m_keepCount = ini.connKeepCount();
}

void StatConnManager::checkCearConnOnStartup()
{
    removeDbFilesToCleanOpen();
}

void StatConnManager::checkCearConnOnExit()
{
    if (!conf()->clearConnOnExit())
        return;

    deleteConn();
}
