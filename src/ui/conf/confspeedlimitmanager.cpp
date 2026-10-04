#include "confspeedlimitmanager.h"

#include <QLoggingCategory>

#include <sqlite/dbquery.h>
#include <sqlite/dbvar.h>
#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include <conf/speedlimit.h>
#include <driver/drivermanager.h>
#include <fortglobal.h>
#include <util/conf/confbuffer.h>
#include <util/conf/confutil.h>
#include <util/dateutil.h>
#include <util/ioc/ioccontainer.h>

#include "confmanager.h"
#include "conftimeperiodmanager.h"

namespace {

const QLoggingCategory LC("confSpeedLimit");

#define SELECT_SPEED_LIMIT_FIELDS                                                                  \
    "    t.limit_id,"                                                                              \
    "    t.enabled,"                                                                               \
    "    t.inbound,"                                                                               \
    "    t.period_enabled,"                                                                        \
    "    t.period_id,"                                                                             \
    "    t.packet_loss,"                                                                           \
    "    t.latency,"                                                                               \
    "    t.kbps,"                                                                                  \
    "    t.bufsize,"                                                                               \
    "    t.name,"                                                                                  \
    "    t.notes,"                                                                                 \
    "    t.mod_time"

const char *const sqlSelectSpeedLimits = "SELECT" SELECT_SPEED_LIMIT_FIELDS "  FROM speed_limit t"
                                         "  ORDER BY t.limit_id;";

const char *const sqlInsertSpeedLimit = "INSERT INTO speed_limit(limit_id, name, notes, enabled,"
                                        "    inbound, period_enabled, period_id, packet_loss,"
                                        "    latency, kbps, bufsize, mod_time)"
                                        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11,"
                                        "    ?12);";

const char *const sqlUpdateSpeedLimit = "UPDATE speed_limit"
                                        "  SET name = ?2, notes = ?3, enabled = ?4, inbound = ?5,"
                                        "    period_enabled = ?6, period_id = ?7,"
                                        "    packet_loss = ?8, latency = ?9, kbps = ?10,"
                                        "    bufsize = ?11, mod_time = ?12"
                                        "  WHERE limit_id = ?1;";

const char *const sqlSelectSpeedLimitNameById = "SELECT name FROM speed_limit WHERE limit_id = ?1;";

const char *const sqlSelectSpeedLimitIds = "SELECT limit_id FROM speed_limit"
                                           "  WHERE limit_id <= ?1 ORDER BY limit_id;";

const char *const sqlDeleteSpeedLimit = "DELETE FROM speed_limit WHERE limit_id = ?1;";

const char *const sqlDeleteAppInSpeedLimit = "UPDATE app SET in_limit_id = NULL"
                                             "  WHERE in_limit_id = ?1;";

const char *const sqlDeleteAppOutSpeedLimit = "UPDATE app SET out_limit_id = NULL"
                                              "  WHERE out_limit_id = ?1;";

const char *const sqlUpdateSpeedLimitName = "UPDATE speed_limit SET name = ?2 WHERE limit_id = ?1;";

const char *const sqlUpdateSpeedLimitEnabled =
        "UPDATE speed_limit SET enabled = ?2 WHERE limit_id = ?1;";

bool driverWriteSpeedLimits(ConfBuffer &confBuf, bool onlyFlags = false)
{
    if (confBuf.hasError()) {
        qCWarning(LC) << "Driver config error:" << confBuf.errorMessage();
        return false;
    }

    auto driverManager = IoC<DriverManager>();
    if (!driverManager->writeSpeedLimits(confBuf.buffer(), onlyFlags)) {
        qCWarning(LC) << "Update driver error:" << driverManager->errorMessage();
        return false;
    }

    return true;
}

}

ConfSpeedLimitManager::ConfSpeedLimitManager(QObject *parent) : ConfManagerBase(parent)
{
    setupSpeedLimitNamesCache();
}

void ConfSpeedLimitManager::setUp()
{
    auto confManager = Fort::dependency<ConfManager>();
    auto confTimePeriodManager = Fort::dependency<ConfTimePeriodManager>();

    // The imported DB may reuse the ids
    connect(confManager, &ConfManager::imported, this,
            &ConfSpeedLimitManager::clearSpeedLimitNamesCache);

    connect(confTimePeriodManager, &ConfTimePeriodManager::activePeriodsChanged, this,
            &ConfSpeedLimitManager::updateDriverSpeedLimitFlags);
}

QString ConfSpeedLimitManager::speedLimitNameById(quint8 limitId)
{
    if (limitId == 0)
        return {};

    QString name = m_speedLimitNamesCache.value(limitId);

    if (name.isEmpty()) {
        name = DbQuery(sqliteDb())
                       .sql(sqlSelectSpeedLimitNameById)
                       .vars({ limitId })
                       .execute()
                       .toString();

        m_speedLimitNamesCache.insert(limitId, name);
    }

    return name;
}

bool ConfSpeedLimitManager::addOrUpdateSpeedLimit(SpeedLimit &limit)
{
    bool ok = true;

    beginWriteTransaction();

    const bool isNew = (limit.limitId == 0);
    if (isNew) {
        limit.limitId = DbQuery(sqliteDb(), &ok)
                                .sql(sqlSelectSpeedLimitIds)
                                .vars({ ConfUtil::speedLimitMaxCount() })
                                .getFreeId(/*maxId=*/ConfUtil::speedLimitMaxCount());
    }

    if (ok) {
        const QVariantList vars = {
            limit.limitId,
            limit.name,
            limit.notes,
            limit.enabled,
            limit.inbound,
            limit.periodEnabled,
            DbVar::nullable(limit.periodId),
            limit.packetLoss,
            limit.latency,
            limit.kbps,
            limit.bufferSize,
            DateUtil::now(),
        };

        DbQuery(sqliteDb(), &ok)
                .sql(isNew ? sqlInsertSpeedLimit : sqlUpdateSpeedLimit)
                .vars(vars)
                .executeOk();
    }

    endTransaction(ok);

    if (!ok)
        return false;

    if (isNew) {
        emit speedLimitAdded();
    } else {
        emit speedLimitUpdated();
    }

    updateDriverSpeedLimits();

    return true;
}

