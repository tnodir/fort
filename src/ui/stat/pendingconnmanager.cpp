#include "pendingconnmanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/app.h>
#include <conf/confappmanager.h>
#include <conf/firewallconf.h>
#include <driver/drivercommon.h>
#include <driver/drivermanager.h>
#include <fortglobal.h>
#include <log/logentryconn.h>
#include <util/dateutil.h>
#include <util/fileutil.h>

#include "deletependingconnjob.h"
#include "deletetimedoutconnsjob.h"
#include "logpendingconnjob.h"
#include "statsql.h"

namespace {

const QLoggingCategory LC("pendingConn");

inline constexpr int timerPeriodMsec = 1000;

bool updateDriver(QByteArray &buf, quint32 code)
{
    auto driverManager = Fort::driverManager();

    if (!driverManager->writePending(buf, code)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    return true;
}

quint32 decisionIoctl(PendingConnManager::ConnDecision decision)
{
    switch (decision) {
    case PendingConnManager::ConnBlock:
        return DriverCommon::ioctlBlockPending();
    case PendingConnManager::ConnAllow:
        return DriverCommon::ioctlAllowPending();
    default: // Continue
        return DriverCommon::ioctlReauthPending();
    }
}

/* The driver finds the flows by the binary search */
bool updateDriverFlows(QVector<quint64> flowIds, quint32 code)
{
    std::sort(flowIds.begin(), flowIds.end());

    QByteArray buf((const char *) flowIds.constData(), flowIds.size() * sizeof(quint64));

    return updateDriver(buf, code);
}

}

PendingConnManager::PendingConnManager(
        const QString &filePath, QObject *parent, quint32 openFlags) :
    StatConnManager(filePath, parent, openFlags)
{
}

void PendingConnManager::setUp()
{
    StatConnManager::setUp();

    // The pending connections don't survive the restart: don't wait for the disk writes
    if ((sqliteDb()->openFlags() & SqliteDb::OpenReadOnly) == 0) {
        sqliteDb()->setSynchronous(SqliteDb::SyncOff);
    }
}

bool PendingConnManager::logConn(const LogEntryConn &entry, bool urgent)
{
    if (!StatConnManager::logConn(entry, urgent))
        return false;

    m_lastConnTime = entry.connTime();

    if (!m_timer.isActive()) {
        m_timer.start();
    }

    return true;
}

bool PendingConnManager::decideConn(qint64 connId, ConnDecision decision)
{
    const auto list = DbQuery(roSqliteDb())
                              .sql(StatSql::sqlSelectConnFlow)
                              .vars({ connId, expireTime() })
                              .execute(2)
                              .toList();

    const quint64 flowId = list.value(0).toULongLong();
    const bool inbound = list.value(1).toBool();

    if (flowId == 0)
        return false;

    // The inbound one's flow id is changed by its re-injection: allowed by the filters only
    if (decision == ConnAllow && inbound)
        return false;

    // The not decided connection stays pending till its timeout
    if (!updateDriverFlows({ flowId }, decisionIoctl(decision)))
        return false;

    removeConns({ connId });

    return true;
}

qint64 PendingConnManager::expireTime()
{
    return DateUtil::getUnixTime() - Fort::conf().askToConnectSeconds();
}

void PendingConnManager::setupWorker()
{
    StatConnManager::setupWorker();

    m_timer.setInterval(timerPeriodMsec);

    connect(&m_timer, &QTimer::timeout, this, &PendingConnManager::deleteTimedOutConns);

    connect(this, &PendingConnManager::timedOutConnsDeleted, this,
            &PendingConnManager::blockTimedOutConns);
}

void PendingConnManager::setupConfManager()
{
    auto confAppManager = Fort::dependency<ConfAppManager>();

    connect(confAppManager, &ConfAppManager::appsDriverUpdated, this,
            &PendingConnManager::reauthApps);
}

LogConnJob *PendingConnManager::createLogConnJob(const LogEntryConn &entry) const
{
    return new LogPendingConnJob(entry);
}

const char *PendingConnManager::migrationsSqlDir() const
{
    return ":/stat/migrations/conn_pending";
}

void PendingConnManager::checkClearConnOnStartup()
{
    /* The pending connections don't survive the restart: recreate the DB by the current schema */
    FileUtil::removeFilesByPrefix(sqliteDb()->filePath());
}

void PendingConnManager::checkClearConnOnExit()
{
    deleteConn();

    runQueuedJobs();
}

void PendingConnManager::reauthApps(const QVector<qint64> &appIdList)
{
    auto confAppManager = Fort::confAppManager();

    for (const qint64 appId : appIdList) {
        const App app = confAppManager->appById(appId);

        // The asked program's pending connections are kept
        if (app.isValid() && !isAppAsked(app)) {
            reauthAppConns(app);
        }
    }
}

void PendingConnManager::reauthAppConns(const App &app)
{
    SqliteStmt stmt;
    if (!DbQuery(roSqliteDb())
                    .sql(StatSql::sqlSelectAppPendingConns)
                    .vars({ app.appId, app.connFilterPath(), expireTime() })
                    .prepare(stmt))
        return;

    QVector<qint64> connIds;
    QVector<quint64> flowIds;

    while (stmt.step() == SqliteStmt::StepRow) {
        connIds.append(stmt.columnInt64(0));
        flowIds.append(stmt.columnInt64(1));
    }

    if (connIds.isEmpty())
        return;

    if (!updateDriverFlows(flowIds, DriverCommon::ioctlReauthPending()))
        return;

    removeConns(connIds);
}

void PendingConnManager::removeConns(const QVector<qint64> &connIds)
{
    // The decided connections are no longer pending
    enqueueJob(WorkerJobPtr(new DeletePendingConnJob(connIds)));
}

void PendingConnManager::deleteTimedOutConns()
{
    const qint64 expireTime = PendingConnManager::expireTime();

    // Processed after the queued logging: finds all the logged connections
    enqueueJob(WorkerJobPtr(new DeleteTimedOutConnsJob(expireTime)));

    /* All the logged connections are timed out */
    if (m_lastConnTime <= expireTime) {
        m_timer.stop();
    }
}

void PendingConnManager::blockTimedOutConns(const QVector<quint64> &flowIds)
{
    updateDriverFlows(flowIds, DriverCommon::ioctlBlockPending());

    emitConnChanged();
}