bool ConfSpeedLimitManager::deleteSpeedLimit(quint8 limitId)
{
    bool ok = false;

    beginWriteTransaction();

    const QVariantList vars = { limitId };

    if (DbQuery(sqliteDb(), &ok).sql(sqlDeleteSpeedLimit).vars(vars).executeOk()) {
        // Delete the Speed Limit from Programs
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteAppInSpeedLimit).vars(vars).executeOk();
        DbQuery(sqliteDb(), &ok).sql(sqlDeleteAppOutSpeedLimit).vars(vars).executeOk();
    }

    endTransaction(ok);

    if (ok) {
        emit speedLimitRemoved(limitId);

        updateDriverSpeedLimits();
    }

    return ok;
}

bool ConfSpeedLimitManager::updateSpeedLimitName(quint8 limitId, const QString &name)
{
    bool ok = false;

    beginWriteTransaction();

    const QVariantList vars = { limitId, name };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateSpeedLimitName).vars(vars).executeOk();

    endTransaction(ok);

    if (ok) {
        emit speedLimitUpdated();
    }

    return ok;
}

bool ConfSpeedLimitManager::updateSpeedLimitEnabled(quint8 limitId, bool enabled)
{
    bool ok = false;

    beginWriteTransaction();

    const QVariantList vars = { limitId, enabled };

    DbQuery(sqliteDb(), &ok).sql(sqlUpdateSpeedLimitEnabled).vars(vars).executeOk();

    endTransaction(ok);

    if (ok) {
        emit speedLimitUpdated();

        updateDriverSpeedLimitFlags(); // the Speed Limit's Time Period may keep it inactive
    }

    return ok;
}

bool ConfSpeedLimitManager::walkSpeedLimits(
        const std::function<walkSpeedLimitsCallback> &func) const
{
    SqliteStmt stmt;
    if (!DbQuery(sqliteDb()).sql(sqlSelectSpeedLimits).prepare(stmt))
        return false;

    while (stmt.step() == SqliteStmt::StepRow) {
        SpeedLimit limit;
        fillSpeedLimit(limit, stmt);

        if (!func(limit))
            return false;
    }

    return true;
}

void ConfSpeedLimitManager::updateDriverSpeedLimits()
{
    const quint32 activeMask = activeSpeedLimitsMask();

    ConfBuffer confBuf;

    confBuf.writeSpeedLimits(*this, activeMask);

    if (driverWriteSpeedLimits(confBuf)) {
        m_driverActiveMask = activeMask;
    }
}

void ConfSpeedLimitManager::updateDriverSpeedLimitFlags()
{
    const quint32 activeMask = activeSpeedLimitsMask();

    if (activeMask == m_driverActiveMask)
        return;

    ConfBuffer confBuf;

    confBuf.writeSpeedLimitFlags(activeMask);

    if (driverWriteSpeedLimits(confBuf, /*onlyFlags=*/true)) {
        m_driverActiveMask = activeMask;
    }
}

quint32 ConfSpeedLimitManager::activeSpeedLimitsMask() const
{
    auto confTimePeriodManager = Fort::confTimePeriodManager();

    quint32 activeMask = 0;

    walkSpeedLimits([&](const SpeedLimit &limit) -> bool {
        if (Q_UNLIKELY(limit.limitId <= 0 || limit.limitId > ConfUtil::speedLimitMaxCount()))
            return true; // skip an out of range Speed Limit

        if (limit.enabled
                && confTimePeriodManager->isTimePeriodActive(limit.periodEnabled, limit.periodId)) {
            activeMask |= (quint32(1) << (limit.limitId - 1));
        }

        return true;
    });

    return activeMask;
}

void ConfSpeedLimitManager::setupSpeedLimitNamesCache()
{
    connect(this, &ConfSpeedLimitManager::speedLimitRemoved, this,
            &ConfSpeedLimitManager::clearSpeedLimitNamesCache);
    connect(this, &ConfSpeedLimitManager::speedLimitUpdated, this,
            &ConfSpeedLimitManager::clearSpeedLimitNamesCache);
}

void ConfSpeedLimitManager::clearSpeedLimitNamesCache()
{
    m_speedLimitNamesCache.clear();
}

void ConfSpeedLimitManager::fillSpeedLimit(SpeedLimit &limit, const SqliteStmt &stmt)
{
    limit.limitId = stmt.columnInt(0);
    limit.enabled = stmt.columnBool(1);
    limit.inbound = stmt.columnBool(2);
    limit.periodEnabled = stmt.columnBool(3);
    limit.periodId = stmt.columnInt(4);
    limit.packetLoss = stmt.columnInt(5);
    limit.latency = stmt.columnUInt(6);
    limit.kbps = stmt.columnUInt(7);
    limit.bufferSize = stmt.columnUInt(8);
    limit.name = stmt.columnText(9);
    limit.notes = stmt.columnText(10);
    limit.modTime = stmt.columnDateTime(11);
}
